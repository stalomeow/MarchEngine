#include "pch.h"
#include "Engine/Rendering/RenderGraphImpl/RenderGraphCore.h"
#include "Engine/Debug.h"
#include <utility>
#include <assert.h>

namespace march
{
    RenderGraphPass& RenderGraphBuilder::GetPass() const
    {
        return m_Graph->m_Passes[m_PassIndex];
    }

    void RenderGraphBuilder::InResource(
        RenderGraphResourceManager* resourceManager,
        size_t resourceIndex,
        const std::optional<RenderGraphResourceVariableDesc>& variableDesc)
    {
        RenderGraphPass& pass = GetPass();

        if (pass.ResourcesIn.insert(resourceIndex).second)
        {
            std::optional<size_t> producerPassIndex = resourceManager->GetLastProducerBeforePassIndex(resourceIndex, m_PassIndex);

            if (producerPassIndex)
            {
                m_Graph->m_Passes[*producerPassIndex].NextPassIndices.push_back(m_PassIndex);
            }
            else
            {
                int32 id = resourceManager->GetResourceId(resourceIndex);
                LOG_WARNING("Failed to find producer pass for resource '{}' in pass '{}'", ShaderUtils::GetStringFromId(id), pass.Name);
            }
        }

        if (variableDesc)
        {
            SetResourceVariable(resourceIndex, variableDesc.value());
        }
    }

    void RenderGraphBuilder::OutResource(
        RenderGraphResourceManager* resourceManager,
        size_t resourceIndex,
        const std::optional<RenderGraphResourceVariableDesc>& variableDesc)
    {
        RenderGraphPass& pass = GetPass();

        if (pass.ResourcesOut.insert(resourceIndex).second)
        {
            pass.HasSideEffects |= resourceManager->IsResourceExternal(resourceIndex);
            resourceManager->AddProducerPassIndex(resourceIndex, m_PassIndex);
        }

        if (variableDesc)
        {
            SetResourceVariable(resourceIndex, variableDesc.value());
        }
    }

    void RenderGraphBuilder::SetResourceVariable(size_t resourceIndex, const RenderGraphResourceVariableDesc& variableDesc)
    {
        RenderGraphPass& pass = GetPass();
        RenderGraphPassVariable newVariable = { resourceIndex, variableDesc };
        int32 aliasId = newVariable.Desc.Id;

        if (auto result = pass.Variables.emplace(aliasId, newVariable); !result.second)
        {
            const RenderGraphPassVariable& oldVariable = result.first->second;

            if (memcmp(&oldVariable, &newVariable, sizeof(newVariable)) != 0)
            {
                LOG_ERROR("Variable '{}' is already set with different configuration in pass '{}'", ShaderUtils::GetStringFromId(aliasId), pass.Name);
            }
        }
    }

    void RenderGraphBuilder::AllowPassCulling(bool value)
    {
        GetPass().AllowPassCulling = value;
    }

    void RenderGraphBuilder::EnableAsyncCompute(bool value)
    {
        GetPass().EnableAsyncCompute = value;
    }

    void RenderGraphBuilder::In(const BufferElementHandle& buffer)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(buffer.Buffer);

        RenderGraphResourceVariableDesc varDesc{};
        varDesc.Id = buffer.AliasId;
        varDesc.BufferElement = buffer.Element;

        InResource(resourceManager, resourceIndex, varDesc);
    }

    void RenderGraphBuilder::Out(const BufferElementHandle& buffer)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(buffer.Buffer);

        RenderGraphResourceVariableDesc varDesc{};
        varDesc.Id = buffer.AliasId;
        varDesc.BufferElement = buffer.Element;

        OutResource(resourceManager, resourceIndex, varDesc);
    }

    void RenderGraphBuilder::InOut(const BufferElementHandle& buffer)
    {
        In(buffer);
        Out(buffer);
    }

    void RenderGraphBuilder::In(const TextureElementHandle& texture)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(texture.Texture);

        RenderGraphResourceVariableDesc varDesc{};
        varDesc.Id = texture.AliasId;
        varDesc.TextureElement = texture.Element;
        varDesc.TextureUnorderedAccessMipSlice = texture.UnorderedAccessMipSlice;

        InResource(resourceManager, resourceIndex, varDesc);
    }

    void RenderGraphBuilder::Out(const TextureElementHandle& texture)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(texture.Texture);

        RenderGraphResourceVariableDesc varDesc{};
        varDesc.Id = texture.AliasId;
        varDesc.TextureElement = texture.Element;
        varDesc.TextureUnorderedAccessMipSlice = texture.UnorderedAccessMipSlice;

        OutResource(resourceManager, resourceIndex, varDesc);
    }

    void RenderGraphBuilder::InOut(const TextureElementHandle& texture)
    {
        In(texture);
        Out(texture);
    }

    void RenderGraphBuilder::SetColorTarget(const TextureHandle& texture, RenderTargetInitMode initMode, const float color[4])
    {
        SetColorTarget(texture, 0, initMode, color);
    }

    void RenderGraphBuilder::SetColorTarget(const TextureHandle& texture, uint32_t index, RenderTargetInitMode initMode, const float color[4])
    {
        RenderGraphPass& pass = GetPass();

        if (index >= std::size(pass.ColorTargets))
        {
            LOG_ERROR("Color target index '{}' out of range in pass '{}'", index, pass.Name);
            return;
        }

        pass.NumColorTargets = std::max(pass.NumColorTargets, index + 1u);
        RenderGraphPassColorTarget& target = pass.ColorTargets[index];

        if (target.IsSet)
        {
            LOG_ERROR("Can not set color target '{}' multiple times in pass '{}'", index, pass.Name);
            return;
        }

        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        target.ResourceIndex = resourceManager->GetResourceIndex(texture);
        target.IsSet = true;
        target.InitMode = initMode;
        std::copy_n(color, std::size(target.ClearColor), target.ClearColor);

        if (initMode == RenderTargetInitMode::Load)
        {
            // RenderTarget 不设置成 Variable
            InResource(resourceManager, target.ResourceIndex, std::nullopt);
        }

        // RenderTarget 不设置成 Variable
        OutResource(resourceManager, target.ResourceIndex, std::nullopt);
    }

    void RenderGraphBuilder::SetDepthStencilTarget(const TextureHandle& texture, RenderTargetInitMode initMode, float depth, uint8_t stencil)
    {
        RenderGraphPass& pass = GetPass();
        RenderGraphPassDepthStencilTarget& target = pass.DepthStencilTarget;

        if (target.IsSet)
        {
            LOG_ERROR("Can not set depth stencil target multiple times in pass '{}'", pass.Name);
            return;
        }

        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        target.ResourceIndex = resourceManager->GetResourceIndex(texture);
        target.IsSet = true;
        target.InitMode = initMode;
        target.ClearDepthValue = depth;
        target.ClearStencilValue = stencil;

        if (initMode == RenderTargetInitMode::Load)
        {
            // RenderTarget 不设置成 Variable
            InResource(resourceManager, target.ResourceIndex, std::nullopt);
        }

        // RenderTarget 不设置成 Variable
        OutResource(resourceManager, target.ResourceIndex, std::nullopt);
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

    void RenderGraphBuilder::SetDepthBias(int32_t bias, float slopeScaledBias, float clamp)
    {
        RenderGraphPass& pass = GetPass();

        pass.HasCustomDepthBias = true;
        pass.DepthBias = bias;
        pass.DepthBiasClamp = clamp;
        pass.SlopeScaledDepthBias = slopeScaledBias;
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

    void RenderGraph::CompilePasses()
    {
        // 每个 pass 的计算都依赖后面的 pass，所以从后往前遍历

        // 所有 pass 结束后，有一个 m_PassIndexToWaitFallback
        // 可以把它当作一个依赖所有 async compute 的虚拟 pass，所以初始化为 m_Passes.size()
        // 如果后面的 async compute 执行完了，那前面的肯定也执行完了，所以 deadline 的值会逐渐变小
        size_t asyncComputeDeadlineIndexExclusive = m_Passes.size();

        // 注意 size_t 是无符号的，所以不能用 i >= 0
        for (size_t passIndex = m_Passes.size() - 1; passIndex != -1; passIndex--)
        {
            CullPass(passIndex, asyncComputeDeadlineIndexExclusive);
        }

        // 设置资源的生命周期
        for (size_t i = 0; i < m_ResourceManager->GetNumResources(); i++)
        {
            if (std::optional<std::pair<size_t, size_t>> lifetime = m_ResourceManager->GetLifetimePassIndexRange(i))
            {
                std::pair<size_t, size_t> range = *lifetime;
                m_Passes[range.first].ResourcesBorn.push_back(i);
                m_Passes[range.second].ResourcesDead.push_back(i);
            }
        }
    }

    void RenderGraph::CullPass(size_t passIndex, size_t& asyncComputeDeadlineIndexExclusive)
    {
        RenderGraphPass& pass = m_Passes[passIndex];
        assert(!pass.IsVisited);

        size_t outdegree = 0;
        size_t asyncComputeDeadlineIndexExclusiveCopy = asyncComputeDeadlineIndexExclusive;

        for (size_t adjIndex : pass.NextPassIndices)
        {
            RenderGraphPass& adjPass = m_Passes[adjIndex];
            assert(adjPass.IsVisited);

            if (!adjPass.IsCulled)
            {
                outdegree++;

                if (!adjPass.IsAsyncCompute)
                {
                    asyncComputeDeadlineIndexExclusiveCopy = std::min(asyncComputeDeadlineIndexExclusiveCopy, adjIndex);
                }
            }
        }

        if (outdegree == 0 && !pass.HasSideEffects && pass.AllowPassCulling)
        {
            pass.IsCulled = true;
        }
        else
        {
            pass.IsCulled = false;

            CompileAsyncCompute(passIndex, asyncComputeDeadlineIndexExclusiveCopy);

            if (pass.IsAsyncCompute)
            {
                asyncComputeDeadlineIndexExclusive = asyncComputeDeadlineIndexExclusiveCopy;
            }

            for (size_t resourceIndex : pass.ResourcesIn)
            {
                m_ResourceManager->SetAlive(resourceIndex, passIndex);

                // async compute 需要延长资源的生命周期
                if (pass.IsAsyncCompute)
                {
                    m_ResourceManager->SetAlive(resourceIndex, asyncComputeDeadlineIndexExclusive - 1);
                }
            }

            for (size_t resourceIndex : pass.ResourcesOut)
            {
                m_ResourceManager->SetAlive(resourceIndex, passIndex);

                // async compute 需要延长资源的生命周期
                if (pass.IsAsyncCompute)
                {
                    m_ResourceManager->SetAlive(resourceIndex, asyncComputeDeadlineIndexExclusive - 1);
                }
            }
        }

        pass.IsVisited = true;
    }

    void RenderGraph::CompileAsyncCompute(size_t passIndex, size_t& deadlineIndexExclusive)
    {
        RenderGraphPass& pass = m_Passes[passIndex];
        assert(!pass.IsVisited);

        if (!pass.EnableAsyncCompute)
        {
            pass.IsAsyncCompute = false;
            return;
        }

        size_t overlappedPassCount = AvoidAsyncComputeResourceCompetition(passIndex, deadlineIndexExclusive);

        // 有重叠的 pass 时 async compute 才有意义
        if (overlappedPassCount == 0)
        {
            pass.IsAsyncCompute = false;
            return;
        }

        pass.IsAsyncCompute = true;

        // 如果后面的 async compute 执行完了，那前面的肯定也执行完了
        // 因为 compile 时是从后往前遍历的，所以第一次赋给 PassIndexToWait 的值一定是最大的，不用再更新

        if (deadlineIndexExclusive == m_Passes.size())
        {
            if (!m_PassIndexToWaitFallback)
            {
                m_PassIndexToWaitFallback = passIndex;
            }
        }
        else
        {
            RenderGraphPass& deadlinePass = m_Passes[deadlineIndexExclusive];

            if (!deadlinePass.PassIndexToWait)
            {
                deadlinePass.PassIndexToWait = passIndex;
            }
        }
    }

    size_t RenderGraph::AvoidAsyncComputeResourceCompetition(size_t passIndex, size_t& deadlineIndexExclusive)
    {
        const RenderGraphPass& pass = m_Passes[passIndex];
        assert(!pass.IsVisited);

        size_t overlappedPassCount = 0;

        for (size_t i = passIndex + 1; i < deadlineIndexExclusive; i++)
        {
            const RenderGraphPass& overlappedPass = m_Passes[i];
            assert(overlappedPass.IsVisited);

            if (overlappedPass.IsCulled || overlappedPass.IsAsyncCompute)
            {
                continue;
            }

            overlappedPassCount++;

            // 允许同时读，但不能同时写 or 一个读一个写

            for (size_t resourceIndex : pass.ResourcesIn)
            {
                if (overlappedPass.ResourcesOut.count(resourceIndex) > 0)
                {
                    deadlineIndexExclusive = i;
                    goto End;
                }
            }

            for (size_t resourceIndex : pass.ResourcesOut)
            {
                if (overlappedPass.ResourcesIn.count(resourceIndex) > 0)
                {
                    deadlineIndexExclusive = i;
                    goto End;
                }

                if (overlappedPass.ResourcesOut.count(resourceIndex) > 0)
                {
                    deadlineIndexExclusive = i;
                    goto End;
                }
            }
        }

    End:
        return overlappedPassCount;
    }

    GfxCommandContext* RenderGraph::EnsurePassContext(RenderGraphContext& context, const RenderGraphPass& pass)
    {
        if (pass.IsAsyncCompute)
        {
            // 对于 async compute pass，每次都要创建新的 context，这样才能得到对应的 sync point
            // 为了避免竞争，还需要等待之前的非 async compute pass 执行完
            context.New(GfxCommandType::AsyncCompute, /* waitPreviousOneOnGpu */ true);
        }
        else if (pass.PassIndexToWait)
        {
            // 需要等待某个 sync point，所以要新建一个 context
            context.New(GfxCommandType::Direct, /* waitPreviousOneOnGpu */ false);

            const GfxSyncPoint& syncPoint = m_Passes[*pass.PassIndexToWait].SyncPoint;
            assert(syncPoint.IsValid());
            context.GetCommandContext()->WaitOnGpu(syncPoint);
        }
        else
        {
            context.Ensure(GfxCommandType::Direct);
        }

        return context.GetCommandContext();
    }

    void RenderGraph::RequestPassResources(GfxCommandContext* cmd, const RenderGraphPass& pass)
    {
        for (size_t resourceIndex : pass.ResourcesBorn)
        {
            m_ResourceManager->RequestResource(resourceIndex);
        }

        for (const auto& [_, variable] : pass.Variables)
        {
            m_ResourceManager->SetAsVariable(variable.ResourceIndex, cmd, variable.Desc);
        }
    }

    void RenderGraph::ReleasePassResources(GfxCommandContext* cmd, const RenderGraphPass& pass)
    {
        if (pass.IsAsyncCompute)
        {
            // async compute 资源的生命周期会被延长，所以这个 pass 不该释放任何资源
            assert(pass.ResourcesDead.empty());
        }
        else
        {
            for (size_t resourceIndex : pass.ResourcesDead)
            {
                m_ResourceManager->ReleaseResource(resourceIndex);
            }
        }

        cmd->UnsetBuffers();
        cmd->UnsetTextures();
    }

    void RenderGraph::SetPassRenderStates(GfxCommandContext* cmd, const RenderGraphPass& pass)
    {
        if (!pass.RenderFunc)
        {
            LOG_WARNING("Render function is not set in pass '{}'", pass.Name);
            return;
        }

        // 如果没有 render target，说明这个 pass 不渲染物体，不用设置 render states
        if (pass.NumColorTargets == 0 && !pass.DepthStencilTarget.IsSet)
        {
            return;
        }

        // async compute pass 不支持设置 render states
        if (pass.IsAsyncCompute)
        {
            LOG_WARNING("Async compute pass '{}' can not have render states", pass.Name);
            return;
        }

        GfxRenderTexture* colorTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
        GfxRenderTexture* depthStencilTarget = nullptr;

        for (uint32_t i = 0; i < pass.NumColorTargets; i++)
        {
            const RenderGraphPassColorTarget& target = pass.ColorTargets[i];

            if (!target.IsSet)
            {
                LOG_WARNING("Color target '{}' is not set in pass '{}'", i, pass.Name);
                continue;
            }

            colorTargets[i] = m_ResourceManager->GetTexture(target.ResourceIndex);
        }

        if (pass.DepthStencilTarget.IsSet)
        {
            depthStencilTarget = m_ResourceManager->GetTexture(pass.DepthStencilTarget.ResourceIndex);
        }

        cmd->SetRenderTargets(pass.NumColorTargets, colorTargets, depthStencilTarget);

        if (pass.HasCustomViewport)
        {
            cmd->SetViewport(pass.CustomViewport);
        }
        else
        {
            cmd->SetDefaultViewport();
        }

        if (pass.HasCustomScissorRect)
        {
            cmd->SetScissorRect(pass.CustomScissorRect);
        }
        else
        {
            cmd->SetDefaultScissorRect();
        }

        for (uint32_t i = 0; i < pass.NumColorTargets; i++)
        {
            const RenderGraphPassColorTarget& target = pass.ColorTargets[i];

            if (target.IsSet && target.InitMode == RenderTargetInitMode::Clear)
            {
                cmd->ClearColorTarget(i, target.ClearColor);
            }
        }

        if (pass.DepthStencilTarget.IsSet && pass.DepthStencilTarget.InitMode == RenderTargetInitMode::Clear)
        {
            cmd->ClearDepthStencilTarget(pass.DepthStencilTarget.ClearDepthValue, pass.DepthStencilTarget.ClearStencilValue);
        }

        cmd->SetWireframe(pass.Wireframe);

        if (pass.HasCustomDepthBias)
        {
            cmd->SetDepthBias(pass.DepthBias, pass.SlopeScaledDepthBias, pass.DepthBiasClamp);
        }
        else
        {
            cmd->SetDefaultDepthBias();
        }
    }

    void RenderGraph::ExecutePasses()
    {
        RenderGraphContext context{};

        for (RenderGraphPass& pass : m_Passes)
        {
            if (pass.IsCulled)
            {
                continue;
            }

            GfxCommandContext* cmd = EnsurePassContext(context, pass);

            cmd->BeginEvent(pass.Name);
            {
                RequestPassResources(cmd, pass);
                SetPassRenderStates(cmd, pass);

                if (pass.RenderFunc)
                {
                    pass.RenderFunc(context);
                }

                ReleasePassResources(cmd, pass);
            }
            cmd->EndEvent();

            if (pass.IsAsyncCompute)
            {
                pass.SyncPoint = context.UncheckedSubmit();
            }
        }

        context.Submit();

        // 保证所有 async compute pass 都执行完
        if (m_PassIndexToWaitFallback)
        {
            GfxCommandManager* cmdManager = GetGfxDevice()->GetCommandManager();
            GfxCommandQueue* cmdQueue = cmdManager->GetQueue(GfxCommandType::Direct);

            const GfxSyncPoint& syncPoint = m_Passes[*m_PassIndexToWaitFallback].SyncPoint;
            assert(syncPoint.IsValid());
            cmdQueue->WaitOnGpu(syncPoint);
        }
    }

    RenderGraphBuilder RenderGraph::AddPass()
    {
        return AddPass("AnonymousPass");
    }

    RenderGraphBuilder RenderGraph::AddPass(const std::string& name)
    {
        RenderGraphPass& pass = m_Passes.emplace_back();
        pass.Name = name;

        return RenderGraphBuilder(this, m_Passes.size() - 1);
    }

    static std::unordered_set<IRenderGraphCompiledEventListener*> g_GraphCompiledEventListeners{};

    void RenderGraph::CompileAndExecute()
    {
        // TODO error guard

        try
        {
            CompilePasses();

            for (IRenderGraphCompiledEventListener* listener : g_GraphCompiledEventListeners)
            {
                listener->OnGraphCompiled(*this, m_Passes);
            }

            ExecutePasses();
        }
        catch (std::exception& e)
        {
            LOG_ERROR("render graph error: {}", e.what());
        }

        // TODO check resource leaks

        // Clean up
        m_Passes.clear();
        m_PassIndexToWaitFallback = std::nullopt;
        m_ResourceManager->ClearResources();
    }

    void RenderGraph::AddGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener)
    {
        g_GraphCompiledEventListeners.insert(listener);
    }

    void RenderGraph::RemoveGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener)
    {
        g_GraphCompiledEventListeners.erase(listener);
    }

    BufferHandle RenderGraph::Import(const std::string& name, GfxBuffer* buffer)
    {
        return Import(ShaderUtils::GetIdFromString(name), buffer);
    }

    BufferHandle RenderGraph::Import(int32 id, GfxBuffer* buffer)
    {
        return m_ResourceManager->ImportBuffer(id, buffer);
    }

    BufferHandle RenderGraph::Request(const std::string& name, const GfxBufferDesc& desc)
    {
        return Request(ShaderUtils::GetIdFromString(name), desc);
    }

    BufferHandle RenderGraph::Request(int32 id, const GfxBufferDesc& desc)
    {
        return m_ResourceManager->CreateBuffer(id, desc);
    }

    TextureHandle RenderGraph::Import(const std::string& name, GfxRenderTexture* texture)
    {
        return Import(ShaderUtils::GetIdFromString(name), texture);
    }

    TextureHandle RenderGraph::Import(int32 id, GfxRenderTexture* texture)
    {
        return m_ResourceManager->ImportTexture(id, texture);
    }

    TextureHandle RenderGraph::Request(const std::string& name, const GfxTextureDesc& desc)
    {
        return Request(ShaderUtils::GetIdFromString(name), desc);
    }

    TextureHandle RenderGraph::Request(int32 id, const GfxTextureDesc& desc)
    {
        return m_ResourceManager->CreateTexture(id, desc);
    }

    void RenderGraphContext::New(GfxCommandType type, bool waitPreviousOneOnGpu)
    {
        GfxSyncPoint prevSyncPoint{};

        if (m_Cmd != nullptr)
        {
            GfxSyncPoint syncPoint = m_Cmd->SubmitAndRelease();

            // 不在一个 command queue 里的才需要等待
            if (waitPreviousOneOnGpu && m_Cmd->GetType() != type)
            {
                prevSyncPoint = syncPoint;
            }
        }

        m_Cmd = GetGfxDevice()->RequestContext(type);

        if (prevSyncPoint.IsValid())
        {
            m_Cmd->WaitOnGpu(prevSyncPoint);
        }
    }

    void RenderGraphContext::Ensure(GfxCommandType type)
    {
        if (m_Cmd != nullptr && m_Cmd->GetType() != type)
        {
            m_Cmd->SubmitAndRelease();
            m_Cmd = nullptr;
        }

        if (m_Cmd == nullptr)
        {
            m_Cmd = GetGfxDevice()->RequestContext(type);
        }
    }

    GfxSyncPoint RenderGraphContext::UncheckedSubmit()
    {
        return std::exchange(m_Cmd, nullptr)->SubmitAndRelease();
    }

    void RenderGraphContext::Submit()
    {
        if (m_Cmd != nullptr)
        {
            m_Cmd->SubmitAndRelease();
            m_Cmd = nullptr;
        }
    }
}
