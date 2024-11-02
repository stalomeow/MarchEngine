#include "RenderGraphResource.h"
#include "GfxDevice.h"
#include "Debug.h"
#include <limits>
#include <utility>
#include <stdexcept>

#undef min
#undef max

namespace march
{
    RenderGraphResourceReadFlags operator|(RenderGraphResourceReadFlags lhs, RenderGraphResourceReadFlags rhs)
    {
        return static_cast<RenderGraphResourceReadFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    RenderGraphResourceReadFlags& operator|=(RenderGraphResourceReadFlags& lhs, RenderGraphResourceReadFlags rhs)
    {
        return lhs = lhs | rhs;
    }

    RenderGraphResourceReadFlags operator&(RenderGraphResourceReadFlags lhs, RenderGraphResourceReadFlags rhs)
    {
        return static_cast<RenderGraphResourceReadFlags>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }

    RenderGraphResourceReadFlags& operator&=(RenderGraphResourceReadFlags& lhs, RenderGraphResourceReadFlags rhs)
    {
        return lhs = lhs & rhs;
    }

    RenderGraphResourceWriteFlags operator|(RenderGraphResourceWriteFlags lhs, RenderGraphResourceWriteFlags rhs)
    {
        return static_cast<RenderGraphResourceWriteFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    RenderGraphResourceWriteFlags& operator|=(RenderGraphResourceWriteFlags& lhs, RenderGraphResourceWriteFlags rhs)
    {
        return lhs = lhs | rhs;
    }

    RenderGraphResourceWriteFlags operator&(RenderGraphResourceWriteFlags lhs, RenderGraphResourceWriteFlags rhs)
    {
        return static_cast<RenderGraphResourceWriteFlags>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }

    RenderGraphResourceWriteFlags& operator&=(RenderGraphResourceWriteFlags& lhs, RenderGraphResourceWriteFlags rhs)
    {
        return lhs = lhs & rhs;
    }

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
            LOG_WARNING("Trying to return a texture that is not from the pool");
            return;
        }

        m_FreeTextures.push_back({ texture, 0 });
    }

    RenderGraphResourceData::RenderGraphResourceData(RenderGraphResourcePool* pool, const GfxRenderTextureDesc& desc)
        : m_ProducerPasses{}
        , m_ResourceType(RenderGraphResourceType::Texture)
        , m_ResourcePtr(nullptr)
        , m_TransientResourcePool(pool)
        , m_TransientTextureDesc(desc)
        , m_TransientLifeTimeMinIndex(std::numeric_limits<int32_t>::max())
        , m_TransientLifeTimeMaxIndex(std::numeric_limits<int32_t>::min())
    {
    }

    RenderGraphResourceData::RenderGraphResourceData(GfxRenderTexture* texture)
        : m_ProducerPasses{}
        , m_ResourceType(RenderGraphResourceType::Texture)
        , m_ResourcePtr(texture)
        , m_TransientResourcePool(nullptr)
        , m_TransientTextureDesc{}
        , m_TransientLifeTimeMinIndex(std::numeric_limits<int32_t>::max())
        , m_TransientLifeTimeMaxIndex(std::numeric_limits<int32_t>::min())
    {
    }

    int32_t RenderGraphResourceData::GetLastProducerPass() const
    {
        if (m_ProducerPasses.empty())
        {
            return -1;
        }

        return m_ProducerPasses.back();
    }

    void RenderGraphResourceData::AddProducerPass(int32_t passIndex)
    {
        m_ProducerPasses.push_back(passIndex);
    }

    RenderGraphResourceType RenderGraphResourceData::GetResourceType() const
    {
        return m_ResourceType;
    }

    GfxResource* RenderGraphResourceData::GetResourcePtr() const
    {
        return m_ResourcePtr;
    }

    GfxRenderTextureDesc RenderGraphResourceData::GetTextureDesc() const
    {
        if (m_ResourceType != RenderGraphResourceType::Texture)
        {
            throw std::runtime_error("Resource is not a texture");
        }

        if (IsTransient())
        {
            return m_TransientTextureDesc;
        }

        return static_cast<GfxRenderTexture*>(m_ResourcePtr)->GetDesc();
    }

    bool RenderGraphResourceData::IsTransient() const
    {
        return m_TransientResourcePool != nullptr;
    }

    void RenderGraphResourceData::RentTransientResource()
    {
        if (!IsTransient())
        {
            return;
        }

        if (m_ResourceType == RenderGraphResourceType::Texture)
        {
            m_ResourcePtr = m_TransientResourcePool->RentTexture(m_TransientTextureDesc);
        }
    }

    void RenderGraphResourceData::ReturnTransientResource()
    {
        if (!IsTransient())
        {
            return;
        }

        if (m_ResourceType == RenderGraphResourceType::Texture)
        {
            m_TransientResourcePool->ReturnTexture(static_cast<GfxRenderTexture*>(m_ResourcePtr));
        }
    }

    void RenderGraphResourceData::UpdateTransientLifeTime(int32_t index)
    {
        if (!IsTransient())
        {
            return;
        }

        m_TransientLifeTimeMinIndex = std::min(m_TransientLifeTimeMinIndex, index);
        m_TransientLifeTimeMaxIndex = std::max(m_TransientLifeTimeMaxIndex, index);
    }

    int32_t RenderGraphResourceData::GetTransientLifeTimeMinIndex() const
    {
        return IsTransient() ? m_TransientLifeTimeMinIndex : -1;
    }

    int32_t RenderGraphResourceData::GetTransientLifeTimeMaxIndex() const
    {
        return IsTransient() ? m_TransientLifeTimeMaxIndex : -1;
    }
}
