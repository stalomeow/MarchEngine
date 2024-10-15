#include "RenderGraphResourcePool.h"
#include "GfxDevice.h"
#include "Debug.h"

namespace march
{
    RenderGraphResourcePool::RenderGraphResourcePool() : m_AllTextures{}, m_TextureMap{}, m_FreeTextures{}
    {
    }

    GfxRenderTexture* RenderGraphResourcePool::RentTexture(const GfxRenderTextureDesc& desc)
    {
        auto it = m_FreeTextures.begin();

        while (it != m_FreeTextures.end())
        {
            GfxRenderTexture* tex = it->Texture;

            if (tex->GetDesc().IsCompatibleWith(desc))
            {
                m_FreeTextures.erase(it);
                return tex;
            }

            it->FailCount++;

            // 失败次数太多
            if (it->FailCount >= MaxFailCount)
            {
                auto texIt = m_TextureMap.find(tex);
                m_AllTextures.erase(texIt->second);
                m_TextureMap.erase(texIt);
                it = m_FreeTextures.erase(it);
            }
            else
            {
                ++it;
            }
        }

        GfxDevice* device = GetGfxDevice();
        m_AllTextures.emplace_back(std::make_unique<GfxRenderTexture>(device, "PooledTexture", desc));
        GfxRenderTexture* texture = m_AllTextures.back().get();
        m_TextureMap[texture] = std::prev(m_AllTextures.end());
        return texture;
    }

    void RenderGraphResourcePool::ReturnTexture(GfxRenderTexture* texture)
    {
        if (m_TextureMap.find(texture) == m_TextureMap.end())
        {
            DEBUG_LOG_WARN("Trying to return a texture that is not from the pool");
            return;
        }

        m_FreeTextures.push_back({ texture, 0 });
    }
}
