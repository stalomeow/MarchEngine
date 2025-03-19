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

    void RenderGraphBuilder::InResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex, RenderGraphPassResourceUsages usages)
    {
        assert(usages != RenderGraphPassResourceUsages::None);

        RenderGraphPass& pass = GetPass();
        RenderGraphPassResourceUsages& lastUsages = pass.ResourcesIn[resourceIndex];

        // 处理新的资源
        if (lastUsages == RenderGraphPassResourceUsages::None)
        {
            std::optional<size_t> producerPassIndex = resourceManager->GetLastProducerBeforePassIndex(resourceIndex, m_PassIndex);

            if (producerPassIndex)
            {
                m_Graph->m_Passes[*producerPassIndex].NextPassIndices.insert(m_PassIndex);
            }
            else if (!resourceManager->IsExternalResource(resourceIndex))
            {
                // 非外部导入的资源应该有生产者

                int32 id = resourceManager->GetResourceId(resourceIndex);
                LOG_WARNING("Failed to find producer pass for resource '{}' in pass '{}'", ShaderUtils::GetStringFromId(id), pass.Name);
            }
        }

        lastUsages |= usages;
    }

    void RenderGraphBuilder::OutResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex, RenderGraphPassResourceUsages usages)
    {
        assert(usages != RenderGraphPassResourceUsages::None);

        RenderGraphPass& pass = GetPass();

        if (!resourceManager->AllowGpuWritingResource(resourceIndex))
        {
            int32 id = resourceManager->GetResourceId(resourceIndex);
            LOG_ERROR("Resource '{}' is not allowed to be written in pass '{}' on GPU", ShaderUtils::GetStringFromId(id), pass.Name);
            return;
        }

        RenderGraphPassResourceUsages& lastUsages = pass.ResourcesOut[resourceIndex];

        // 处理新的资源
        if (lastUsages == RenderGraphPassResourceUsages::None)
        {
            pass.HasSideEffects |= resourceManager->IsExternalResource(resourceIndex);
            resourceManager->AddProducerPassIndex(resourceIndex, m_PassIndex);
        }

        lastUsages |= usages;
    }

    void RenderGraphBuilder::AllowPassCulling(bool value)
    {
        GetPass().AllowPassCulling = value;
    }

    void RenderGraphBuilder::EnableAsyncCompute(bool value)
    {
        GetPass().EnableAsyncCompute = value;
    }

    void RenderGraphBuilder::UseDefaultVariables(bool value)
    {
        GetPass().UseDefaultVariables = value;
    }

    void RenderGraphBuilder::In(const BufferHandle& buffer)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(buffer);
        InResource(resourceManager, resourceIndex, RenderGraphPassResourceUsages::Variable);
    }

    void RenderGraphBuilder::Out(const BufferHandle& buffer)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(buffer);
        OutResource(resourceManager, resourceIndex, RenderGraphPassResourceUsages::Variable);
    }

    void RenderGraphBuilder::InOut(const BufferHandle& buffer)
    {
        In(buffer);
        Out(buffer);
    }

    void RenderGraphBuilder::In(const TextureHandle& texture)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(texture);
        InResource(resourceManager, resourceIndex, RenderGraphPassResourceUsages::Variable);
    }

    void RenderGraphBuilder::Out(const TextureHandle& texture)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(texture);
        OutResource(resourceManager, resourceIndex, RenderGraphPassResourceUsages::Variable);
    }

    void RenderGraphBuilder::InOut(const TextureHandle& texture)
    {
        In(texture);
        Out(texture);
    }

    void RenderGraphBuilder::SetColorTarget(const TextureSliceHandle& texture, RenderTargetInitMode initMode, const float color[4])
    {
        SetColorTarget(texture, 0, initMode, color);
    }

    void RenderGraphBuilder::SetColorTarget(const TextureSliceHandle& texture, uint32_t index, RenderTargetInitMode initMode, const float color[4])
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
        target.ResourceIndex = resourceManager->GetResourceIndex(texture.Handle);
        target.IsSet = true;
        target.Face = texture.Face;
        target.WOrArraySlice = texture.WOrArraySlice;
        target.MipSlice = texture.MipSlice;
        target.InitMode = initMode;
        std::copy_n(color, std::size(target.ClearColor), target.ClearColor);

        if (initMode == RenderTargetInitMode::Load)
        {
            InResource(resourceManager, target.ResourceIndex, RenderGraphPassResourceUsages::RenderTarget);
        }

        OutResource(resourceManager, target.ResourceIndex, RenderGraphPassResourceUsages::RenderTarget);
    }

    void RenderGraphBuilder::SetDepthStencilTarget(const TextureSliceHandle& texture, RenderTargetInitMode initMode, float depth, uint8_t stencil)
    {
        RenderGraphPass& pass = GetPass();
        RenderGraphPassDepthStencilTarget& target = pass.DepthStencilTarget;

        if (target.IsSet)
        {
            LOG_ERROR("Can not set depth stencil target multiple times in pass '{}'", pass.Name);
            return;
        }

        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        target.ResourceIndex = resourceManager->GetResourceIndex(texture.Handle);
        target.IsSet = true;
        target.Face = texture.Face;
        target.WOrArraySlice = texture.WOrArraySlice;
        target.MipSlice = texture.MipSlice;
        target.InitMode = initMode;
        target.ClearDepthValue = depth;
        target.ClearStencilValue = stencil;

        if (initMode == RenderTargetInitMode::Load)
        {
            InResource(resourceManager, target.ResourceIndex, RenderGraphPassResourceUsages::RenderTarget);
        }

        OutResource(resourceManager, target.ResourceIndex, RenderGraphPassResourceUsages::RenderTarget);
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

        // 注意 size_t 是无符号的，所以不能用 passIndex >= 0
        for (size_t passIndex = m_Passes.size() - 1; passIndex != -1; passIndex--)
        {
            CullPass(passIndex, asyncComputeDeadlineIndexExclusive);
        }

        BatchAsyncComputePasses();

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

            for (const auto& kv : pass.ResourcesIn)
            {
                size_t resourceIndex = kv.first;
                m_ResourceManager->SetAlive(resourceIndex, passIndex);

                // async compute 需要延长资源的生命周期
                if (pass.IsAsyncCompute)
                {
                    m_ResourceManager->SetAlive(resourceIndex, asyncComputeDeadlineIndexExclusive - 1);
                }
            }

            for (const auto& kv : pass.ResourcesOut)
            {
                size_t resourceIndex = kv.first;
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

        size_t overlappedPassCount = AvoidAsyncComputeResourceHazard(passIndex, deadlineIndexExclusive);

        // 有重叠的 pass 时 async compute 才有意义
        if (overlappedPassCount == 0)
        {
            pass.IsAsyncCompute = false;
            return;
        }

        pass.IsAsyncCompute = true;

        // 如果后面的 async compute 执行完了，那前面的肯定也执行完了
        // 因为 compile 时是从后往前遍历的，所以第一次赋给 PassIndexToWait 的值一定是最大的，不用再更新

        if (deadlineIndexExclusive >= m_Passes.size())
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

    size_t RenderGraph::AvoidAsyncComputeResourceHazard(size_t passIndex, size_t& deadlineIndexExclusive)
    {
        const RenderGraphPass& pass = m_Passes[passIndex];
        assert(!pass.IsVisited);

        size_t overlappedPassCount = 0;
        size_t lastNonAsyncComputePassIndex = deadlineIndexExclusive;

        for (size_t i = passIndex + 1; i < deadlineIndexExclusive; i++)
        {
            const RenderGraphPass& overlappedPass = m_Passes[i];
            assert(overlappedPass.IsVisited);

            if (overlappedPass.IsCulled)
            {
                continue;
            }

            if (!overlappedPass.IsAsyncCompute)
            {
                lastNonAsyncComputePassIndex = i;
            }

            for (const auto& kv : pass.ResourcesIn)
            {
                size_t resourceIndex = kv.first;

                // 在没有 resource barrier 时允许两个同时读
                if (overlappedPass.ResourcesIn.count(resourceIndex) > 0)
                {
                    // 如果不是 D3D12_RESOURCE_STATE_GENERIC_READ 的话，读取前可能会有一个 resource barrier
                    // 当两个 pass 在 GPU 上并行时，两个 barrier 的顺序不确定，可能导致状态损坏
                    if (!m_ResourceManager->IsGenericallyReadableResource(resourceIndex))
                    {
                        deadlineIndexExclusive = lastNonAsyncComputePassIndex;
                        goto End;
                    }
                }

                // 禁止一个读一个写
                if (overlappedPass.ResourcesOut.count(resourceIndex) > 0)
                {
                    deadlineIndexExclusive = lastNonAsyncComputePassIndex;
                    goto End;
                }
            }

            for (const auto& kv : pass.ResourcesOut)
            {
                size_t resourceIndex = kv.first;

                // 禁止一个读一个写
                if (overlappedPass.ResourcesIn.count(resourceIndex) > 0)
                {
                    deadlineIndexExclusive = lastNonAsyncComputePassIndex;
                    goto End;
                }

                // 禁止两个同时写
                if (overlappedPass.ResourcesOut.count(resourceIndex) > 0)
                {
                    deadlineIndexExclusive = lastNonAsyncComputePassIndex;
                    goto End;
                }
            }

            // 每个 async compute pass 在开始前，会在 direct command queue 中插入 resource barrier
            // 如果出现两个 async compute pass 先后执行（中间没有 deadline），那么后一个 pass 的 resource barrier 可能和前一个 pass 并行
            // 所以，遇到 async compute pass 时，也要做上面的 read write 检查，避免状态损坏
            // 但是 async compute pass 是在 async compute command queue 上先后执行的，所以不算 overlap
            if (!overlappedPass.IsAsyncCompute)
            {
                overlappedPassCount++;
            }
        }

    End:
        return overlappedPassCount;
    }

    void RenderGraph::BatchAsyncComputePasses()
    {
        std::optional<size_t> firstAsyncComputePassIndex = std::nullopt;
        std::optional<size_t> lastAsyncComputePassIndex = std::nullopt;

        // 只有连续的 async compute pass 才能合批

        for (size_t passIndex = 0; passIndex < m_Passes.size(); passIndex++)
        {
            RenderGraphPass& pass = m_Passes[passIndex];

            if (pass.IsCulled)
            {
                continue;
            }

            if (pass.IsAsyncCompute)
            {
                if (firstAsyncComputePassIndex)
                {
                    pass.IsBatchedWithPrevious = true;

                    // 合批后，Resource Barrier 会提前到第一个 async compute pass，资源的生命周期也要提前
                    size_t firstAsyncComputePassIndexValue = *firstAsyncComputePassIndex;

                    for (const auto& kv : pass.ResourcesIn)
                    {
                        size_t resourceIndex = kv.first;
                        m_ResourceManager->SetAlive(resourceIndex, firstAsyncComputePassIndexValue);
                    }

                    for (const auto& kv : pass.ResourcesOut)
                    {
                        size_t resourceIndex = kv.first;
                        m_ResourceManager->SetAlive(resourceIndex, firstAsyncComputePassIndexValue);
                    }
                }
                else
                {
                    firstAsyncComputePassIndex = passIndex;
                }

                lastAsyncComputePassIndex = passIndex;
            }
            else if (lastAsyncComputePassIndex)
            {
                m_Passes[*lastAsyncComputePassIndex].NeedSyncPoint = true;

                firstAsyncComputePassIndex = std::nullopt;
                lastAsyncComputePassIndex = std::nullopt;
            }
        }

        if (lastAsyncComputePassIndex)
        {
            m_Passes[*lastAsyncComputePassIndex].NeedSyncPoint = true;

            firstAsyncComputePassIndex = std::nullopt;
            lastAsyncComputePassIndex = std::nullopt;
        }
    }

    void RenderGraph::RequestPassResources(const RenderGraphPass& pass)
    {
        for (size_t resourceIndex : pass.ResourcesBorn)
        {
            m_ResourceManager->RequestResource(resourceIndex);
        }
    }

    void RenderGraph::EnsureAsyncComputePassResourceStates(RenderGraphContext& context, size_t passIndex)
    {
        // https://microsoft.github.io/DirectX-Specs/d3d/CPUEfficiency.html#state-support-by-command-list-type
        constexpr D3D12_RESOURCE_STATES disallowedComputeStates
            = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
            | D3D12_RESOURCE_STATE_INDEX_BUFFER
            | D3D12_RESOURCE_STATE_RENDER_TARGET
            | D3D12_RESOURCE_STATE_DEPTH_WRITE
            | D3D12_RESOURCE_STATE_DEPTH_READ
            | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
            | D3D12_RESOURCE_STATE_STREAM_OUT
            | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT
            | D3D12_RESOURCE_STATE_RESOLVE_DEST
            | D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

        // 需要避免在 AsyncComputeCommandList 中使用含有 disallowedComputeStates 的 Resource Barriers，
        // BeforeState 和 AfterState 都不行，否则会报错！！！
        // 这里检查资源的状态，如果不符合要求，就提前在 DirectCommandList 中将资源转换成 Common 状态

        bool hasValidDirectContext = false;

        for (size_t i = passIndex; i < m_Passes.size(); i++)
        {
            const RenderGraphPass& pass = m_Passes[i];

            if (pass.IsCulled)
            {
                continue;
            }

            // 如果合批的话，需要把 Resource Barrier 提前到第一个 async compute pass
            if (!pass.IsAsyncCompute || (i != passIndex && !pass.IsBatchedWithPrevious))
            {
                break;
            }

            for (const auto& kv : pass.ResourcesIn)
            {
                size_t resourceIndex = kv.first;

                // 如果既读又写，那么当作写，在下面一个循环中处理
                if (pass.ResourcesOut.count(resourceIndex) > 0)
                {
                    continue;
                }

                auto res = m_ResourceManager->GetUnderlyingResource(resourceIndex);

                if (!res->HasAllStates(D3D12_RESOURCE_STATE_GENERIC_READ))
                {
                    if (!hasValidDirectContext)
                    {
                        context.Ensure(GfxCommandType::Direct);
                        hasValidDirectContext = true;
                    }

                    // 只读的资源都转成 GENERIC_READ，避免两个并行的 pass 中出现 Resource Barrier
                    context.GetCommandContext()->TransitionResource(res, D3D12_RESOURCE_STATE_GENERIC_READ);
                }
            }

            for (const auto& kv : pass.ResourcesOut)
            {
                size_t resourceIndex = kv.first;
                auto res = m_ResourceManager->GetUnderlyingResource(resourceIndex);

                if (res->HasAnyStates(disallowedComputeStates))
                {
                    if (!hasValidDirectContext)
                    {
                        context.Ensure(GfxCommandType::Direct);
                        hasValidDirectContext = true;
                    }

                    // 需要写的资源都转成 COMMON，避免 Resource Barrier 中出现 disallowedComputeStates
                    context.GetCommandContext()->TransitionResource(res, D3D12_RESOURCE_STATE_COMMON);
                }
            }
        }
    }

    GfxCommandContext* RenderGraph::EnsurePassContext(RenderGraphContext& context, size_t passIndex)
    {
        const RenderGraphPass& pass = m_Passes[passIndex];

        if (pass.IsAsyncCompute)
        {
            if (pass.IsBatchedWithPrevious)
            {
                context.Ensure(GfxCommandType::AsyncCompute);
            }
            else
            {
                EnsureAsyncComputePassResourceStates(context, passIndex);

                // 对于 async compute pass，要创建新的 context，这样才能得到对应的 sync point
                // 为了避免资源竞争，还需要等待之前的非 async compute pass 执行完
                context.New(GfxCommandType::AsyncCompute, /* waitPreviousOneOnGpu */ true);
            }
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

    GfxRenderTargetDesc RenderGraph::ResolveRenderTarget(const RenderGraphPassRenderTarget& target)
    {
        GfxRenderTargetDesc desc{};
        desc.Texture = m_ResourceManager->GetTexture(target.ResourceIndex);
        desc.Face = target.Face;
        desc.WOrArraySlice = target.WOrArraySlice;
        desc.MipSlice = target.MipSlice;
        return desc;
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

        GfxRenderTargetDesc colorTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};

        for (uint32_t i = 0; i < pass.NumColorTargets; i++)
        {
            const RenderGraphPassColorTarget& target = pass.ColorTargets[i];

            if (!target.IsSet)
            {
                LOG_WARNING("Color target '{}' is not set in pass '{}'", i, pass.Name);
                continue;
            }

            colorTargets[i] = ResolveRenderTarget(target);
        }

        if (pass.DepthStencilTarget.IsSet)
        {
            GfxRenderTargetDesc depthStencilTarget = ResolveRenderTarget(pass.DepthStencilTarget);
            cmd->SetRenderTargets(pass.NumColorTargets, colorTargets, depthStencilTarget);
        }
        else
        {
            cmd->SetRenderTargets(pass.NumColorTargets, colorTargets);
        }

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

    void RenderGraph::ReleasePassResources(const RenderGraphPass& pass)
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
    }

    void RenderGraph::SetPassDefaultVariables(GfxCommandContext* cmd, const RenderGraphPass& pass)
    {
        if (!pass.UseDefaultVariables)
        {
            return;
        }

        auto fn = [this, cmd](size_t resourceIndex, RenderGraphPassResourceUsages usages)
        {
            if ((usages & RenderGraphPassResourceUsages::Variable) == RenderGraphPassResourceUsages::Variable)
            {
                m_ResourceManager->SetDefaultVariable(resourceIndex, cmd);
            }
        };

        for (const auto& kv : pass.ResourcesIn)
        {
            fn(kv.first, kv.second);
        }

        for (const auto& kv : pass.ResourcesOut)
        {
            fn(kv.first, kv.second);
        }
    }

    void RenderGraph::ExecutePasses()
    {
        RenderGraphContext context{};

        try
        {
            for (size_t passIndex = 0; passIndex < m_Passes.size(); passIndex++)
            {
                RenderGraphPass& pass = m_Passes[passIndex];

                if (pass.IsCulled)
                {
                    continue;
                }

                RequestPassResources(pass);
                GfxCommandContext* cmd = EnsurePassContext(context, passIndex);

                cmd->BeginEvent(pass.Name);
                {
                    SetPassRenderStates(cmd, pass);
                    SetPassDefaultVariables(cmd, pass);

                    if (pass.RenderFunc)
                    {
                        pass.RenderFunc(context);
                    }

                    cmd->UnsetTexturesAndBuffers();
                }
                cmd->EndEvent();

                ReleasePassResources(pass);

                if (pass.IsAsyncCompute && pass.NeedSyncPoint)
                {
                    pass.SyncPoint = context.UncheckedSubmit();
                }
            }
        }
        catch (std::exception& e)
        {
            LOG_ERROR("RenderGraphExecutionError: {}", e.what());
        }

        context.Submit();

        // 保证所有 async compute pass 都执行完
        if (m_PassIndexToWaitFallback)
        {
            const GfxSyncPoint& syncPoint = m_Passes[*m_PassIndexToWaitFallback].SyncPoint;

            if (syncPoint.IsValid())
            {
                GfxCommandManager* cmdManager = GetGfxDevice()->GetCommandManager();
                GfxCommandQueue* cmdQueue = cmdManager->GetQueue(GfxCommandType::Direct);
                cmdQueue->WaitOnGpu(syncPoint);
            }
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
        DeferredCleanup cleanup{ this };

        CompilePasses();

        for (IRenderGraphCompiledEventListener* listener : g_GraphCompiledEventListeners)
        {
            listener->OnGraphCompiled(m_Passes, m_ResourceManager.get());
        }

        ExecutePasses();
    }

    void RenderGraph::AddGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener)
    {
        g_GraphCompiledEventListeners.insert(listener);
    }

    void RenderGraph::RemoveGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener)
    {
        g_GraphCompiledEventListeners.erase(listener);
    }

    BufferHandle RenderGraph::ImportBuffer(const std::string& name, GfxBuffer* buffer)
    {
        return ImportBuffer(ShaderUtils::GetIdFromString(name), buffer);
    }

    BufferHandle RenderGraph::ImportBuffer(int32 id, GfxBuffer* buffer)
    {
        return m_ResourceManager->ImportBuffer(id, buffer);
    }

    BufferHandle RenderGraph::RequestBuffer(const std::string& name, const GfxBufferDesc& desc)
    {
        return RequestBuffer(ShaderUtils::GetIdFromString(name), desc);
    }

    BufferHandle RenderGraph::RequestBuffer(int32 id, const GfxBufferDesc& desc)
    {
        return m_ResourceManager->CreateBuffer(id, desc, nullptr, std::nullopt);
    }

    BufferHandle RenderGraph::RequestBufferWithContent(const std::string& name, const GfxBufferDesc& desc, const void* pData, std::optional<uint32_t> counter)
    {
        return RequestBufferWithContent(ShaderUtils::GetIdFromString(name), desc, pData, counter);
    }

    BufferHandle RenderGraph::RequestBufferWithContent(int32 id, const GfxBufferDesc& desc, const void* pData, std::optional<uint32_t> counter)
    {
        return m_ResourceManager->CreateBuffer(id, desc, pData, counter);
    }

    TextureHandle RenderGraph::ImportTexture(const std::string& name, GfxTexture* texture)
    {
        return ImportTexture(ShaderUtils::GetIdFromString(name), texture);
    }

    TextureHandle RenderGraph::ImportTexture(int32 id, GfxTexture* texture)
    {
        return m_ResourceManager->ImportTexture(id, texture);
    }

    TextureHandle RenderGraph::RequestTexture(const std::string& name, const GfxTextureDesc& desc)
    {
        return RequestTexture(ShaderUtils::GetIdFromString(name), desc);
    }

    TextureHandle RenderGraph::RequestTexture(int32 id, const GfxTextureDesc& desc)
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

    void RenderGraphContext::SetVariable(const TextureHandle& texture, GfxTextureElement element, std::optional<uint32_t> mipSlice)
    {
        SetVariable(texture, texture.GetId(), element, mipSlice);
    }

    void RenderGraphContext::SetVariable(const TextureHandle& texture, const std::string& aliasName, GfxTextureElement element, std::optional<uint32_t> mipSlice)
    {
        SetVariable(texture, ShaderUtils::GetIdFromString(aliasName), element, mipSlice);
    }

    void RenderGraphContext::SetVariable(const TextureHandle& texture, int32 aliasId, GfxTextureElement element, std::optional<uint32_t> mipSlice)
    {
        m_Cmd->SetTexture(aliasId, texture, element, mipSlice);
    }

    void RenderGraphContext::SetVariable(const BufferHandle& buffer, GfxBufferElement element)
    {
        SetVariable(buffer, buffer.GetId(), element);
    }

    void RenderGraphContext::SetVariable(const BufferHandle& buffer, const std::string& aliasName, GfxBufferElement element)
    {
        SetVariable(buffer, ShaderUtils::GetIdFromString(aliasName), element);
    }

    void RenderGraphContext::SetVariable(const BufferHandle& buffer, int32 aliasId, GfxBufferElement element)
    {
        m_Cmd->SetBuffer(aliasId, buffer, element);
    }

    void RenderGraphContext::UnsetVariables()
    {
        m_Cmd->UnsetTexturesAndBuffers();
    }
}
