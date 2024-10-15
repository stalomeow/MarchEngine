#pragma once

#include "GfxTexture.h"
#include <memory>
#include <list>

namespace march
{
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

        GfxRenderTexture* RentTexture(const GfxRenderTextureDesc& desc);
        void ReturnTexture(GfxRenderTexture* texture);

    private:
        std::list<std::unique_ptr<GfxRenderTexture>> m_AllTextures;
        std::unordered_map<GfxRenderTexture*, decltype(m_AllTextures)::iterator> m_TextureMap;
        std::list<FreeTexture> m_FreeTextures; // 新的都 push 到最后，这样前面都是旧的
    };
}
