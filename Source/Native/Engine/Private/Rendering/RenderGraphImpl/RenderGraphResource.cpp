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

    __forceinline static bool IsCpuAccessibleBuffer(const GfxBuffer* buffer)
    {
        return GfxBufferAllocUtils::IsHeapCpuAccessible(buffer->GetDesc().GetAllocStrategy());
    }

    bool RenderGraphResourceData::IsExternal() const
    {
        // 如果 temp buffer 是 CPU 可写的，那通常就是 CPU 在初始化时写入数据，之后不再修改，所以当成 external

        return std::visit(overloaded{
            [](const RenderGraphResourceTempBuffer& b) -> bool { return IsCpuAccessibleBuffer(&b.Buffer); },
            [](const RenderGraphResourcePooledBuffer& b) -> bool { return false; },
            [](const RenderGraphResourceExternalBuffer& b) -> bool { return true; },
            [](const RenderGraphResourcePooledTexture& t) -> bool { return false; },
            [](const RenderGraphResourceExternalTexture& t) -> bool { return true; },
            [](auto&&) -> bool { throw std::runtime_error("Resource is not a buffer or texture"); },
        }, m_Resource);
    }

    __forceinline static bool AllowGenericRead(RefCountPtr<GfxResource> resource)
    {
        // 允许把状态改为 GENERIC_READ 的也算
        return resource->HasAllStates(D3D12_RESOURCE_STATE_GENERIC_READ) || !resource->IsStateLocked();
    }

    __forceinline static bool AllowGenericRead(GfxBuffer* buffer)
    {
        return AllowGenericRead(buffer->GetUnderlyingResource());
    }

    __forceinline static bool AllowGenericRead(GfxTexture* texture)
    {
        return AllowGenericRead(texture->GetUnderlyingResource());
    }

    bool RenderGraphResourceData::IsGenericallyReadable()
    {
        // pool 中的资源是按需分配的（此处可能还没分配），且状态是可以转为 D3D12_RESOURCE_STATE_GENERIC_READ 的

        return std::visit(overloaded{
            [](RenderGraphResourceTempBuffer& b) -> bool { return AllowGenericRead(&b.Buffer); },
            [](RenderGraphResourcePooledBuffer& b) -> bool { return true; },
            [](RenderGraphResourceExternalBuffer& b) -> bool {return AllowGenericRead(b.Buffer); },
            [](RenderGraphResourcePooledTexture& t) -> bool { return true; },
            [](RenderGraphResourceExternalTexture& t) -> bool {return AllowGenericRead(t.Texture); },
            [](auto&&) -> bool { throw std::runtime_error("Resource is not a buffer or texture"); },
        }, m_Resource);
    }

    __forceinline static bool IsSubAllocatedBuffer(const GfxBuffer* buffer)
    {
        return GfxBufferAllocUtils::IsSubAlloc(buffer->GetDesc().GetAllocStrategy());
    }

    bool RenderGraphResourceData::AllowGpuWriting() const
    {
        // 如果 buffer 是 SubAlloc 得到的，绝对不能让 GPU 写，否则会有 resource barrier 改变整个原始资源的状态
        // PooledBuffer 一定不是 SubAlloc 的

        return std::visit(overloaded{
            [](const RenderGraphResourceTempBuffer& b) -> bool { return !IsSubAllocatedBuffer(&b.Buffer); },
            [](const RenderGraphResourcePooledBuffer& b) -> bool { return true; },
            [](const RenderGraphResourceExternalBuffer& b) -> bool { return !IsSubAllocatedBuffer(b.Buffer); },
            [](const RenderGraphResourcePooledTexture& t) -> bool { return true; },
            [](const RenderGraphResourceExternalTexture& t) -> bool { return !t.Texture->IsReadOnly(); },
            [](auto&&) -> bool { throw std::runtime_error("Resource is not a buffer or texture"); },
        }, m_Resource);
    }

    GfxBuffer* RenderGraphResourceData::GetBuffer()
    {
        return std::visit(overloaded{
            [](RenderGraphResourceTempBuffer& b) -> GfxBuffer* { return &b.Buffer; },
            [](RenderGraphResourcePooledBuffer& b) -> GfxBuffer* { return b.Buffer.get(); },
            [](RenderGraphResourceExternalBuffer& b) -> GfxBuffer* { return b.Buffer; },
            [](auto&&) -> GfxBuffer* { throw std::runtime_error("Resource is not a buffer"); },
        }, m_Resource);
    }

    const GfxBufferDesc& RenderGraphResourceData::GetBufferDesc() const
    {
        return std::visit(overloaded{
            [](const RenderGraphResourceTempBuffer& b) -> const GfxBufferDesc& { return b.Buffer.GetDesc(); },
            [](const RenderGraphResourcePooledBuffer& b) -> const GfxBufferDesc& { return b.Desc; },
            [](const RenderGraphResourceExternalBuffer& b) -> const GfxBufferDesc& { return b.Buffer->GetDesc(); },
            [](auto&&) -> const GfxBufferDesc& { throw std::runtime_error("Resource is not a buffer"); },
        }, m_Resource);
    }

    GfxTexture* RenderGraphResourceData::GetTexture()
    {
        return std::visit(overloaded{
            [](RenderGraphResourcePooledTexture& t) -> GfxTexture* { return t.Texture.get(); },
            [](RenderGraphResourceExternalTexture& t) -> GfxTexture* { return t.Texture; },
            [](auto&&) -> GfxTexture* { throw std::runtime_error("Resource is not a texture"); },
        }, m_Resource);
    }

    const GfxTextureDesc& RenderGraphResourceData::GetTextureDesc() const
    {
        return std::visit(overloaded{
            [](const RenderGraphResourcePooledTexture& t) -> const GfxTextureDesc& { return t.Desc; },
            [](const RenderGraphResourceExternalTexture& t) -> const GfxTextureDesc& { return t.Texture->GetDesc(); },
            [](auto&&) -> const GfxTextureDesc& { throw std::runtime_error("Resource is not a texture"); },
        }, m_Resource);
    }

    void RenderGraphResourceData::SetDefaultVariable(GfxCommandContext* cmd)
    {
        std::visit(overloaded{
            [id = m_Id, cmd](RenderGraphResourceTempBuffer& b) -> void { cmd->SetBuffer(id, &b.Buffer); },
            [id = m_Id, cmd](RenderGraphResourcePooledBuffer& b) -> void { cmd->SetBuffer(id, b.Buffer.get()); },
            [id = m_Id, cmd](RenderGraphResourceExternalBuffer& b) -> void { cmd->SetBuffer(id, b.Buffer); },
            [id = m_Id, cmd](RenderGraphResourcePooledTexture& t) -> void { cmd->SetTexture(id, t.Texture.get()); },
            [id = m_Id, cmd](RenderGraphResourceExternalTexture& t) -> void { cmd->SetTexture(id, t.Texture); },
            [](auto&&) -> void { throw std::runtime_error("Resource is not a buffer or texture"); },
        }, m_Resource);
    }

    RefCountPtr<GfxResource> RenderGraphResourceData::GetUnderlyingResource()
    {
        return std::visit(overloaded{
            [](RenderGraphResourceTempBuffer& b) -> RefCountPtr<GfxResource> { return b.Buffer.GetUnderlyingResource(); },
            [](RenderGraphResourcePooledBuffer& b) -> RefCountPtr<GfxResource> { return b.Buffer->GetUnderlyingResource(); },
            [](RenderGraphResourceExternalBuffer& b) -> RefCountPtr<GfxResource> { return b.Buffer->GetUnderlyingResource(); },
            [](RenderGraphResourcePooledTexture& t) -> RefCountPtr<GfxResource> { return t.Texture->GetUnderlyingResource(); },
            [](RenderGraphResourceExternalTexture& t) -> RefCountPtr<GfxResource> { return t.Texture->GetUnderlyingResource(); },
            [](auto&&) -> RefCountPtr<GfxResource> { throw std::runtime_error("Resource is not a buffer or texture"); },
        }, m_Resource);
    }

    void RenderGraphResourceData::RequestResource()
    {
        std::visit(overloaded{
            [](RenderGraphResourcePooledBuffer& b) { b.RequestBuffer(); },
            [](RenderGraphResourcePooledTexture& t) { t.RequestTexture(); },
            [](auto&&) {},
        }, m_Resource);
    }

    void RenderGraphResourceData::ReleaseResource()
    {
        std::visit(overloaded{
            [](RenderGraphResourcePooledBuffer& b) { b.ReleaseBuffer(); },
            [](RenderGraphResourcePooledTexture& t) { t.ReleaseTexture(); },
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

    BufferHandle RenderGraphResourceManager::CreateBuffer(int32 id, const GfxBufferDesc& desc, const void* pInitialData, std::optional<uint32_t> initialCounter)
    {
        RenderGraphResourceData& resData = m_Resources.emplace_back();
        GfxBufferAllocStrategy allocStrategy = desc.GetAllocStrategy();
        bool isHeapCpuAccessible = GfxBufferAllocUtils::IsHeapCpuAccessible(allocStrategy);

        if (GfxBufferAllocUtils::IsSubAlloc(allocStrategy))
        {
            // SubAlloc 开销小，没必要再复用了
            resData.InitAsTempBuffer(id, desc);
        }
        else if (isHeapCpuAccessible)
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

        if (pInitialData != nullptr || initialCounter)
        {
            if (isHeapCpuAccessible)
            {
                // 此时一定是 TempBuffer，会立刻分配资源，所以能放心设置 Data
                resData.GetBuffer()->SetData(pInitialData, initialCounter);
            }
            else
            {
                LOG_ERROR("Can not set the initial content of RenderGraphBuffer '{}' because it is not CPU accessible.", ShaderUtils::GetStringFromId(id));
            }
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

    TextureHandle RenderGraphResourceManager::ImportTexture(int32 id, GfxTexture* texture)
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

    bool RenderGraphResourceManager::IsExternalResource(size_t resourceIndex) const
    {
        return m_Resources[resourceIndex].IsExternal();
    }

    bool RenderGraphResourceManager::IsGenericallyReadableResource(size_t resourceIndex)
    {
        return m_Resources[resourceIndex].IsGenericallyReadable();
    }

    bool RenderGraphResourceManager::AllowGpuWritingResource(size_t resourceIndex) const
    {
        return m_Resources[resourceIndex].AllowGpuWriting();
    }

    GfxBuffer* RenderGraphResourceManager::GetBuffer(size_t resourceIndex)
    {
        return m_Resources[resourceIndex].GetBuffer();
    }

    const GfxBufferDesc& RenderGraphResourceManager::GetBufferDesc(size_t resourceIndex) const
    {
        return m_Resources[resourceIndex].GetBufferDesc();
    }

    GfxTexture* RenderGraphResourceManager::GetTexture(size_t resourceIndex)
    {
        return m_Resources[resourceIndex].GetTexture();
    }

    const GfxTextureDesc& RenderGraphResourceManager::GetTextureDesc(size_t resourceIndex) const
    {
        return m_Resources[resourceIndex].GetTextureDesc();
    }

    void RenderGraphResourceManager::SetDefaultVariable(size_t resourceIndex, GfxCommandContext* cmd)
    {
        m_Resources[resourceIndex].SetDefaultVariable(cmd);
    }

    RefCountPtr<GfxResource> RenderGraphResourceManager::GetUnderlyingResource(size_t resourceIndex)
    {
        return m_Resources[resourceIndex].GetUnderlyingResource();
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

    TextureHandle::operator TextureSliceHandle() const
    {
        return TextureSliceHandle{ *this, GfxCubemapFace::PositiveX, 0, 0 };
    }

    TextureSliceHandle TextureHandle::Slice2D(uint32_t mipSlice) const
    {
        return TextureSliceHandle{ *this, GfxCubemapFace::PositiveX, 0, mipSlice };
    }

    TextureSliceHandle TextureHandle::Slice3D(uint32_t wSlice, uint32_t mipSlice) const
    {
        return TextureSliceHandle{ *this, GfxCubemapFace::PositiveX, wSlice, mipSlice };
    }

    TextureSliceHandle TextureHandle::SliceCube(GfxCubemapFace face, uint32_t mipSlice) const
    {
        return TextureSliceHandle{ *this, face, 0, mipSlice };
    }

    TextureSliceHandle TextureHandle::Slice2DArray(uint32_t arraySlice, uint32_t mipSlice) const
    {
        return TextureSliceHandle{ *this, GfxCubemapFace::PositiveX, arraySlice, mipSlice };
    }

    TextureSliceHandle TextureHandle::SliceCubeArray(GfxCubemapFace face, uint32_t arraySlice, uint32_t mipSlice) const
    {
        return TextureSliceHandle{ *this, face, arraySlice, mipSlice };
    }
}
