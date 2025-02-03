#include "pch.h"
#include "Engine/Rendering/RenderGraphImpl/RenderGraphCore.h"
#include "Engine/Debug.h"
#include <utility>

namespace march
{
    RenderGraphPass& RenderGraphBuilder::GetPass() const
    {
        return m_Graph->m_Passes[m_PassIndex];
    }

    void RenderGraphBuilder::ReadResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex)
    {
        RenderGraphPass& pass = GetPass();

        if (!pass.ResourcesRead.insert(resourceIndex).second)
        {
            return;
        }

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

    void RenderGraphBuilder::WriteResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex)
    {
        RenderGraphPass& pass = GetPass();

        if (!pass.ResourcesWritten.insert(resourceIndex).second)
        {
            return;
        }

        pass.HasSideEffects |= resourceManager->IsResourceExternal(resourceIndex);
        resourceManager->AddProducerPassIndex(resourceIndex, m_PassIndex);
    }

    void RenderGraphBuilder::AllowPassCulling(bool value)
    {
        GetPass().AllowPassCulling = value;
    }

    void RenderGraphBuilder::EnableAsyncCompute(bool value)
    {
        GetPass().EnableAsyncCompute = value;
    }

    void RenderGraphBuilder::Read(const BufferHandle& buffer)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(buffer);
        ReadResource(resourceManager, resourceIndex);
    }

    void RenderGraphBuilder::Write(const BufferHandle& buffer)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(buffer);
        WriteResource(resourceManager, resourceIndex);
    }

    void RenderGraphBuilder::ReadWrite(const BufferHandle& buffer)
    {
        Read(buffer);
        Write(buffer);
    }

    void RenderGraphBuilder::Read(const TextureHandle& texture)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(texture);
        ReadResource(resourceManager, resourceIndex);
    }

    void RenderGraphBuilder::Write(const TextureHandle& texture)
    {
        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        size_t resourceIndex = resourceManager->GetResourceIndex(texture);
        WriteResource(resourceManager, resourceIndex);
    }

    void RenderGraphBuilder::ReadWrite(const TextureHandle& texture)
    {
        Read(texture);
        Write(texture);
    }

    void RenderGraphBuilder::SetColorTarget(const TextureHandle& texture, bool load)
    {
        SetColorTarget(texture, 0, load);
    }

    void RenderGraphBuilder::SetColorTarget(const TextureHandle& texture, uint32_t index, bool load)
    {
        RenderGraphPass& pass = GetPass();

        if (index >= std::size(pass.ColorTargets))
        {
            LOG_ERROR("Color target index '{}' out of range in pass '{}'", index, pass.Name);
            return;
        }

        pass.NumColorTargets = std::max(pass.NumColorTargets, index + 1u);
        RenderGraphPassRenderTarget& target = pass.ColorTargets[index];

        if (target.IsSet)
        {
            LOG_ERROR("Can not set color target '{}' multiple times in pass '{}'", index, pass.Name);
            return;
        }

        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        target.ResourceIndex = resourceManager->GetResourceIndex(texture);
        target.IsSet = true;
        target.Load = load;

        if (load)
        {
            ReadResource(resourceManager, target.ResourceIndex);
        }

        WriteResource(resourceManager, target.ResourceIndex);
    }

    void RenderGraphBuilder::SetDepthStencilTarget(const TextureHandle& texture, bool load)
    {
        RenderGraphPass& pass = GetPass();
        RenderGraphPassRenderTarget& target = pass.DepthStencilTarget;

        if (target.IsSet)
        {
            LOG_ERROR("Can not set depth stencil target multiple times in pass '{}'", pass.Name);
            return;
        }

        RenderGraphResourceManager* resourceManager = m_Graph->m_ResourceManager.get();
        target.ResourceIndex = resourceManager->GetResourceIndex(texture);
        target.IsSet = true;
        target.Load = load;

        if (load)
        {
            ReadResource(resourceManager, target.ResourceIndex);
        }

        WriteResource(resourceManager, target.ResourceIndex);
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

    void RenderGraphBuilder::SetDepthBias(int32_t bias, float slopeScaledBias, float clamp)
    {
        RenderGraphPass& pass = GetPass();

        pass.HashCustomDepthBias = true;
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

    bool RenderGraph::CullPasses()
    {
        for (size_t i = 0; i < m_Passes.size(); i++)
        {
            if (m_Passes[i].State == RenderGraphPassState::None)
            {
                if (!CullPassDFS(i))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool RenderGraph::CullPassDFS(size_t passIndex)
    {
        RenderGraphPass& pass = m_Passes[passIndex];
        pass.State = RenderGraphPassState::Visiting;

        size_t outdegree = 0;

        for (size_t adjIndex : pass.NextPassIndices)
        {
            RenderGraphPass& adjPass = m_Passes[adjIndex];

            if (adjPass.State == RenderGraphPassState::Visiting)
            {
                LOG_ERROR("A cycle is detected in the render graph");
                return false;
            }

            if (adjPass.State == RenderGraphPassState::None)
            {
                if (!CullPassDFS(adjIndex))
                {
                    return false;
                }
            }

            if (adjPass.State != RenderGraphPassState::Culled)
            {
                outdegree++;
            }
        }

        if (outdegree == 0 && !pass.HasSideEffects && pass.AllowPassCulling)
        {
            pass.State = RenderGraphPassState::Culled;
        }
        else
        {
            pass.State = RenderGraphPassState::Visited;
        }

        return true;
    }

    void RenderGraph::ComputeResourceLifetime()
    {
        for (size_t i = 0; i < m_Passes.size(); i++)
        {
            const RenderGraphPass& pass = m_Passes[i];

            if (pass.State == RenderGraphPassState::Culled)
            {
                continue;
            }

            for (size_t resourceIndex : pass.ResourcesRead)
            {
                m_ResourceManager->SetAlive(resourceIndex, i);
            }

            for (size_t resourceIndex : pass.ResourcesWritten)
            {
                m_ResourceManager->SetAlive(resourceIndex, i);
            }
        }

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

    bool RenderGraph::CompilePasses()
    {
        if (CullPasses())
        {
            ComputeResourceLifetime();
            return true;
        }

        return false;
    }

    void RenderGraph::SetPassRenderTargets(GfxCommandContext* context, RenderGraphPass& pass)
    {
        if (pass.NumColorTargets == 0 && !pass.DepthStencilTarget.IsSet)
        {
            return;
        }

        GfxRenderTexture* colorTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
        GfxRenderTexture* depthStencilTarget = nullptr;

        for (uint32_t i = 0; i < pass.NumColorTargets; i++)
        {
            if (!pass.ColorTargets[i].IsSet)
            {
                LOG_ERROR("Color target '{}' is not set in pass '{}'", i, pass.Name);
                continue;
            }

            colorTargets[i] = m_ResourceManager->GetTexture(pass.ColorTargets[i].ResourceIndex);
        }

        if (pass.DepthStencilTarget.IsSet)
        {
            depthStencilTarget = m_ResourceManager->GetTexture(pass.DepthStencilTarget.ResourceIndex);
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

    void RenderGraph::ExecutePasses()
    {
        RenderGraphContext context{};

        for (RenderGraphPass& pass : m_Passes)
        {
            if (pass.State == RenderGraphPassState::Culled)
            {
                continue;
            }

            context.ClearPassData();

            for (size_t resourceIndex : pass.ResourcesBorn)
            {
                m_ResourceManager->RequestResource(resourceIndex);
            }

            context.GetCommandContext()->BeginEvent(pass.Name);
            {
                SetPassRenderTargets(context.GetCommandContext(), pass);
                context.GetCommandContext()->SetWireframe(pass.Wireframe);

                if (pass.HashCustomDepthBias)
                {
                    context.GetCommandContext()->SetDepthBias(pass.DepthBias, pass.SlopeScaledDepthBias, pass.DepthBiasClamp);
                }
                else
                {
                    context.GetCommandContext()->SetDefaultDepthBias();
                }

                if (pass.RenderFunc)
                {
                    pass.RenderFunc(context);
                }
            }
            context.GetCommandContext()->EndEvent();

            for (size_t resourceIndex : pass.ResourcesDead)
            {
                m_ResourceManager->ReleaseResource(resourceIndex);
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
        // TODO error guard

        try
        {
            if (CompilePasses())
            {
                for (IRenderGraphCompiledEventListener* listener : g_GraphCompiledEventListeners)
                {
                    listener->OnGraphCompiled(*this, m_Passes);
                }

                ExecutePasses();
            }
        }
        catch (std::exception& e)
        {
            LOG_ERROR("error: {}", e.what());
        }

        // TODO check resource leaks

        // Clean up
        m_Passes.clear();
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
        m_Context->UnsetBuffers();
        m_Context->UnsetTextures();
    }
}
