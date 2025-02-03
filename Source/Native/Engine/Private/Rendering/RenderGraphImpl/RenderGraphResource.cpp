#include "pch.h"
#include "Engine/Rendering/RenderGraphImpl/RenderGraphResource.h"
#include "Engine/Debug.h"
#include <utility>
#include <stdexcept>
#include <assert.h>

namespace march
{
    // https://en.cppreference.com/w/cpp/utility/variant/visit2
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    bool RenderGraphResourceData::IsExternal() const
    {
        return std::visit([](auto&& arg) -> bool
        {
            using T = std::decay_t<decltype(arg)>;

            constexpr bool isExternalBuffer = std::is_same_v<T, RenderGraphResourceExternalBuffer>;
            constexpr bool isExternalTexture = std::is_same_v<T, RenderGraphResourceExternalTexture>;
            constexpr bool result = isExternalBuffer || isExternalTexture;

            return result;
        }, m_Resource);
    }

    GfxBuffer* RenderGraphResourceData::GetBuffer()
    {
        return std::visit(overloaded{
            [](RenderGraphResourceTempBuffer& buffer) -> GfxBuffer* { return &buffer.Buffer; },
            [](RenderGraphResourcePooledBuffer& buffer) -> GfxBuffer* { return buffer.Buffer.get(); },
            [](RenderGraphResourceExternalBuffer& buffer) -> GfxBuffer* { return buffer.Buffer; },
            [](auto&&) -> GfxBuffer* { throw std::runtime_error("Resource is not a buffer"); },
        }, m_Resource);
    }

    const GfxBufferDesc& RenderGraphResourceData::GetBufferDesc() const
    {
        return std::visit(overloaded{
            [](const RenderGraphResourceTempBuffer& buffer) -> const GfxBufferDesc& { return buffer.Buffer.GetDesc(); },
            [](const RenderGraphResourcePooledBuffer& buffer) -> const GfxBufferDesc& { return buffer.Desc; },
            [](const RenderGraphResourceExternalBuffer& buffer) -> const GfxBufferDesc& { return buffer.Buffer->GetDesc(); },
            [](auto&&) -> const GfxBufferDesc& { throw std::runtime_error("Resource is not a buffer"); },
        }, m_Resource);
    }

    GfxRenderTexture* RenderGraphResourceData::GetTexture()
    {
        return std::visit(overloaded{
            [](RenderGraphResourcePooledTexture& texture) -> GfxRenderTexture* { return texture.Texture.get(); },
            [](RenderGraphResourceExternalTexture& texture) -> GfxRenderTexture* { return texture.Texture; },
            [](auto&&) -> GfxRenderTexture* { throw std::runtime_error("Resource is not a texture"); },
        }, m_Resource);
    }

    const GfxTextureDesc& RenderGraphResourceData::GetTextureDesc() const
    {
        return std::visit(overloaded{
            [](const RenderGraphResourcePooledTexture& texture) -> const GfxTextureDesc& { return texture.Desc; },
            [](const RenderGraphResourceExternalTexture& texture) -> const GfxTextureDesc& { return texture.Texture->GetDesc(); },
            [](auto&&) -> const GfxTextureDesc& { throw std::runtime_error("Resource is not a texture"); },
        }, m_Resource);
    }

    void RenderGraphResourceData::RequestResource()
    {
        std::visit(overloaded{
            [](RenderGraphResourcePooledBuffer& buffer) { buffer.RequestBuffer(); },
            [](RenderGraphResourcePooledTexture& texture) { texture.RequestTexture(); },
            [](auto&&) {},
        }, m_Resource);
    }

    void RenderGraphResourceData::ReleaseResource()
    {
        std::visit(overloaded{
            [](RenderGraphResourcePooledBuffer& buffer) { buffer.ReleaseBuffer(); },
            [](RenderGraphResourcePooledTexture& texture) { texture.ReleaseTexture(); },
            [](auto&&) {},
        }, m_Resource);
    }

    std::optional<size_t> RenderGraphResourceData::GetLastProducerBeforePassIndex(size_t passIndex) const
    {
        // 如果自己是自己的 producer 就会产生环，所以要从后往前找第一个不等于自己的 producer
        for (auto it = m_ProducerPassIndices.rbegin(); it != m_ProducerPassIndices.rend(); ++it)
        {
            if (size_t v = *it; v != passIndex)
            {
                return v;
            }
        }

        return std::nullopt;
    }

    void RenderGraphResourceData::AddProducerPassIndex(size_t passIndex)
    {
        m_ProducerPassIndices.push_back(passIndex);
    }

    void RenderGraphResourceData::SetAlive(size_t passIndex)
    {
        if (m_LifetimePassIndexRange)
        {
            std::pair<size_t, size_t>& range = m_LifetimePassIndexRange.value();
            range.first = std::min(range.first, passIndex);
            range.second = std::max(range.second, passIndex);
        }
        else
        {
            m_LifetimePassIndexRange = std::make_pair(passIndex, passIndex);
        }
    }

    RenderGraphResourceManager::RenderGraphResourceManager()
        : m_Resources{}
    {
        m_BufferPool = std::make_unique<RenderGraphResourcePool<GfxBuffer>>();
        m_TexturePool = std::make_unique<RenderGraphResourcePool<GfxRenderTexture>>();
    }

    void RenderGraphResourceManager::ClearResources()
    {
        m_Resources.clear();
    }

    BufferHandle RenderGraphResourceManager::CreateBuffer(int32 id, const GfxBufferDesc& desc)
    {
        RenderGraphResourceData& resData = m_Resources.emplace_back();
        GfxBufferAllocStrategy allocStrategy = desc.GetAllocStrategy();

        if (GfxBufferAllocUtils::IsSubAlloc(allocStrategy))
        {
            // SubAlloc 开销小，没必要再复用了
            resData.InitAsTempBuffer(id, desc);
        }
        else if (GfxBufferAllocUtils::IsHeapCpuAccessible(allocStrategy))
        {
            // 在 GPU 没使用完成前，复用 CPU 可访问的 Buffer 可能导致 CPU 和 GPU 之间的竞争
            // 另外，对于 CPU 可访问的 Buffer，GfxBuffer::SetData(...) 每次都会重新创建资源，复用了也没什么意义
            LOG_WARNING("RenderGraphBuffer '{}' can not be pooled because it is CPU accessible. Consider to use a sub-allocated buffer instead.", ShaderUtils::GetStringFromId(id));

            resData.InitAsTempBuffer(id, desc);
        }
        else
        {
            resData.InitAsPooledBuffer(id, desc, m_BufferPool.get());
        }

        return BufferHandle(this, m_Resources.size() - 1);
    }

    BufferHandle RenderGraphResourceManager::ImportBuffer(int32 id, GfxBuffer* buffer)
    {
        RenderGraphResourceData& resData = m_Resources.emplace_back();
        resData.InitAsExternalBuffer(id, buffer);
        return BufferHandle(this, m_Resources.size() - 1);
    }

    TextureHandle RenderGraphResourceManager::CreateTexture(int32 id, const GfxTextureDesc& desc)
    {
        RenderGraphResourceData& resData = m_Resources.emplace_back();
        resData.InitAsPooledTexture(id, desc, m_TexturePool.get());
        return TextureHandle(this, m_Resources.size() - 1);
    }

    TextureHandle RenderGraphResourceManager::ImportTexture(int32 id, GfxRenderTexture* texture)
    {
        RenderGraphResourceData& resData = m_Resources.emplace_back();
        resData.InitAsExternalTexture(id, texture);
        return TextureHandle(this, m_Resources.size() - 1);
    }

    size_t RenderGraphResourceManager::GetResourceIndex(const BufferHandle& handle) const
    {
        assert(handle.m_Manager == this);
        return handle.m_ResourceIndex;
    }

    size_t RenderGraphResourceManager::GetResourceIndex(const TextureHandle& handle) const
    {
        assert(handle.m_Manager == this);
        return handle.m_ResourceIndex;
    }

    int32 RenderGraphResourceManager::GetResourceId(size_t resourceIndex) const
    {
        return m_Resources[resourceIndex].GetId();
    }

    bool RenderGraphResourceManager::IsResourceExternal(size_t resourceIndex) const
    {
        return m_Resources[resourceIndex].IsExternal();
    }

    GfxBuffer* RenderGraphResourceManager::GetBuffer(size_t resourceIndex)
    {
        return m_Resources[resourceIndex].GetBuffer();
    }

    const GfxBufferDesc& RenderGraphResourceManager::GetBufferDesc(size_t resourceIndex) const
    {
        return m_Resources[resourceIndex].GetBufferDesc();
    }

    GfxRenderTexture* RenderGraphResourceManager::GetTexture(size_t resourceIndex)
    {
        return m_Resources[resourceIndex].GetTexture();
    }

    const GfxTextureDesc& RenderGraphResourceManager::GetTextureDesc(size_t resourceIndex) const
    {
        return m_Resources[resourceIndex].GetTextureDesc();
    }

    void RenderGraphResourceManager::RequestResource(size_t resourceIndex)
    {
        m_Resources[resourceIndex].RequestResource();
    }

    void RenderGraphResourceManager::ReleaseResource(size_t resourceIndex)
    {
        m_Resources[resourceIndex].ReleaseResource();
    }

    std::optional<size_t> RenderGraphResourceManager::GetLastProducerBeforePassIndex(size_t resourceIndex, size_t passIndex) const
    {
        return m_Resources[resourceIndex].GetLastProducerBeforePassIndex(passIndex);
    }

    void RenderGraphResourceManager::AddProducerPassIndex(size_t resourceIndex, size_t passIndex)
    {
        m_Resources[resourceIndex].AddProducerPassIndex(passIndex);
    }

    void RenderGraphResourceManager::SetAlive(size_t resourceIndex, size_t passIndex)
    {
        m_Resources[resourceIndex].SetAlive(passIndex);
    }

    std::optional<std::pair<size_t, size_t>> RenderGraphResourceManager::GetLifetimePassIndexRange(size_t resourceIndex) const
    {
        return m_Resources[resourceIndex].GetLifetimePassIndexRange();
    }
}
