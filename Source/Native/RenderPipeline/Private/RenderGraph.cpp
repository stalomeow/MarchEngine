#include "RenderGraph.h"
#include "GfxDevice.h"
#include "GfxCommand.h"
#include "Debug.h"
#include <utility>
#include <stdexcept>

#undef min
#undef max

namespace march
{
    RenderGraphContext::RenderGraphContext()
    {
        m_Context = GetGfxDevice()->RequestContext(GfxCommandType::Direct);
    }

    RenderGraphContext::~RenderGraphContext()
    {
        m_Context->SubmitAndRelease();
    }

    void RenderGraphContext::ClearPassData()
    {
        m_Context->ClearTextures();
    }

    RenderGraphPass::RenderGraphPass(const std::string& name)
        : Name(name)
        , HasSideEffects(false)
        , AllowPassCulling(true)
        , ResourcesRead{}
        , ResourcesWritten{}
        , NumColorTargets(0)
        , ColorTargets{}
        , DepthStencilTarget{}
        , RenderTargetsClearFlags(GfxClearFlags::None)
        , ClearColorValue{}
        , ClearDepthValue(0)
        , ClearStencilValue(0)
        , HasCustomViewport(false)
        , CustomViewport{}
        , HasCustomScissorRect(false)
        , CustomScissorRect{}
        , Wireframe(false)
        , SortState(RenderGraphPassSortState::None)
        , NextPasses{}
        , ResourcesBorn{}
        , ResourcesDead{}
        , RenderFunc{}
    {
    }

    RenderGraph::RenderGraph(bool emitEvents)
        : m_EmitEvents(emitEvents)
        , m_Passes{}
        , m_SortedPasses{}
        , m_ResourceDataMap{}
    {
        m_ResourcePool = std::make_unique<RenderGraphResourcePool>();
    }

    RenderGraphBuilder RenderGraph::AddPass()
    {
        return AddPass("Unnamed");
    }

    RenderGraphBuilder RenderGraph::AddPass(const std::string& name)
    {
        m_Passes.emplace_back(name);
        return RenderGraphBuilder(this, static_cast<int32_t>(m_Passes.size() - 1));
    }

    void RenderGraph::CompileAndExecute()
    {
        // TODO error guard

        try
        {
            if (CompilePasses())
            {
                if (m_EmitEvents)
                {
                    for (IRenderGraphCompiledEventListener* listener : s_GraphCompiledEventListeners)
                    {
                        listener->OnGraphCompiled(*this, m_SortedPasses);
                    }
                }

                ExecutePasses();
            }
        }
        catch (std::exception& e)
        {
            LOG_ERROR("error: %s", e.what());
        }

        // TODO check resource leaks

        // Clean up
        m_Passes.clear();
        m_SortedPasses.clear();
        m_ResourceDataMap.clear();
    }

    const RenderGraphPass& RenderGraph::GetPass(int32_t index) const
    {
        if (index < 0 || index >= static_cast<int32_t>(m_Passes.size()))
        {
            throw std::out_of_range("Pass index out of range");
        }

        return m_Passes[index];
    }

    int32_t RenderGraph::GetPassCount() const
    {
        return static_cast<int32_t>(m_Passes.size());
    }

    bool RenderGraph::CompilePasses()
    {
        return CullAndSortPasses() && RecordResourceLifeTime();
    }

    void RenderGraph::ExecutePasses()
    {
        RenderGraphContext context{};

        for (int32_t passIndex : m_SortedPasses)
        {
            context.ClearPassData();

            RenderGraphPass& pass = m_Passes[passIndex];

            if (!RentOrReturnResources(pass.ResourcesBorn, false))
            {
                return;
            }

            context.GetCommandContext()->BeginEvent(pass.Name);
            {
                SetPassRenderTargets(context.GetCommandContext(), pass);
                context.GetCommandContext()->SetWireframe(pass.Wireframe);

                if (pass.RenderFunc)
                {
                    pass.RenderFunc(context);
                }
            }
            context.GetCommandContext()->EndEvent();

            if (!RentOrReturnResources(pass.ResourcesDead, true))
            {
                return;
            }
        }
    }

    bool RenderGraph::CullAndSortPasses()
    {
        // 资源是从零入度 pass 开始向后传递的，所以从零入度 pass 开始做 dfs 拓扑排序，尽可能减少资源的生命周期

        // 之后要对结果反转，所以倒着遍历，保证最后排序是 stable 的
        for (int32_t i = static_cast<int32_t>(m_Passes.size() - 1); i >= 0; i--)
        {
            RenderGraphPass& pass = m_Passes[i];

            if (pass.ResourcesRead.empty() && pass.SortState == RenderGraphPassSortState::None)
            {
                if (!CullAndSortPassesDFS(i))
                {
                    return false;
                }
            }
        }

        std::reverse(m_SortedPasses.begin(), m_SortedPasses.end());
        return true;
    }

    bool RenderGraph::CullAndSortPassesDFS(int32_t passIndex)
    {
        RenderGraphPass& pass = m_Passes[passIndex];
        pass.SortState = RenderGraphPassSortState::Visiting;
        int32_t outdegree = 0;

        // 之后要对结果反转，所以倒着遍历，保证最后排序是 stable 的
        for (int32_t i = static_cast<int32_t>(pass.NextPasses.size() - 1); i >= 0; i--)
        {
            int32_t adjIndex = pass.NextPasses[i];
            RenderGraphPass& adj = m_Passes[adjIndex];

            if (adj.SortState == RenderGraphPassSortState::Visiting)
            {
                LOG_ERROR("Cycle detected in render graph");
                return false;
            }

            if (adj.SortState == RenderGraphPassSortState::None)
            {
                if (!CullAndSortPassesDFS(adjIndex))
                {
                    return false;
                }
            }

            if (adj.SortState != RenderGraphPassSortState::Culled)
            {
                outdegree++;
            }
        }

        if (outdegree < 0 || outdegree > static_cast<int32_t>(pass.NextPasses.size()))
        {
            LOG_ERROR("Invalid outdegree %d for pass %s", outdegree, pass.Name.c_str());
            return false;
        }

        if (outdegree == 0 && !pass.HasSideEffects && pass.AllowPassCulling)
        {
            pass.SortState = RenderGraphPassSortState::Culled;
            return true;
        }
        else
        {
            pass.SortState = RenderGraphPassSortState::Visited;
            m_SortedPasses.push_back(passIndex);
            return true;
        }
    }

    bool RenderGraph::RecordResourceLifeTime()
    {
        for (int32_t i = 0; i < m_SortedPasses.size(); i++)
        {
            RenderGraphPass& pass = m_Passes[m_SortedPasses[i]];

            for (int32_t id : pass.ResourcesRead)
            {
                if (!UpdateResourceLifeTime(i, id))
                {
                    return false;
                }
            }

            for (int32_t id : pass.ResourcesWritten)
            {
                if (!UpdateResourceLifeTime(i, id))
                {
                    return false;
                }
            }

            for (uint32_t j = 0; j < pass.NumColorTargets; j++)
            {
                if (!pass.ColorTargets[j].IsSet)
                {
                    LOG_ERROR("Color target %d is not set", i);
                    continue;
                }

                if (!UpdateResourceLifeTime(i, pass.ColorTargets[j].Id))
                {
                    return false;
                }
            }

            if (pass.DepthStencilTarget.IsSet)
            {
                if (!UpdateResourceLifeTime(i, pass.DepthStencilTarget.Id))
                {
                    return false;
                }
            }
        }

        for (auto& kv : m_ResourceDataMap)
        {
            int32_t resourceId = kv.first;
            RenderGraphResourceData& data = kv.second;

            if (data.IsTransient())
            {
                m_Passes[m_SortedPasses[data.GetTransientLifeTimeMinIndex()]].ResourcesBorn.push_back(resourceId);
                m_Passes[m_SortedPasses[data.GetTransientLifeTimeMaxIndex()]].ResourcesDead.push_back(resourceId);
            }
        }

        return true;
    }

    bool RenderGraph::UpdateResourceLifeTime(int32_t sortedPassIndex, int32_t resourceId)
    {
        if (auto it = m_ResourceDataMap.find(resourceId); it == m_ResourceDataMap.end())
        {
            LOG_ERROR("Failed to find resource data for resource %d", resourceId);
            return false;
        }
        else
        {
            it->second.UpdateTransientLifeTime(sortedPassIndex);
            return true;
        }
    }

    bool RenderGraph::RentOrReturnResources(const std::vector<int32_t>& resourceIds, bool isReturn)
    {
        for (int32_t id : resourceIds)
        {
            if (auto it = m_ResourceDataMap.find(id); it == m_ResourceDataMap.end())
            {
                LOG_ERROR("Failed to find resource data for resource %d", id);
                return false;
            }
            else
            {
                RenderGraphResourceData& data = it->second;

                if (isReturn)
                {
                    data.ReturnTransientResource();
                }
                else
                {
                    data.RentTransientResource();
                }
            }
        }

        return true;
    }

    void RenderGraph::SetPassRenderTargets(GfxCommandContext* context, RenderGraphPass& pass)
    {
        GfxRenderTexture* colorTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
        GfxRenderTexture* depthStencilTarget = nullptr;

        for (uint32_t i = 0; i < pass.NumColorTargets; i++)
        {
            if (!pass.ColorTargets[i].IsSet)
            {
                LOG_ERROR("Color target %d is not set", i);
                continue;
            }

            colorTargets[i] = GetResourceData(pass.ColorTargets[i].Id).GetTexture();
        }

        if (pass.DepthStencilTarget.IsSet)
        {
            depthStencilTarget = GetResourceData(pass.DepthStencilTarget.Id).GetTexture();
        }

        context->SetRenderTargets(pass.NumColorTargets, colorTargets, depthStencilTarget);

        if (pass.HasCustomViewport)
        {
            context->SetViewport(pass.CustomViewport);
        }
        else
        {
            context->SetDefaultViewport();
        }

        if (pass.HasCustomScissorRect)
        {
            context->SetScissorRect(pass.CustomScissorRect);
        }
        else
        {
            context->SetDefaultScissorRect();
        }

        context->ClearRenderTargets(pass.RenderTargetsClearFlags, pass.ClearColorValue, pass.ClearDepthValue, pass.ClearStencilValue);
    }

    RenderGraphResourceData& RenderGraph::GetResourceData(int32_t id)
    {
        if (auto it = m_ResourceDataMap.find(id); it == m_ResourceDataMap.end())
        {
            throw std::runtime_error("Resource data not found");
        }
        else
        {
            return it->second;
        }
    }

    std::unordered_set<IRenderGraphCompiledEventListener*> RenderGraph::s_GraphCompiledEventListeners{};

    void RenderGraph::AddGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener)
    {
        s_GraphCompiledEventListeners.insert(listener);
    }

    void RenderGraph::RemoveGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener)
    {
        s_GraphCompiledEventListeners.erase(listener);
    }

    RenderGraphBuilder::RenderGraphBuilder(RenderGraph* graph, int32_t passIndex)
        : m_Graph(graph)
        , m_PassIndex(passIndex)
    {
    }

    void RenderGraphBuilder::AllowPassCulling(bool value)
    {
        GetPass().AllowPassCulling = value;
    }

    void RenderGraphBuilder::ImportTexture(int32_t id, GfxRenderTexture* texture)
    {
        std::unordered_map<int32_t, RenderGraphResourceData>& resMap = m_Graph->m_ResourceDataMap;

        if (resMap.find(id) != resMap.end())
        {
            LOG_ERROR("Resource %d already exists", id);
            return;
        }

        resMap.emplace(std::move(id), RenderGraphResourceData(texture));
    }

    void RenderGraphBuilder::CreateTransientTexture(int32_t id, const GfxTextureDesc& desc)
    {
        std::unordered_map<int32_t, RenderGraphResourceData>& resMap = m_Graph->m_ResourceDataMap;

        if (resMap.find(id) != resMap.end())
        {
            LOG_ERROR("Resource %d already exists", id);
            return;
        }

        resMap.emplace(std::move(id), RenderGraphResourceData(m_Graph->m_ResourcePool.get(), desc));
    }

    const GfxTextureDesc& RenderGraphBuilder::GetTextureDesc(int32_t id) const
    {
        if (auto it = m_Graph->m_ResourceDataMap.find(id); it == m_Graph->m_ResourceDataMap.end())
        {
            throw std::runtime_error("Resource data not found");
        }
        else
        {
            return it->second.GetTextureDesc();
        }
    }

    TextureHandle RenderGraphBuilder::ReadTexture(int32_t id)
    {
        RenderGraphPass& pass = GetPass();

        if (pass.ResourcesRead.count(id) > 0)
        {
            throw std::runtime_error("Resource already read");
        }
        else
        {
            if (pass.ResourcesWritten.count(id) > 0)
            {
                LOG_ERROR("Resource %d is both read and written in pass %s", id, pass.Name.c_str());
                return TextureHandle(m_Graph, -1);
            }

            auto resIt = m_Graph->m_ResourceDataMap.find(id);
            if (resIt == m_Graph->m_ResourceDataMap.end())
            {
                throw std::runtime_error("Resource data not found");
            }

            int32_t producerPassIndex = resIt->second.GetLastProducerPass();
            if (producerPassIndex < 0)
            {
                LOG_ERROR("Failed to find producer pass for resource %d in pass %s", id, pass.Name.c_str());
                return TextureHandle(m_Graph, -1);
            }

            pass.ResourcesRead.emplace(id);
            m_Graph->m_Passes[producerPassIndex].NextPasses.push_back(m_PassIndex);
        }

        return TextureHandle(m_Graph, id);
    }

    TextureHandle RenderGraphBuilder::WriteTexture(int32_t id)
    {
        RenderGraphPass& pass = GetPass();

        if (pass.ResourcesWritten.count(id) > 0)
        {
            throw std::runtime_error("Resource already written");
        }
        else
        {
            if (pass.ResourcesRead.count(id) > 0)
            {
                LOG_ERROR("Resource %d is both read and written in pass %s", id, pass.Name.c_str());
                return TextureHandle(m_Graph, -1);
            }

            auto resIt = m_Graph->m_ResourceDataMap.find(id);
            if (resIt == m_Graph->m_ResourceDataMap.end())
            {
                throw std::runtime_error("Resource data not found");
            }

            pass.HasSideEffects |= !resIt->second.IsTransient();
            pass.ResourcesWritten.emplace(id);
            resIt->second.AddProducerPass(m_PassIndex);
        }

        return TextureHandle(m_Graph, id);
    }

    void RenderGraphBuilder::SetColorTarget(int32_t id, bool load)
    {
        SetColorTarget(id, 0, load);
    }

    void RenderGraphBuilder::SetColorTarget(int32_t id, uint32_t index, bool load)
    {
        RenderGraphPass& pass = GetPass();

        if (index < 0)
        {
            LOG_ERROR("Invalid color target index");
            return;
        }

        if (index >= std::size(pass.ColorTargets))
        {
            LOG_ERROR("Too many color targets");
            return;
        }

        pass.NumColorTargets = std::max(pass.NumColorTargets, index + 1u);
        RenderTargetData& data = pass.ColorTargets[index];

        if (data.IsSet)
        {
            throw std::runtime_error("Color target already set");
        }

        data.Id = id;
        data.IsSet = true;
        data.Load = load;

        auto resIt = m_Graph->m_ResourceDataMap.find(id);
        if (resIt == m_Graph->m_ResourceDataMap.end())
        {
            throw std::runtime_error("Resource data not found");
        }

        if (load)
        {
            // render target 可以没有 producer
            if (int32_t producerPassIndex = resIt->second.GetLastProducerPass(); producerPassIndex >= 0)
            {
                m_Graph->m_Passes[producerPassIndex].NextPasses.push_back(m_PassIndex);
            }
        }

        pass.HasSideEffects |= !resIt->second.IsTransient();
        resIt->second.AddProducerPass(m_PassIndex);
    }

    void RenderGraphBuilder::SetDepthStencilTarget(int32_t id, bool load)
    {
        RenderGraphPass& pass = GetPass();
        RenderTargetData& data = pass.DepthStencilTarget;

        if (data.IsSet)
        {
            throw std::runtime_error("Depth stencil target already set");
        }

        data.Id = id;
        data.IsSet = true;
        data.Load = load;

        auto resIt = m_Graph->m_ResourceDataMap.find(id);
        if (resIt == m_Graph->m_ResourceDataMap.end())
        {
            throw std::runtime_error("Resource data not found");
        }

        if (load)
        {
            // render target 可以没有 producer
            if (int32_t producerPassIndex = resIt->second.GetLastProducerPass(); producerPassIndex >= 0)
            {
                m_Graph->m_Passes[producerPassIndex].NextPasses.push_back(m_PassIndex);
            }
        }

        pass.HasSideEffects |= !resIt->second.IsTransient();
        resIt->second.AddProducerPass(m_PassIndex);
    }

    void RenderGraphBuilder::ClearRenderTargets(GfxClearFlags flags, const float color[4], float depth, uint8_t stencil)
    {
        RenderGraphPass& pass = GetPass();

        pass.RenderTargetsClearFlags = flags;
        std::copy_n(color, std::size(pass.ClearColorValue), pass.ClearColorValue);
        pass.ClearDepthValue = depth;
        pass.ClearStencilValue = stencil;
    }

    void RenderGraphBuilder::SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
    {
        RenderGraphPass& pass = GetPass();

        pass.HasCustomViewport = true;
        pass.CustomViewport.TopLeftX = topLeftX;
        pass.CustomViewport.TopLeftY = topLeftY;
        pass.CustomViewport.Width = width;
        pass.CustomViewport.Height = height;
        pass.CustomViewport.MinDepth = minDepth;
        pass.CustomViewport.MaxDepth = maxDepth;
    }

    void RenderGraphBuilder::SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
    {
        RenderGraphPass& pass = GetPass();

        pass.HasCustomScissorRect = true;
        pass.CustomScissorRect.left = static_cast<LONG>(left);
        pass.CustomScissorRect.top = static_cast<LONG>(top);
        pass.CustomScissorRect.right = static_cast<LONG>(right);
        pass.CustomScissorRect.bottom = static_cast<LONG>(bottom);
    }

    void RenderGraphBuilder::SetWireframe(bool value)
    {
        RenderGraphPass& pass = GetPass();
        pass.Wireframe = value;
    }

    void RenderGraphBuilder::SetRenderFunc(const std::function<void(RenderGraphContext&)>& func)
    {
        GetPass().RenderFunc = func;
    }

    RenderGraphPass& RenderGraphBuilder::GetPass()
    {
        return m_Graph->m_Passes[m_PassIndex];
    }
}
