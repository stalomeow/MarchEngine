#include "RenderGraphPass.h"
#include "RenderGraph.h"

namespace march
{
    RenderGraphPass::RenderGraphPass(const std::string& name)
        : m_Name(name)
        , m_AllowPassCulling(true)
        , m_ResourcesWritten{}
        , m_ResourcesRead{}
        , m_TexturesCreated{}
        , m_SortState(RenderGraphPassSortState::None)
        , m_NextPasses{}
        , m_ResourcesBorn{}
        , m_ResourcesDead{}
    {
    }

    const std::string& RenderGraphPass::GetName() const
    {
        return m_Name;
    }

    RGTextureHandle::RGTextureHandle(RenderGraph* graph, int32_t resourceId) : m_Graph(graph), m_ResourceId(resourceId)
    {
    }

    GfxRenderTexture* RGTextureHandle::GetTexture()
    {
        RenderGraphResourceData& data = m_Graph->GetResourceData(m_ResourceId);

        if (data.ResourceType != RenderGraphResourceType::Texture)
        {
            return nullptr;
        }

        return static_cast<GfxRenderTexture*>(data.ResourcePtr);
    }

    RenderGraphBuilder::RenderGraphBuilder(RenderGraph* graph, RenderGraphPass* pass) : m_Graph(graph), m_Pass(pass)
    {
        // 清理 pass
        pass->m_AllowPassCulling = true;
        pass->m_ResourcesWritten.clear();
        pass->m_ResourcesRead.clear();
        pass->m_TexturesCreated.clear();

        pass->m_SortState = RenderGraphPassSortState::None;
        pass->m_NextPasses.clear();
        pass->m_ResourcesBorn.clear();
        pass->m_ResourcesDead.clear();
    }

    void RenderGraphBuilder::AllowPassCulling(bool value)
    {
        m_Pass->m_AllowPassCulling = value;
    }

    void RenderGraphBuilder::CreateTexture(int32_t id, const GfxRenderTextureDesc& desc)
    {
        m_Pass->m_TexturesCreated[id] = desc;
    }

    RGTextureHandle RenderGraphBuilder::ReadTexture(int32_t id)
    {
        m_Pass->m_ResourcesRead.insert(id);
        return RGTextureHandle(m_Graph, id);
    }

    RGTextureHandle RenderGraphBuilder::WriteTexture(int32_t id)
    {
        m_Pass->m_ResourcesWritten.insert(id);
        return RGTextureHandle(m_Graph, id);
    }
}
