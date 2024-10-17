#pragma once

#include "GfxTexture.h"
#include <memory>
#include <list>
#include <vector>
#include <stdint.h>
#include <unordered_map>

namespace march
{
    enum class RenderGraphResourceType
    {
        Texture,
    };

    enum class RenderGraphResourceReadFlags
    {
        None = 0,
        Copy = 1 << 0,
        Resolve = 1 << 1,
        PixelShader = 1 << 2,
        NonPixelShader = 1 << 3,
        AllShader = PixelShader | NonPixelShader,
    };

    RenderGraphResourceReadFlags operator|(RenderGraphResourceReadFlags lhs, RenderGraphResourceReadFlags rhs);
    RenderGraphResourceReadFlags& operator|=(RenderGraphResourceReadFlags& lhs, RenderGraphResourceReadFlags rhs);
    RenderGraphResourceReadFlags operator&(RenderGraphResourceReadFlags lhs, RenderGraphResourceReadFlags rhs);
    RenderGraphResourceReadFlags& operator&=(RenderGraphResourceReadFlags& lhs, RenderGraphResourceReadFlags rhs);

    enum class RenderGraphResourceWriteFlags
    {
        None = 0,
        Copy = 1 << 0,
        Resolve = 1 << 1,
    };

    RenderGraphResourceWriteFlags operator|(RenderGraphResourceWriteFlags lhs, RenderGraphResourceWriteFlags rhs);
    RenderGraphResourceWriteFlags& operator|=(RenderGraphResourceWriteFlags& lhs, RenderGraphResourceWriteFlags rhs);
    RenderGraphResourceWriteFlags operator&(RenderGraphResourceWriteFlags lhs, RenderGraphResourceWriteFlags rhs);
    RenderGraphResourceWriteFlags& operator&=(RenderGraphResourceWriteFlags& lhs, RenderGraphResourceWriteFlags rhs);

    class RenderGraphResourcePool final
    {
        struct FreeTexture
        {
            GfxRenderTexture* Texture;
            uint32_t FailCount;
        };

        const uint32_t MaxFailCount = 20;

    public:
        RenderGraphResourcePool();
        ~RenderGraphResourcePool() = default;

        RenderGraphResourcePool(const RenderGraphResourcePool&) = delete;
        RenderGraphResourcePool& operator=(const RenderGraphResourcePool&) = delete;

        RenderGraphResourcePool(RenderGraphResourcePool&&) = default;
        RenderGraphResourcePool& operator=(RenderGraphResourcePool&&) = default;

        GfxRenderTexture* RentTexture(const GfxRenderTextureDesc& desc);
        void ReturnTexture(GfxRenderTexture* texture);

    private:
        std::list<std::unique_ptr<GfxRenderTexture>> m_AllTextures;
        std::unordered_map<GfxRenderTexture*, decltype(m_AllTextures)::iterator> m_TextureMap;
        std::list<FreeTexture> m_FreeTextures; // 新的都 push 到最后，这样前面都是旧的
    };

    class RenderGraphResourceData final
    {
    public:
        RenderGraphResourceData(RenderGraphResourcePool* pool, const GfxRenderTextureDesc& desc);
        RenderGraphResourceData(GfxRenderTexture* texture);
        ~RenderGraphResourceData() = default;

        RenderGraphResourceData(const RenderGraphResourceData&) = delete;
        RenderGraphResourceData& operator=(const RenderGraphResourceData&) = delete;

        RenderGraphResourceData(RenderGraphResourceData&&) = default;
        RenderGraphResourceData& operator=(RenderGraphResourceData&&) = default;

        int32_t GetLastProducerPass() const;
        void AddProducerPass(int32_t passIndex);

        RenderGraphResourceType GetResourceType() const;
        GfxResource* GetResourcePtr() const;
        GfxRenderTextureDesc GetTextureDesc() const;

        bool IsTransient() const;
        void RentTransientResource();
        void ReturnTransientResource();
        void UpdateTransientLifeTime(int32_t index);
        int32_t GetTransientLifeTimeMinIndex() const;
        int32_t GetTransientLifeTimeMaxIndex() const;

    private:
        std::vector<int32_t> m_ProducerPasses;

        RenderGraphResourceType m_ResourceType;
        GfxResource* m_ResourcePtr;

        RenderGraphResourcePool* m_TransientResourcePool;
        GfxRenderTextureDesc m_TransientTextureDesc;
        int32_t m_TransientLifeTimeMinIndex;
        int32_t m_TransientLifeTimeMaxIndex;
    };
}
