#pragma once

#include "Engine/Ints.h"
#include "Engine/Rendering/D3D12.h"
#include <memory>
#include <list>
#include <vector>
#include <string>
#include <variant>
#include <optional>

namespace march
{
    template <typename _ResourceType>
    struct RenderGraphResourceTraits;

    template <>
    struct RenderGraphResourceTraits<GfxBuffer>
    {
        using DescType = GfxBufferDesc;

        static bool IsCompatibleWith(GfxBuffer* buffer, const DescType& desc)
        {
            return buffer->GetDesc().IsCompatibleWith(desc);
        }

        static std::unique_ptr<GfxBuffer> Allocate(const DescType& desc, uint32_t allocCounter)
        {
            GfxDevice* device = GetGfxDevice();
            std::string name = "RenderGraphBuffer" + std::to_string(allocCounter);
            return std::make_unique<GfxBuffer>(device, name, desc);
        }
    };

    template <>
    struct RenderGraphResourceTraits<GfxRenderTexture>
    {
        using DescType = GfxTextureDesc;

        static bool IsCompatibleWith(GfxRenderTexture* texture, const DescType& desc)
        {
            return texture->GetDesc().IsCompatibleWith(desc);
        }

        static std::unique_ptr<GfxRenderTexture> Allocate(const DescType& desc, uint32_t allocCounter)
        {
            GfxDevice* device = GetGfxDevice();
            std::string name = "RenderGraphTexture" + std::to_string(allocCounter);
            return std::make_unique<GfxRenderTexture>(device, name, desc, GfxTexureAllocStrategy::DefaultHeapPlaced);
        }
    };

    template <typename _ResourceType>
    class RenderGraphResourcePool
    {
        using ResourceTraits = RenderGraphResourceTraits<_ResourceType>;

        struct PoolItem
        {
            std::unique_ptr<_ResourceType> Res;
            uint32_t FailCount;
        };

        std::list<PoolItem> m_FreeItems{}; // 新的都 push 到最后，这样前面都是旧的
        uint32_t m_AllocCounter = 0; // 记录分配数量

    public:
        std::unique_ptr<_ResourceType> Request(const ResourceTraits::DescType& desc)
        {
            auto it = m_FreeItems.begin();

            while (it != m_FreeItems.end())
            {
                if (ResourceTraits::IsCompatibleWith(it->Res.get(), desc))
                {
                    std::unique_ptr<_ResourceType> result = std::move(it->Res);
                    m_FreeItems.erase(it);
                    return result;
                }

                if (it->FailCount++; it->FailCount >= 20)
                {
                    // 失败次数太多
                    it = m_FreeItems.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            return ResourceTraits::Allocate(desc, ++m_AllocCounter);
        }

        void Release(std::unique_ptr<_ResourceType>&& value)
        {
            m_FreeItems.push_back({ std::move(value), 0 });
        }
    };

    struct RenderGraphResourceTempBuffer
    {
        GfxBuffer Buffer; // 用完就扔，不循环使用

        RenderGraphResourceTempBuffer(const GfxBufferDesc& desc)
            : Buffer(GetGfxDevice(), "RenderGraphTempBuffer", desc)
        {
        }
    };

    struct RenderGraphResourcePooledBuffer
    {
        GfxBufferDesc Desc;
        RenderGraphResourcePool<GfxBuffer>* Pool;

        std::unique_ptr<GfxBuffer> Buffer;

        RenderGraphResourcePooledBuffer(const GfxBufferDesc& desc, RenderGraphResourcePool<GfxBuffer>* pool)
            : Desc(desc)
            , Pool(pool)
            , Buffer(nullptr)
        {
        }

        ~RenderGraphResourcePooledBuffer()
        {
            ReleaseBuffer();
        }

        void RequestBuffer()
        {
            if (!Buffer)
            {
                Buffer = Pool->Request(Desc);
            }
        }

        void ReleaseBuffer()
        {
            if (Buffer)
            {
                Pool->Release(std::move(Buffer));
                Buffer = nullptr;
            }
        }
    };

    struct RenderGraphResourceExternalBuffer
    {
        GfxBuffer* Buffer;

        RenderGraphResourceExternalBuffer(GfxBuffer* buffer) : Buffer(buffer) {}
    };

    struct RenderGraphResourcePooledTexture
    {
        GfxTextureDesc Desc;
        RenderGraphResourcePool<GfxRenderTexture>* Pool;

        std::unique_ptr<GfxRenderTexture> Texture;

        RenderGraphResourcePooledTexture(const GfxTextureDesc& desc, RenderGraphResourcePool<GfxRenderTexture>* pool)
            : Desc(desc)
            , Pool(pool)
            , Texture(nullptr)
        {
        }

        ~RenderGraphResourcePooledTexture()
        {
            ReleaseTexture();
        }

        void RequestTexture()
        {
            if (!Texture)
            {
                Texture = Pool->Request(Desc);
            }
        }

        void ReleaseTexture()
        {
            if (Texture)
            {
                Pool->Release(std::move(Texture));
                Texture = nullptr;
            }
        }
    };

    struct RenderGraphResourceExternalTexture
    {
        GfxRenderTexture* Texture;

        RenderGraphResourceExternalTexture(GfxRenderTexture* texture) : Texture(texture) {}
    };

    class RenderGraphResourceData final
    {
        int32 m_Id;

        std::variant<
            std::monostate,
            RenderGraphResourceTempBuffer,
            RenderGraphResourcePooledBuffer,
            RenderGraphResourceExternalBuffer,
            RenderGraphResourcePooledTexture,
            RenderGraphResourceExternalTexture,
        > m_Resource;

        std::vector<size_t> m_ProducerPassIndices;
        std::optional<std::pair<size_t, size_t>> m_LifetimePassIndexRange = std::nullopt;

    public:
        bool IsExternal() const;

        GfxBuffer* GetBuffer();
        const GfxBufferDesc& GetBufferDesc() const;

        GfxRenderTexture* GetTexture();
        const GfxTextureDesc& GetTextureDesc() const;

        void RequestResource();
        void ReleaseResource();

        std::optional<size_t> GetLastProducerBeforePassIndex(size_t passIndex) const;
        void AddProducerPassIndex(size_t passIndex);

        void SetAlive(size_t passIndex);

        int32 GetId() const { return m_Id; }

        std::optional<std::pair<size_t, size_t>> GetLifetimePassIndexRange() const { return m_LifetimePassIndexRange; }

        void InitAsTempBuffer(int32 id, const GfxBufferDesc& desc)
        {
            m_Id = id;
            m_Resource.emplace<RenderGraphResourceTempBuffer>(desc);
        }

        void InitAsPooledBuffer(int32 id, const GfxBufferDesc& desc, RenderGraphResourcePool<GfxBuffer>* pool)
        {
            m_Id = id;
            m_Resource.emplace<RenderGraphResourcePooledBuffer>(desc, pool);
        }

        void InitAsExternalBuffer(int32 id, GfxBuffer* buffer)
        {
            m_Id = id;
            m_Resource.emplace<RenderGraphResourceExternalBuffer>(buffer);
        }

        void InitAsPooledTexture(int32 id, const GfxTextureDesc& desc, RenderGraphResourcePool<GfxRenderTexture>* pool)
        {
            m_Id = id;
            m_Resource.emplace<RenderGraphResourcePooledTexture>(desc, pool);
        }

        void InitAsExternalTexture(int32 id, GfxRenderTexture* texture)
        {
            m_Id = id;
            m_Resource.emplace<RenderGraphResourceExternalTexture>(texture);
        }
    };

    class BufferHandle;
    class TextureHandle;

    class RenderGraphResourceManager
    {
        std::unique_ptr<RenderGraphResourcePool<GfxBuffer>> m_BufferPool;
        std::unique_ptr<RenderGraphResourcePool<GfxRenderTexture>> m_TexturePool;
        std::vector<RenderGraphResourceData> m_Resources;

    public:
        RenderGraphResourceManager();

        size_t GetNumResources() const { return m_Resources.size(); }

        void ClearResources();

        BufferHandle CreateBuffer(int32 id, const GfxBufferDesc& desc);
        BufferHandle ImportBuffer(int32 id, GfxBuffer* buffer);

        TextureHandle CreateTexture(int32 id, const GfxTextureDesc& desc);
        TextureHandle ImportTexture(int32 id, GfxRenderTexture* texture);

        size_t GetResourceIndex(const BufferHandle& handle) const;
        size_t GetResourceIndex(const TextureHandle& handle) const;

        int32 GetResourceId(size_t resourceIndex) const;

        bool IsResourceExternal(size_t resourceIndex) const;

        GfxBuffer* GetBuffer(size_t resourceIndex);
        const GfxBufferDesc& GetBufferDesc(size_t resourceIndex) const;

        GfxRenderTexture* GetTexture(size_t resourceIndex);
        const GfxTextureDesc& GetTextureDesc(size_t resourceIndex) const;

        void RequestResource(size_t resourceIndex);
        void ReleaseResource(size_t resourceIndex);

        std::optional<size_t> GetLastProducerBeforePassIndex(size_t resourceIndex, size_t passIndex) const;
        void AddProducerPassIndex(size_t resourceIndex, size_t passIndex);

        void SetAlive(size_t resourceIndex, size_t passIndex);

        std::optional<std::pair<size_t, size_t>> GetLifetimePassIndexRange(size_t resourceIndex) const;
    };

    class BufferHandle
    {
        friend RenderGraphResourceManager;

        RenderGraphResourceManager* m_Manager;
        size_t m_ResourceIndex;

        BufferHandle(RenderGraphResourceManager* manager, size_t resourceIndex)
            : m_Manager(manager)
            , m_ResourceIndex(resourceIndex)
        {
        }

    public:
        BufferHandle() = default;

        bool IsValid() const { return m_Manager != nullptr; }

        operator bool() const { return m_Manager != nullptr; }

        operator GfxBuffer* () const { return m_Manager->GetBuffer(m_ResourceIndex); }

        GfxBuffer* operator->() const { return m_Manager->GetBuffer(m_ResourceIndex); }

        int32 GetId() const { return m_Manager->GetResourceId(m_ResourceIndex); }

        const GfxBufferDesc& GetDesc() const { return m_Manager->GetBufferDesc(m_ResourceIndex); }
    };

    class TextureHandle
    {
        friend RenderGraphResourceManager;

        RenderGraphResourceManager* m_Manager;
        size_t m_ResourceIndex;

        TextureHandle(RenderGraphResourceManager* manager, size_t resourceIndex)
            : m_Manager(manager)
            , m_ResourceIndex(resourceIndex)
        {
        }

    public:
        TextureHandle() = default;

        bool IsValid() const { return m_Manager != nullptr; }

        operator bool() const { return m_Manager != nullptr; }

        operator GfxRenderTexture* () const { return m_Manager->GetTexture(m_ResourceIndex); }

        GfxRenderTexture* operator->() const { return m_Manager->GetTexture(m_ResourceIndex); }

        int32 GetId() const { return m_Manager->GetResourceId(m_ResourceIndex); }

        const GfxTextureDesc& GetDesc() const { return m_Manager->GetTextureDesc(m_ResourceIndex); }
    };
}
