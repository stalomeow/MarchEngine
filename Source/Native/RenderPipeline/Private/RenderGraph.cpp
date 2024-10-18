#include "RenderGraph.h"
#include "GfxDevice.h"
#include "GfxCommandList.h"
#include "GfxResource.h"
#include "Debug.h"
#include <utility>
#include <stdexcept>

namespace march
{
    RenderTargetLoadFlags operator|(RenderTargetLoadFlags lhs, RenderTargetLoadFlags rhs)
    {
        return static_cast<RenderTargetLoadFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    RenderTargetLoadFlags& operator|=(RenderTargetLoadFlags& lhs, RenderTargetLoadFlags rhs)
    {
        return lhs = lhs | rhs;
    }

    RenderTargetLoadFlags operator&(RenderTargetLoadFlags lhs, RenderTargetLoadFlags rhs)
    {
        return static_cast<RenderTargetLoadFlags>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }

    RenderTargetLoadFlags& operator&=(RenderTargetLoadFlags& lhs, RenderTargetLoadFlags rhs)
    {
        return lhs = lhs & rhs;
    }

    RenderGraphPass::RenderGraphPass(const std::string& name)
        : Name(name)
        , AllowPassCulling(true)
        , ResourcesRead{}
        , ResourcesWritten{}
        , HasRenderTargets(false)
        , NumColorTargets(0)
        , ColorTargets{}
        , HasDepthStencilTarget(false)
        , DepthStencilTarget(0)
        , RenderTargetsLoadFlags(RenderTargetLoadFlags::None)
        , RenderTargetsClearFlags(ClearFlags::None)
        , ClearColorValue{}
        , ClearDepthValue(0)
        , ClearStencilValue(0)
        , HasCustomViewport(false)
        , CustomViewport{}
        , HasCustomScissorRect(false)
        , CustomScissorRect{}
        , SortState(RenderGraphPassSortState::None)
        , NextPasses{}
        , ResourcesBorn{}
        , ResourcesDead{}
        , RenderFunc{}
    {
    }

    RenderGraph::RenderGraph()
        : m_Passes{}
        , m_SortedPasses{}
        , m_ResourceDataMap{}
    {
        m_ResourcePool = std::make_unique<RenderGraphResourcePool>();
        m_Context = std::make_unique<RenderGraphContext>();
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
                for (IRenderGraphCompiledEventListener* listener : s_GraphCompiledEventListeners)
                {
                    listener->OnGraphCompiled(*this, m_SortedPasses);
                }

                ExecutePasses();
            }
        }
        catch (std::exception& e)
        {
            DEBUG_LOG_ERROR("error: %s", e.what());
        }

        // TODO check resource leaks

        // Clean up
        m_Passes.clear();
        m_SortedPasses.clear();
        m_ResourceDataMap.clear();
        m_Context->Reset();
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
        RenderGraphContext& context = *m_Context.get();

        for (int32_t passIndex : m_SortedPasses)
        {
            RenderGraphPass& pass = m_Passes[passIndex];

            if (!RentOrReturnResources(pass.ResourcesBorn, false))
            {
                return;
            }

            AddPassResourceBarriers(pass);
            SetPassRenderTargets(pass);

            if (pass.RenderFunc)
            {
                pass.RenderFunc(context);
            }

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
                DEBUG_LOG_ERROR("Cycle detected in render graph");
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
            DEBUG_LOG_ERROR("Invalid outdegree %d for pass %s", outdegree, pass.Name.c_str());
            return false;
        }

        if (outdegree == 0 && pass.AllowPassCulling)
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

            for (auto& kv : pass.ResourcesRead)
            {
                if (!UpdateResourceLifeTime(i, kv.first))
                {
                    return false;
                }
            }

            for (auto& kv : pass.ResourcesWritten)
            {
                if (!UpdateResourceLifeTime(i, kv.first))
                {
                    return false;
                }
            }

            if (pass.HasRenderTargets)
            {
                for (size_t j = 0; j < pass.NumColorTargets; j++)
                {
                    if (!UpdateResourceLifeTime(i, pass.ColorTargets[j]))
                    {
                        return false;
                    }
                }

                if (pass.HasDepthStencilTarget)
                {
                    if (!UpdateResourceLifeTime(i, pass.DepthStencilTarget))
                    {
                        return false;
                    }
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
            DEBUG_LOG_ERROR("Failed to find resource data for resource %d", resourceId);
            return false;
        }
        else
        {
            it->second.UpdateTransientLifeTime(sortedPassIndex);
            return true;
        }
    }

    bool RenderGraph::RentOrReturnResources(std::vector<int32_t> resourceIds, bool isReturn)
    {
        for (int32_t id : resourceIds)
        {
            if (auto it = m_ResourceDataMap.find(id); it == m_ResourceDataMap.end())
            {
                DEBUG_LOG_ERROR("Failed to find resource data for resource %d", id);
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

    void RenderGraph::SetPassRenderTargets(RenderGraphPass& pass)
    {
        if (!pass.HasRenderTargets)
        {
            return;
        }

        GfxRenderTexture* colorTargets[8]{};
        GfxRenderTexture* depthStencilTarget = nullptr;

        for (size_t i = 0; i < pass.NumColorTargets; i++)
        {
            colorTargets[i] = static_cast<GfxRenderTexture*>(GetResourceData(pass.ColorTargets[i]).GetResourcePtr());
        }

        if (pass.HasDepthStencilTarget)
        {
            depthStencilTarget = static_cast<GfxRenderTexture*>(GetResourceData(pass.DepthStencilTarget).GetResourcePtr());
        }

        m_Context->SetRenderTargets(pass.NumColorTargets, colorTargets, depthStencilTarget,
            pass.HasCustomViewport ? &pass.CustomViewport : nullptr,
            pass.HasCustomScissorRect ? &pass.CustomScissorRect : nullptr);
        m_Context->ClearRenderTargets(pass.RenderTargetsClearFlags, pass.ClearColorValue, pass.ClearDepthValue, pass.ClearStencilValue);
    }

    void RenderGraph::AddPassResourceBarriers(RenderGraphPass& pass)
    {
        GfxCommandList* cmdList = GetGfxDevice()->GetGraphicsCommandList();

        for (const std::pair<int32_t, RenderGraphResourceReadFlags>& kv : pass.ResourcesRead)
        {
            RenderGraphResourceData& res = GetResourceData(kv.first);
            cmdList->ResourceBarrier(res.GetResourcePtr(), GetResourceReadState(res, kv.second));
        }

        for (const std::pair<int32_t, RenderGraphResourceWriteFlags>& kv : pass.ResourcesWritten)
        {
            RenderGraphResourceData& res = GetResourceData(kv.first);
            cmdList->ResourceBarrier(res.GetResourcePtr(), GetResourceWriteState(res, kv.second));
        }

        if (pass.HasRenderTargets)
        {
            for (size_t i = 0; i < pass.NumColorTargets; i++)
            {
                RenderGraphResourceData& res = GetResourceData(pass.ColorTargets[i]);
                cmdList->ResourceBarrier(res.GetResourcePtr(), D3D12_RESOURCE_STATE_RENDER_TARGET);
            }

            if (pass.HasDepthStencilTarget)
            {
                RenderGraphResourceData& res = GetResourceData(pass.DepthStencilTarget);
                cmdList->ResourceBarrier(res.GetResourcePtr(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
            }
        }

        cmdList->FlushResourceBarriers();
    }

    D3D12_RESOURCE_STATES RenderGraph::GetResourceReadState(RenderGraphResourceData& res, RenderGraphResourceReadFlags flags)
    {
        D3D12_RESOURCE_STATES result{};

        if (res.GetResourceType() == RenderGraphResourceType::Texture)
        {
            if ((flags & RenderGraphResourceReadFlags::Copy) == RenderGraphResourceReadFlags::Copy)
            {
                result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
            }

            if ((flags & RenderGraphResourceReadFlags::Resolve) == RenderGraphResourceReadFlags::Resolve)
            {
                result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
            }

            if ((flags & RenderGraphResourceReadFlags::PixelShader) == RenderGraphResourceReadFlags::PixelShader)
            {
                result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            }

            if ((flags & RenderGraphResourceReadFlags::NonPixelShader) == RenderGraphResourceReadFlags::NonPixelShader)
            {
                result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }
        }
        else
        {
            DEBUG_LOG_ERROR("Unsupported resource type");
        }

        return result;
    }

    D3D12_RESOURCE_STATES RenderGraph::GetResourceWriteState(RenderGraphResourceData& res, RenderGraphResourceWriteFlags flags)
    {
        D3D12_RESOURCE_STATES result{};

        if (res.GetResourceType() == RenderGraphResourceType::Texture)
        {
            if ((flags & RenderGraphResourceWriteFlags::Copy) == RenderGraphResourceWriteFlags::Copy)
            {
                result |= D3D12_RESOURCE_STATE_COPY_DEST;
            }

            if ((flags & RenderGraphResourceWriteFlags::Resolve) == RenderGraphResourceWriteFlags::Resolve)
            {
                result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
            }
        }
        else
        {
            DEBUG_LOG_ERROR("Unsupported resource type");
        }

        return result;
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

    RenderGraphTextureHandle::RenderGraphTextureHandle(RenderGraph* graph, int32_t resourceId) : m_Graph(graph), m_ResourceId(resourceId)
    {
    }

    GfxRenderTextureDesc RenderGraphTextureHandle::GetDesc() const
    {
        RenderGraphResourceData& data = m_Graph->GetResourceData(m_ResourceId);
        return data.GetTextureDesc();
    }

    GfxRenderTexture* RenderGraphTextureHandle::Get()  const
    {
        RenderGraphResourceData& data = m_Graph->GetResourceData(m_ResourceId);

        if (data.GetResourceType() != RenderGraphResourceType::Texture)
        {
            return nullptr;
        }

        return static_cast<GfxRenderTexture*>(data.GetResourcePtr());
    }

    GfxRenderTexture* RenderGraphTextureHandle::operator->()  const
    {
        return Get();
    }

    RenderGraphBuilder::RenderGraphBuilder(RenderGraph* graph, int32_t passIndex) : m_Graph(graph), m_PassIndex(passIndex)
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
            DEBUG_LOG_ERROR("Resource %d already exists", id);
            return;
        }

        resMap.emplace(std::move(id), RenderGraphResourceData(texture));
    }

    void RenderGraphBuilder::CreateTransientTexture(int32_t id, const GfxRenderTextureDesc& desc)
    {
        std::unordered_map<int32_t, RenderGraphResourceData>& resMap = m_Graph->m_ResourceDataMap;

        if (resMap.find(id) != resMap.end())
        {
            DEBUG_LOG_ERROR("Resource %d already exists", id);
            return;
        }

        resMap.emplace(std::move(id), RenderGraphResourceData(m_Graph->m_ResourcePool.get(), desc));
    }

    TextureHandle RenderGraphBuilder::ReadTexture(int32_t id, ReadFlags flags)
    {
        RenderGraphPass& pass = GetPass();

        if (auto flagIt = pass.ResourcesRead.find(id); flagIt != pass.ResourcesRead.end())
        {
            throw std::runtime_error("Resource already read");
        }
        else if (flags != ReadFlags::None)
        {
            if (pass.ResourcesWritten.count(id) > 0)
            {
                DEBUG_LOG_ERROR("Resource %d is both read and written in pass %s", id, pass.Name.c_str());
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
                DEBUG_LOG_ERROR("Failed to find producer pass for resource %d in pass %s", id, pass.Name.c_str());
                return TextureHandle(m_Graph, -1);
            }

            pass.ResourcesRead.emplace(id, flags);
            m_Graph->m_Passes[producerPassIndex].NextPasses.push_back(m_PassIndex);
        }

        return TextureHandle(m_Graph, id);
    }

    TextureHandle RenderGraphBuilder::WriteTexture(int32_t id, WriteFlags flags)
    {
        RenderGraphPass& pass = GetPass();

        if (auto flagIt = pass.ResourcesWritten.find(id); flagIt != pass.ResourcesWritten.end())
        {
            throw std::runtime_error("Resource already written");
        }
        else if (flags != WriteFlags::None)
        {
            if (pass.ResourcesRead.count(id) > 0)
            {
                DEBUG_LOG_ERROR("Resource %d is both read and written in pass %s", id, pass.Name.c_str());
                return TextureHandle(m_Graph, -1);
            }

            auto resIt = m_Graph->m_ResourceDataMap.find(id);
            if (resIt == m_Graph->m_ResourceDataMap.end())
            {
                throw std::runtime_error("Resource data not found");
            }

            pass.ResourcesWritten.emplace(id, flags);
            resIt->second.AddProducerPass(m_PassIndex);
        }

        return TextureHandle(m_Graph, id);
    }

    void RenderGraphBuilder::SetRenderTargets(int32_t colorTarget, LoadFlags flags)
    {
        RenderGraphPass& pass = GetPass();

        if (pass.HasRenderTargets)
        {
            throw std::runtime_error("Render targets already set");
        }

        pass.HasRenderTargets = true;
        pass.NumColorTargets = 1;
        pass.ColorTargets[0] = colorTarget;
        pass.HasDepthStencilTarget = false;
        pass.RenderTargetsLoadFlags = flags;

        PostSetRenderTargets();
    }

    void RenderGraphBuilder::SetRenderTargets(int32_t colorTarget, int32_t depthStencilTarget, LoadFlags flags)
    {
        RenderGraphPass& pass = GetPass();

        if (pass.HasRenderTargets)
        {
            throw std::runtime_error("Render targets already set");
        }

        pass.HasRenderTargets = true;
        pass.NumColorTargets = 1;
        pass.ColorTargets[0] = colorTarget;
        pass.HasDepthStencilTarget = true;
        pass.DepthStencilTarget = depthStencilTarget;
        pass.RenderTargetsLoadFlags = flags;

        PostSetRenderTargets();
    }

    void RenderGraphBuilder::SetRenderTargets(size_t numColorTargets, const int32_t* colorTargets, LoadFlags flags)
    {
        RenderGraphPass& pass = GetPass();

        if (pass.HasRenderTargets)
        {
            throw std::runtime_error("Render targets already set");
        }

        if (numColorTargets > std::size(pass.ColorTargets))
        {
            DEBUG_LOG_ERROR("Too many color targets");
            return;
        }

        pass.HasRenderTargets = true;
        pass.NumColorTargets = numColorTargets;

        if (numColorTargets > 0)
        {
            std::copy_n(colorTargets, numColorTargets, pass.ColorTargets);
        }

        pass.HasDepthStencilTarget = false;
        pass.RenderTargetsLoadFlags = flags;

        PostSetRenderTargets();
    }

    void RenderGraphBuilder::SetRenderTargets(size_t numColorTargets, const int32_t* colorTargets, int32_t depthStencilTarget, LoadFlags flags)
    {
        RenderGraphPass& pass = GetPass();

        if (pass.HasRenderTargets)
        {
            throw std::runtime_error("Render targets already set");
        }

        if (numColorTargets > std::size(pass.ColorTargets))
        {
            DEBUG_LOG_ERROR("Too many color targets");
            return;
        }

        pass.HasRenderTargets = true;
        pass.NumColorTargets = numColorTargets;

        if (numColorTargets > 0)
        {
            std::copy_n(colorTargets, numColorTargets, pass.ColorTargets);
        }

        pass.HasDepthStencilTarget = true;
        pass.DepthStencilTarget = depthStencilTarget;
        pass.RenderTargetsLoadFlags = flags;

        PostSetRenderTargets();
    }

    void RenderGraphBuilder::PostSetRenderTargets()
    {
        RenderGraphPass& pass = GetPass();

        if (!pass.HasRenderTargets)
        {
            return;
        }

        // Load Colors
        if ((pass.RenderTargetsLoadFlags & LoadFlags::DiscardColors) == LoadFlags::None)
        {
            for (size_t i = 0; i < pass.NumColorTargets; i++)
            {
                auto it = m_Graph->m_ResourceDataMap.find(pass.ColorTargets[i]);
                if (it == m_Graph->m_ResourceDataMap.end())
                {
                    throw std::runtime_error("Resource data not found");
                }

                // render target 可以没有 producer
                if (int32_t producerPassIndex = it->second.GetLastProducerPass(); producerPassIndex >= 0)
                {
                    m_Graph->m_Passes[producerPassIndex].NextPasses.push_back(m_PassIndex);
                }

                it->second.AddProducerPass(m_PassIndex);
            }
        }

        // Load Depth Stencil
        if (pass.HasDepthStencilTarget && (pass.RenderTargetsLoadFlags & LoadFlags::DiscardDepthStencil) == LoadFlags::None)
        {
            auto it = m_Graph->m_ResourceDataMap.find(pass.DepthStencilTarget);
            if (it == m_Graph->m_ResourceDataMap.end())
            {
                throw std::runtime_error("Resource data not found");
            }

            // render target 可以没有 producer
            if (int32_t producerPassIndex = it->second.GetLastProducerPass(); producerPassIndex >= 0)
            {
                m_Graph->m_Passes[producerPassIndex].NextPasses.push_back(m_PassIndex);
            }

            it->second.AddProducerPass(m_PassIndex);
        }
    }

    void RenderGraphBuilder::ClearRenderTargets(ClearFlags flags, const float color[4], float depth, uint8_t stencil)
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

    void RenderGraphBuilder::SetRenderFunc(std::function<void(RenderGraphContext&)> func)
    {
        GetPass().RenderFunc = std::move(func);
    }

    RenderGraphPass& RenderGraphBuilder::GetPass()
    {
        return m_Graph->m_Passes[m_PassIndex];
    }
}
