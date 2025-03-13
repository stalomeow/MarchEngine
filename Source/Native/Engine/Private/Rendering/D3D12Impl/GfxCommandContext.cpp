#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxCommand.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxResource.h"
#include "Engine/Rendering/D3D12Impl/GfxMesh.h"
#include "Engine/Rendering/D3D12Impl/Material.h"
#include "Engine/Rendering/D3D12Impl/MeshRenderer.h"
#include "Engine/Rendering/D3D12Impl/ShaderUtils.h"
#include "Engine/Misc/MathUtils.h"
#include "Engine/Debug.h"
#include "Engine/Transform.h"
#include <pix3.h>
#include <assert.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace march
{
    GfxRenderTargetDesc::GfxRenderTargetDesc(GfxTexture* texture)
        : Texture(texture)
        , Face(GfxCubemapFace::PositiveX)
        , WOrArraySlice(0)
        , MipSlice(0)
    {
    }

    GfxRenderTargetDesc GfxRenderTargetDesc::Tex2D(GfxTexture* texture, uint32_t mipSlice)
    {
        GfxRenderTargetDesc desc{ texture };
        desc.MipSlice = mipSlice;
        return desc;
    }

    GfxRenderTargetDesc GfxRenderTargetDesc::Tex3D(GfxTexture* texture, uint32_t wSlice, uint32_t mipSlice)
    {
        GfxRenderTargetDesc desc{ texture };
        desc.WOrArraySlice = wSlice;
        desc.MipSlice = mipSlice;
        return desc;
    }

    GfxRenderTargetDesc GfxRenderTargetDesc::Cube(GfxTexture* texture, GfxCubemapFace face, uint32_t mipSlice)
    {
        GfxRenderTargetDesc desc{ texture };
        desc.Face = face;
        desc.MipSlice = mipSlice;
        return desc;
    }

    GfxRenderTargetDesc GfxRenderTargetDesc::Tex2DArray(GfxTexture* texture, uint32_t arraySlice, uint32_t mipSlice)
    {
        GfxRenderTargetDesc desc{ texture };
        desc.WOrArraySlice = arraySlice;
        desc.MipSlice = mipSlice;
        return desc;
    }

    GfxRenderTargetDesc GfxRenderTargetDesc::CubeArray(GfxTexture* texture, GfxCubemapFace face, uint32_t arraySlice, uint32_t mipSlice)
    {
        GfxRenderTargetDesc desc{ texture };
        desc.Face = face;
        desc.WOrArraySlice = arraySlice;
        desc.MipSlice = mipSlice;
        return desc;
    }

    GfxCommandContext::GfxCommandContext(GfxDevice* device, GfxCommandType type)
        : m_Device(device)
        , m_Type(type)
        , m_CommandAllocator(nullptr)
        , m_CommandList(nullptr)
        , m_ResourceBarriers{}
        , m_SyncPointsToWait{}
        , m_GraphicsViewCache(device)
        , m_ComputeViewCache(device)
        , m_ViewHeap(nullptr)
        , m_SamplerHeap(nullptr)
        , m_ColorTargets{}
        , m_DepthStencilTarget{}
        , m_NumViewports(0)
        , m_Viewports{}
        , m_NumScissorRects(0)
        , m_ScissorRects{}
        , m_OutputDesc{}
        , m_CurrentPipelineState(nullptr)
        , m_CurrentPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
        , m_CurrentVertexBuffer{}
        , m_CurrentIndexBuffer{}
        , m_CurrentStencilRef(std::nullopt)
        , m_GlobalTextures{}
        , m_GlobalBuffers{}
        , m_InstanceBuffer{ device, "_InstanceBuffer" }
    {
    }

    void GfxCommandContext::Open()
    {
        assert(m_CommandAllocator == nullptr);

        GfxCommandQueue* queue = m_Device->GetCommandManager()->GetQueue(m_Type);
        m_CommandAllocator = queue->RequestCommandAllocator();

        if (m_CommandList == nullptr)
        {
            CHECK_HR(m_Device->GetD3DDevice4()->CreateCommandList(0, queue->GetType(),
                m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
        }
        else
        {
            CHECK_HR(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));
        }
    }

    GfxSyncPoint GfxCommandContext::SubmitAndRelease()
    {
        GfxCommandManager* manager = m_Device->GetCommandManager();
        GfxCommandQueue* queue = manager->GetQueue(m_Type);

        // 准备好所有命令，然后关闭
        FlushResourceBarriers();
        CHECK_HR(m_CommandList->Close());

        // 等待其他 queue 上的异步操作，例如 async compute, async copy
        for (const GfxSyncPoint& syncPoint : m_SyncPointsToWait)
        {
            queue->WaitOnGpu(syncPoint);
        }

        // 正式提交
        ID3D12CommandList* commandLists[] = { m_CommandList.Get() };
        queue->GetQueue()->ExecuteCommandLists(static_cast<UINT>(std::size(commandLists)), commandLists);
        GfxSyncPoint syncPoint = queue->ReleaseCommandAllocator(m_CommandAllocator);

        // 清除状态、释放临时资源
        m_CommandAllocator = nullptr;
        m_ResourceBarriers.clear();
        m_SyncPointsToWait.clear();
        m_GraphicsViewCache.Reset();
        m_ComputeViewCache.Reset();
        m_ViewHeap = nullptr;
        m_SamplerHeap = nullptr;
        memset(m_ColorTargets, 0, sizeof(m_ColorTargets));
        m_DepthStencilTarget = RenderTargetData{};
        m_NumViewports = 0;
        m_NumScissorRects = 0;
        m_OutputDesc = {};
        m_CurrentPipelineState = nullptr;
        m_CurrentPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        m_CurrentVertexBuffer = {};
        m_CurrentIndexBuffer = {};
        m_CurrentStencilRef = std::nullopt;
        m_GlobalTextures.clear();
        m_GlobalBuffers.clear();
        m_InstanceBuffer.ReleaseResource();

        // 回收
        manager->RecycleContext(this);
        return syncPoint;
    }

    void GfxCommandContext::BeginEvent(const std::string& name)
    {
        PIXBeginEvent(m_CommandList.Get(), 0, name.c_str());
    }

    void GfxCommandContext::EndEvent()
    {
        PIXEndEvent(m_CommandList.Get());
    }

    void GfxCommandContext::TransitionResource(RefCountPtr<GfxResource> resource, D3D12_RESOURCE_STATES stateAfter)
    {
        D3D12_RESOURCE_STATES stateBefore = resource->GetState();
        bool needTransition;

        if (stateAfter == D3D12_RESOURCE_STATE_COMMON)
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states
            // D3D12_RESOURCE_STATE_COMMON 为 0，要特殊处理
            needTransition = stateBefore != stateAfter;
        }
        else
        {
            needTransition = (stateBefore & stateAfter) != stateAfter;
        }

        if (needTransition)
        {
            ID3D12Resource* res = resource->GetD3DResource();
            m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(res, stateBefore, stateAfter));
            resource->SetState(stateAfter);
        }
    }

    void GfxCommandContext::FlushResourceBarriers()
    {
        // 尽量合并后一起提交

        if (!m_ResourceBarriers.empty())
        {
            UINT count = static_cast<UINT>(m_ResourceBarriers.size());
            m_CommandList->ResourceBarrier(count, m_ResourceBarriers.data());
            m_ResourceBarriers.clear();
        }
    }

    void GfxCommandContext::WaitOnGpu(const GfxSyncPoint& syncPoint)
    {
        m_SyncPointsToWait.push_back(syncPoint);
    }

    void GfxCommandContext::SetTexture(const std::string& name, GfxTexture* value, GfxTextureElement element, uint32_t unorderedAccessMipSlice)
    {
        SetTexture(ShaderUtils::GetIdFromString(name), value, element, unorderedAccessMipSlice);
    }

    void GfxCommandContext::SetTexture(int32_t id, GfxTexture* value, GfxTextureElement element, uint32_t unorderedAccessMipSlice)
    {
        m_GlobalTextures[id] = GlobalTextureData{ value, element , unorderedAccessMipSlice };
    }

    void GfxCommandContext::UnsetTextures()
    {
        m_GlobalTextures.clear();
    }

    void GfxCommandContext::SetBuffer(const std::string& name, GfxBuffer* value, GfxBufferElement element)
    {
        SetBuffer(ShaderUtils::GetIdFromString(name), value, element);
    }

    void GfxCommandContext::SetBuffer(int32_t id, GfxBuffer* value, GfxBufferElement element)
    {
        m_GlobalBuffers[id] = GlobalBufferData{ value, element };
    }

    void GfxCommandContext::UnsetBuffers()
    {
        m_GlobalBuffers.clear();
    }

    void GfxCommandContext::UnsetTexturesAndBuffers()
    {
        UnsetTextures();
        UnsetBuffers();
    }

    void GfxCommandContext::SetColorTarget(const GfxRenderTargetDesc& colorTarget)
    {
        SetRenderTargets(1, &colorTarget, nullptr);
    }

    void GfxCommandContext::SetDepthStencilTarget(const GfxRenderTargetDesc& depthStencilTarget)
    {
        SetRenderTargets(0, nullptr, &depthStencilTarget);
    }

    void GfxCommandContext::SetRenderTarget(const GfxRenderTargetDesc& colorTarget, const GfxRenderTargetDesc& depthStencilTarget)
    {
        SetRenderTargets(1, &colorTarget, &depthStencilTarget);
    }

    void GfxCommandContext::SetRenderTargets(uint32_t numColorTargets, const GfxRenderTargetDesc* colorTargets)
    {
        SetRenderTargets(numColorTargets, colorTargets, nullptr);
    }

    void GfxCommandContext::SetRenderTargets(uint32_t numColorTargets, const GfxRenderTargetDesc* colorTargets, const GfxRenderTargetDesc& depthStencilTarget)
    {
        SetRenderTargets(numColorTargets, colorTargets, &depthStencilTarget);
    }

    void GfxCommandContext::SetRenderTargets(uint32_t numColorTargets, const GfxRenderTargetDesc* colorTargets, const GfxRenderTargetDesc* depthStencilTarget)
    {
        assert(numColorTargets <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

        if (numColorTargets == 0 && depthStencilTarget == nullptr)
        {
            LOG_WARNING("SetRenderTargets called with zero render target");
            return;
        }

        bool isDirty = false;

        if (m_OutputDesc.NumRTV != numColorTargets)
        {
            isDirty = true;
            m_OutputDesc.NumRTV = numColorTargets;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE rtv[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};

        for (uint32_t i = 0; i < numColorTargets; i++)
        {
            const GfxRenderTargetDesc& desc = colorTargets[i];
            TransitionResource(desc.Texture->GetUnderlyingResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

            rtv[i] = GetRtvDsvFromRenderTargetDesc(desc);
            RenderTargetData rtData{ desc.Texture, rtv[i] };

            if (memcmp(&m_ColorTargets[i], &rtData, sizeof(RenderTargetData)) != 0)
            {
                isDirty = true;
                m_ColorTargets[i] = rtData;
                m_OutputDesc.RTVFormats[i] = desc.Texture->GetDesc().GetRtvDsvDXGIFormat();
                m_OutputDesc.SampleCount = desc.Texture->GetSampleCount();
                m_OutputDesc.SampleQuality = desc.Texture->GetSampleQuality();
            }
        }

        D3D12_CPU_DESCRIPTOR_HANDLE dsv{};

        if (depthStencilTarget != nullptr)
        {
            const GfxRenderTargetDesc& desc = depthStencilTarget[0];
            TransitionResource(desc.Texture->GetUnderlyingResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

            dsv = GetRtvDsvFromRenderTargetDesc(desc);
            RenderTargetData rtData{ desc.Texture, dsv };

            if (memcmp(&m_DepthStencilTarget, &rtData, sizeof(RenderTargetData)) != 0)
            {
                isDirty = true;
                m_DepthStencilTarget = rtData;
                m_OutputDesc.DSVFormat = desc.Texture->GetDesc().GetRtvDsvDXGIFormat();
                m_OutputDesc.SampleCount = desc.Texture->GetSampleCount();
                m_OutputDesc.SampleQuality = desc.Texture->GetSampleQuality();
            }
        }
        else if (m_DepthStencilTarget.Texture != nullptr)
        {
            isDirty = true;
            m_DepthStencilTarget = RenderTargetData{};
            m_OutputDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
        }

        if (isDirty)
        {
            m_OutputDesc.MarkDirty();

            const D3D12_CPU_DESCRIPTOR_HANDLE* pDsv = (depthStencilTarget != nullptr) ? &dsv : nullptr;
            m_CommandList->OMSetRenderTargets(static_cast<UINT>(numColorTargets), rtv, FALSE, pDsv);
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxCommandContext::GetRtvDsvFromRenderTargetDesc(const GfxRenderTargetDesc& desc)
    {
        switch (desc.Texture->GetDesc().Dimension)
        {
        case GfxTextureDimension::Cube:
        case GfxTextureDimension::CubeArray:
            return desc.Texture->GetRtvDsv(desc.Face, 1, desc.WOrArraySlice, desc.MipSlice);
        default:
            return desc.Texture->GetRtvDsv(desc.WOrArraySlice, 1, desc.MipSlice);
        }
    }

    void GfxCommandContext::ClearRenderTargets(GfxClearFlags flags, const float color[4], float depth, uint8_t stencil)
    {
        bool clearColor = (m_OutputDesc.NumRTV > 0 && (flags & GfxClearFlags::Color) == GfxClearFlags::Color);
        D3D12_CLEAR_FLAGS clearDepthStencil = static_cast<D3D12_CLEAR_FLAGS>(0);

        if (m_DepthStencilTarget.Texture != nullptr)
        {
            if ((flags & GfxClearFlags::Depth) == GfxClearFlags::Depth)
            {
                clearDepthStencil |= D3D12_CLEAR_FLAG_DEPTH;
            }

            if ((flags & GfxClearFlags::Stencil) == GfxClearFlags::Stencil)
            {
                clearDepthStencil |= D3D12_CLEAR_FLAG_STENCIL;
            }
        }

        if (clearColor || clearDepthStencil != 0)
        {
            FlushResourceBarriers();

            if (clearColor)
            {
                for (uint32_t i = 0; i < m_OutputDesc.NumRTV; i++)
                {
                    m_CommandList->ClearRenderTargetView(m_ColorTargets[i].RtvDsv, color, 0, nullptr);
                }
            }

            if (clearDepthStencil != 0)
            {
                m_CommandList->ClearDepthStencilView(m_DepthStencilTarget.RtvDsv, clearDepthStencil, depth, static_cast<UINT8>(stencil), 0, nullptr);
            }
        }
    }

    void GfxCommandContext::ClearColorTarget(uint32_t index, const float color[4])
    {
        if (index >= m_OutputDesc.NumRTV)
        {
            LOG_WARNING("Failed to clear color target: index out of range");
            return;
        }

        FlushResourceBarriers();

        m_CommandList->ClearRenderTargetView(m_ColorTargets[index].RtvDsv, color, 0, nullptr);
    }

    void GfxCommandContext::ClearDepthStencilTarget(float depth, uint8_t stencil)
    {
        if (m_DepthStencilTarget.Texture == nullptr)
        {
            LOG_WARNING("Failed to clear depth-stencil target: no depth-stencil target is set");
            return;
        }

        FlushResourceBarriers();

        constexpr D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
        m_CommandList->ClearDepthStencilView(m_DepthStencilTarget.RtvDsv, flags, depth, static_cast<UINT8>(stencil), 0, nullptr);
    }

    void GfxCommandContext::SetViewport(const D3D12_VIEWPORT& viewport)
    {
        SetViewports(1, &viewport);
    }

    void GfxCommandContext::SetViewports(uint32_t numViewports, const D3D12_VIEWPORT* viewports)
    {
        assert(numViewports <= std::size(m_Viewports));

        const size_t sizeInBytes = static_cast<size_t>(numViewports) * sizeof(D3D12_VIEWPORT);

        if (numViewports != m_NumViewports || memcmp(viewports, m_Viewports, sizeInBytes) != 0)
        {
            m_NumViewports = numViewports;
            memcpy(m_Viewports, viewports, sizeInBytes);

            m_CommandList->RSSetViewports(static_cast<UINT>(numViewports), viewports);
        }
    }

    void GfxCommandContext::SetScissorRect(const D3D12_RECT& rect)
    {
        SetScissorRects(1, &rect);
    }

    void GfxCommandContext::SetScissorRects(uint32_t numRects, const D3D12_RECT* rects)
    {
        assert(numRects <= std::size(m_ScissorRects));

        const size_t sizeInBytes = static_cast<size_t>(numRects) * sizeof(D3D12_RECT);

        if (numRects != m_NumScissorRects || memcmp(rects, m_ScissorRects, sizeInBytes) != 0)
        {
            m_NumScissorRects = numRects;
            memcpy(m_ScissorRects, rects, sizeInBytes);

            m_CommandList->RSSetScissorRects(static_cast<UINT>(numRects), rects);
        }
    }

    void GfxCommandContext::SetDefaultViewport()
    {
        GfxTexture* target = GetFirstRenderTarget();

        if (target == nullptr)
        {
            LOG_WARNING("Failed to set default viewport: no render target is set");
            return;
        }

        D3D12_VIEWPORT viewport{};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = static_cast<float>(target->GetDesc().Width);
        viewport.Height = static_cast<float>(target->GetDesc().Height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        SetViewport(viewport);
    }

    void GfxCommandContext::SetDefaultScissorRect()
    {
        GfxTexture* target = GetFirstRenderTarget();

        if (target == nullptr)
        {
            LOG_WARNING("Failed to set default scissor rect: no render target is set");
            return;
        }

        D3D12_RECT rect{};
        rect.left = 0;
        rect.top = 0;
        rect.right = static_cast<LONG>(target->GetDesc().Width);
        rect.bottom = static_cast<LONG>(target->GetDesc().Height);
        SetScissorRect(rect);
    }

    void GfxCommandContext::SetDepthBias(int32_t bias, float slopeScaledBias, float clamp)
    {
        if (m_OutputDesc.DepthBias != bias || m_OutputDesc.SlopeScaledDepthBias != slopeScaledBias || m_OutputDesc.DepthBiasClamp != clamp)
        {
            m_OutputDesc.DepthBias = bias;
            m_OutputDesc.SlopeScaledDepthBias = slopeScaledBias;
            m_OutputDesc.DepthBiasClamp = clamp;
            m_OutputDesc.MarkDirty();
        }
    }

    void GfxCommandContext::SetDefaultDepthBias()
    {
        SetDepthBias(D3D12_DEFAULT_DEPTH_BIAS, D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, D3D12_DEFAULT_DEPTH_BIAS_CLAMP);
    }

    void GfxCommandContext::SetWireframe(bool value)
    {
        if (m_OutputDesc.Wireframe != value)
        {
            m_OutputDesc.Wireframe = value;
            m_OutputDesc.MarkDirty();
        }
    }

    GfxTexture* GfxCommandContext::GetFirstRenderTarget() const
    {
        return m_OutputDesc.NumRTV > 0 ? m_ColorTargets[0].Texture : m_DepthStencilTarget.Texture;
    }

    GfxTexture* GfxCommandContext::FindTexture(int32_t id, GfxTextureElement* pOutElement, uint32_t* pOutUnorderedAccessMipSlice)
    {
        if (auto it = m_GlobalTextures.find(id); it != m_GlobalTextures.end())
        {
            *pOutElement = it->second.Element;
            *pOutUnorderedAccessMipSlice = it->second.UnorderedAccessMipSlice;
            return it->second.Texture;
        }

        return nullptr;
    }

    GfxTexture* GfxCommandContext::FindTexture(int32_t id, Material* material, GfxTextureElement* pOutElement, uint32_t* pOutUnorderedAccessMipSlice)
    {
        if (GfxTexture* texture = nullptr; material->GetTexture(id, &texture))
        {
            *pOutElement = GfxTextureElement::Default;
            *pOutUnorderedAccessMipSlice = 0;
            return texture;
        }

        return FindTexture(id, pOutElement, pOutUnorderedAccessMipSlice);
    }

    GfxBuffer* GfxCommandContext::FindComputeBuffer(int32_t id, bool isConstantBuffer, GfxBufferElement* pOutElement)
    {
        if (auto it = m_GlobalBuffers.find(id); it != m_GlobalBuffers.end())
        {
            GfxBuffer* buffer = it->second.Buffer;

            if (!isConstantBuffer || (isConstantBuffer && buffer->GetDesc().HasAnyUsages(GfxBufferUsages::Constant)))
            {
                *pOutElement = it->second.Element;
                return buffer;
            }
        }

        return nullptr;
    }

    GfxBuffer* GfxCommandContext::FindGraphicsBuffer(int32_t id, bool isConstantBuffer, Material* material, size_t passIndex, GfxBufferElement* pOutElement)
    {
        if (isConstantBuffer)
        {
            if (id == Shader::GetMaterialConstantBufferId())
            {
                *pOutElement = GfxBufferElement::StructuredData;
                return material->GetConstantBuffer(passIndex);
            }
        }
        else
        {
            static int32_t instanceBufferId = ShaderUtils::GetIdFromString("_InstanceBuffer");

            if (id == instanceBufferId)
            {
                *pOutElement = GfxBufferElement::StructuredData;
                return &m_InstanceBuffer;
            }
        }

        return FindComputeBuffer(id, isConstantBuffer, pOutElement);
    }

    void GfxCommandContext::SetGraphicsPipelineParameters(ID3D12PipelineState* pso, Material* material, size_t passIndex)
    {
        if (m_CurrentPipelineState != pso)
        {
            m_CurrentPipelineState = pso;
            m_CommandList->SetPipelineState(pso);
        }

        ShaderPass* pass = material->GetShader()->GetPass(passIndex);

        m_GraphicsViewCache.SetRootSignature(pass->GetRootSignature(material->GetKeywords()));

        m_GraphicsViewCache.SetSrvCbvBuffers([this, material, passIndex](const ShaderParamSrvCbvBuffer& buf, GfxBufferElement* pOutElement) -> GfxBuffer*
        {
            return FindGraphicsBuffer(buf.Id, buf.IsConstantBuffer, material, passIndex, pOutElement);
        });

        m_GraphicsViewCache.SetSrvTexturesAndSamplers([this, material](const ShaderParamSrvTexture& tex, GfxTextureElement* pOutElement) -> GfxTexture*
        {
            uint32_t mipSliceUnused = 0;
            return FindTexture(tex.Id, material, pOutElement, &mipSliceUnused);
        });

        m_GraphicsViewCache.SetUavBuffers([this, material, passIndex](const ShaderParamUavBuffer& buf, GfxBufferElement* pOutElement) -> GfxBuffer*
        {
            return FindGraphicsBuffer(buf.Id, false, material, passIndex, pOutElement);
        });

        m_GraphicsViewCache.SetUavTextures([this, material](const ShaderParamUavTexture& tex, GfxTextureElement* pOutElement, uint32_t* pOutMipSlice) -> GfxTexture*
        {
            return FindTexture(tex.Id, material, pOutElement, pOutMipSlice);
        });

        m_GraphicsViewCache.TransitionResources([this](auto resource, auto state) -> void
        {
            TransitionResource(resource, state);
        });

        m_GraphicsViewCache.Apply(m_CommandList.Get(), &m_ViewHeap, &m_SamplerHeap);

        SetResolvedRenderState(material->GetResolvedRenderState(passIndex));
    }

    void GfxCommandContext::SetComputePipelineParameters(ID3D12PipelineState* pso, ComputeShader* shader, size_t kernelIndex)
    {
        if (m_CurrentPipelineState != pso)
        {
            m_CurrentPipelineState = pso;
            m_CommandList->SetPipelineState(pso);
        }

        m_ComputeViewCache.SetRootSignature(shader->GetRootSignature(kernelIndex));

        m_ComputeViewCache.SetSrvCbvBuffers([this](const ShaderParamSrvCbvBuffer& buf, GfxBufferElement* pOutElement) -> GfxBuffer*
        {
            return FindComputeBuffer(buf.Id, buf.IsConstantBuffer, pOutElement);
        });

        m_ComputeViewCache.SetSrvTexturesAndSamplers([this](const ShaderParamSrvTexture& tex, GfxTextureElement* pOutElement) -> GfxTexture*
        {
            uint32_t mipSliceUnused = 0;
            return FindTexture(tex.Id, pOutElement, &mipSliceUnused);
        });

        m_ComputeViewCache.SetUavBuffers([this](const ShaderParamUavBuffer& buf, GfxBufferElement* pOutElement) -> GfxBuffer*
        {
            return FindComputeBuffer(buf.Id, false, pOutElement);
        });

        m_ComputeViewCache.SetUavTextures([this](const ShaderParamUavTexture& tex, GfxTextureElement* pOutElement, uint32_t* pOutMipSlice) -> GfxTexture*
        {
            return FindTexture(tex.Id, pOutElement, pOutMipSlice);
        });

        m_ComputeViewCache.TransitionResources([this](auto resource, auto state) -> void
        {
            TransitionResource(resource, state);
        });

        m_ComputeViewCache.Apply(m_CommandList.Get(), &m_ViewHeap, &m_SamplerHeap);
    }

    void GfxCommandContext::SetResolvedRenderState(const ShaderPassRenderState& state)
    {
        if (state.StencilState.Enable)
        {
            SetStencilRef(state.StencilState.Ref.Value);
        }
    }

    void GfxCommandContext::SetStencilRef(uint8_t value)
    {
        if (m_CurrentStencilRef != value)
        {
            m_CurrentStencilRef = value;
            m_CommandList->OMSetStencilRef(value);
        }
    }

    void GfxCommandContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY value)
    {
        if (m_CurrentPrimitiveTopology != value)
        {
            m_CurrentPrimitiveTopology = value;
            m_CommandList->IASetPrimitiveTopology(value);
        }
    }

    void GfxCommandContext::SetVertexBuffer(GfxBuffer* buffer)
    {
        TransitionResource(buffer->GetUnderlyingResource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        D3D12_VERTEX_BUFFER_VIEW vbv = buffer->GetVbv();

        if (m_CurrentVertexBuffer.BufferLocation != vbv.BufferLocation ||
            m_CurrentVertexBuffer.SizeInBytes != vbv.SizeInBytes ||
            m_CurrentVertexBuffer.StrideInBytes != vbv.StrideInBytes)
        {
            m_CurrentVertexBuffer = vbv;
            m_CommandList->IASetVertexBuffers(0, 1, &vbv);
        }
    }

    void GfxCommandContext::SetIndexBuffer(GfxBuffer* buffer)
    {
        TransitionResource(buffer->GetUnderlyingResource(), D3D12_RESOURCE_STATE_INDEX_BUFFER);

        D3D12_INDEX_BUFFER_VIEW ibv = buffer->GetIbv();

        if (m_CurrentIndexBuffer.BufferLocation != ibv.BufferLocation ||
            m_CurrentIndexBuffer.SizeInBytes != ibv.SizeInBytes ||
            m_CurrentIndexBuffer.Format != ibv.Format)
        {
            m_CurrentIndexBuffer = ibv;
            m_CommandList->IASetIndexBuffer(&ibv);
        }
    }

    void GfxCommandContext::SetInstanceBufferData(uint32_t numInstances, const InstanceData* instances)
    {
        GfxBufferDesc desc{};
        desc.Stride = sizeof(InstanceData);
        desc.Count = numInstances;
        desc.Usages = GfxBufferUsages::Structured;
        desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        m_InstanceBuffer.SetData(desc, instances);
    }

    void GfxCommandContext::DrawSubMesh(const GfxSubMeshDesc& subMesh, uint32_t instanceCount)
    {
        SetPrimitiveTopology(subMesh.InputDesc.GetPrimitiveTopology());
        SetVertexBuffer(subMesh.VertexBuffer);
        SetIndexBuffer(subMesh.IndexBuffer);
        FlushResourceBarriers();

        m_CommandList->DrawIndexedInstanced(
            static_cast<UINT>(subMesh.SubMesh.IndexCount),
            static_cast<UINT>(instanceCount),
            static_cast<UINT>(subMesh.SubMesh.StartIndexLocation),
            static_cast<INT>(subMesh.SubMesh.BaseVertexLocation),
            0);
    }

    void GfxCommandContext::DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex)
    {
        DrawMesh(geometry, material, shaderPassIndex, MathUtils::Identity4x4());
    }

    void GfxCommandContext::DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex, const XMFLOAT4X4& matrix)
    {
        DrawMesh(GfxMesh::GetGeometry(geometry), 0, material, shaderPassIndex, matrix);
    }

    void GfxCommandContext::DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex)
    {
        DrawMesh(mesh, subMeshIndex, material, shaderPassIndex, MathUtils::Identity4x4());
    }

    void GfxCommandContext::DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex, const XMFLOAT4X4& matrix)
    {
        DrawMesh(mesh->GetSubMeshDesc(subMeshIndex), material, shaderPassIndex, matrix);
    }

    void GfxCommandContext::DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex)
    {
        DrawMesh(subMesh, material, shaderPassIndex, MathUtils::Identity4x4());
    }

    void GfxCommandContext::DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex, const XMFLOAT4X4& matrix)
    {
        // TODO 允许设置上一帧的矩阵
        SetInstanceBufferData(1, &CreateInstanceData(matrix, matrix));

        ID3D12PipelineState* pso = material->GetPSO(shaderPassIndex, subMesh.InputDesc, m_OutputDesc);
        SetGraphicsPipelineParameters(pso, material, shaderPassIndex);

        DrawSubMesh(subMesh, 1);
    }

    // 相同的可以合批，使用 GPU instancing 绘制
    struct DrawCall
    {
        GfxMesh* Mesh;
        uint32_t SubMeshIndex;
        Material* Mat;
        size_t ShaderPassIndex;

        bool operator==(const DrawCall& other) const
        {
            return memcmp(this, &other, sizeof(DrawCall)) == 0;
        }

        struct Hash
        {
            size_t operator()(const DrawCall& drawCall) const
            {
                return std::hash<GfxMesh*>{}(drawCall.Mesh)
                    ^ std::hash<uint32_t>{}(drawCall.SubMeshIndex)
                    ^ std::hash<Material*>{}(drawCall.Mat)
                    ^ std::hash<size_t>{}(drawCall.ShaderPassIndex);
            };
        };
    };

    void GfxCommandContext::DrawMeshRenderers(size_t numRenderers, MeshRenderer* const* renderers, const std::string& lightMode)
    {
        if (numRenderers == 0)
        {
            return;
        }

        std::unordered_map<ID3D12PipelineState*,
            std::unordered_map<DrawCall, std::vector<InstanceData>, DrawCall::Hash>> psoMap{}; // 优化 pso 切换

        for (size_t i = 0; i < numRenderers; i++)
        {
            MeshRenderer* renderer = renderers[i];
            if (!renderer->GetIsActiveAndEnabled() || renderer->Mesh == nullptr || renderer->Materials.empty())
            {
                continue;
            }

            for (uint32_t j = 0; j < renderer->Mesh->GetSubMeshCount(); j++)
            {
                Material* mat = j < renderer->Materials.size() ? renderer->Materials[j] : renderer->Materials.back();
                if (mat == nullptr || mat->GetShader() == nullptr)
                {
                    continue;
                }

                std::optional<size_t> shaderPassIndex = mat->GetShader()->GetFirstPassIndexWithTagValue("LightMode", lightMode);
                if (!shaderPassIndex)
                {
                    continue;
                }

                ID3D12PipelineState* pso = mat->GetPSO(*shaderPassIndex, renderer->Mesh->GetInputDesc(), m_OutputDesc);
                DrawCall dc{ renderer->Mesh, j, mat, *shaderPassIndex };
                psoMap[pso][dc].emplace_back(CreateInstanceData(renderer->GetTransform()->GetLocalToWorldMatrix(), renderer->GetPrevLocalToWorldMatrix()));
            }
        }

        for (auto& [pso, drawCalls] : psoMap)
        {
            for (auto& [dc, instances] : drawCalls)
            {
                uint32_t instanceCount = static_cast<uint32_t>(instances.size());
                SetInstanceBufferData(instanceCount, instances.data());
                SetGraphicsPipelineParameters(pso, dc.Mat, dc.ShaderPassIndex);
                DrawSubMesh(dc.Mesh->GetSubMeshDesc(dc.SubMeshIndex), instanceCount);
            }
        }
    }

    void GfxCommandContext::DispatchCompute(ComputeShader* shader, size_t kernelIndex, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
    {
        SetComputePipelineParameters(shader->GetPSO(kernelIndex), shader, kernelIndex);
        FlushResourceBarriers();

        m_CommandList->Dispatch(
            static_cast<UINT>(threadGroupCountX),
            static_cast<UINT>(threadGroupCountY),
            static_cast<UINT>(threadGroupCountZ));
    }

    void GfxCommandContext::DispatchComputeByThreadCount(ComputeShader* shader, size_t kernelIndex, uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ)
    {
        uint32_t groupSizeX{}, groupSizeY{}, groupSizeZ{};
        shader->GetThreadGroupSize(kernelIndex, &groupSizeX, &groupSizeY, &groupSizeZ);

        uint32_t groupCountX = static_cast<uint32_t>(std::ceil(threadCountX / static_cast<float>(groupSizeX)));
        uint32_t groupCountY = static_cast<uint32_t>(std::ceil(threadCountY / static_cast<float>(groupSizeY)));
        uint32_t groupCountZ = static_cast<uint32_t>(std::ceil(threadCountZ / static_cast<float>(groupSizeZ)));

        DispatchCompute(shader, kernelIndex, groupCountX, groupCountY, groupCountZ);
    }

    GfxCommandContext::InstanceData GfxCommandContext::CreateInstanceData(const XMFLOAT4X4& currMatrix, const XMFLOAT4X4& prevMatrix)
    {
        XMFLOAT4X4 currMatrixIT{};
        XMStoreFloat4x4(&currMatrixIT, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&currMatrix))));
        return InstanceData{ currMatrix, currMatrixIT, prevMatrix };
    }

    void GfxCommandContext::ResolveTexture(GfxTexture* source, GfxTexture* destination)
    {
        TransitionResource(source->GetUnderlyingResource(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        TransitionResource(destination->GetUnderlyingResource(), D3D12_RESOURCE_STATE_RESOLVE_DEST);
        FlushResourceBarriers();

        m_CommandList->ResolveSubresource(
            destination->GetUnderlyingD3DResource(), 0,
            source->GetUnderlyingD3DResource(), 0,
            source->GetDesc().GetResDXGIFormat());
    }

    void GfxCommandContext::CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement)
    {
        uint32_t srcSize = sourceBuffer->GetSizeInBytes(sourceElement);
        uint32_t dstSize = destinationBuffer->GetSizeInBytes(destinationElement);

        if (srcSize != dstSize)
        {
            throw std::invalid_argument("Source and destination buffer sizes do not match");
        }

        return CopyBuffer(sourceBuffer, sourceElement, 0, destinationBuffer, destinationElement, 0, srcSize);
    }

    void GfxCommandContext::CopyBuffer(
        GfxBuffer* sourceBuffer,
        GfxBufferElement sourceElement,
        uint32_t sourceOffsetInBytes,
        GfxBuffer* destinationBuffer,
        GfxBufferElement destinationElement,
        uint32_t destinationOffsetInBytes,
        uint32_t sizeInBytes)
    {
        uint32_t srcSize = sourceBuffer->GetSizeInBytes(sourceElement);
        uint32_t dstSize = destinationBuffer->GetSizeInBytes(destinationElement);

        if (srcSize - sourceOffsetInBytes < sizeInBytes)
        {
            throw std::invalid_argument("Source buffer size is too small");
        }

        if (dstSize - destinationOffsetInBytes < sizeInBytes)
        {
            throw std::invalid_argument("Destination buffer size is too small");
        }

        TransitionResource(sourceBuffer->GetUnderlyingResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
        TransitionResource(destinationBuffer->GetUnderlyingResource(), D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();

        uint32_t srcOffset = sourceBuffer->GetOffsetInBytes(sourceElement) + sourceOffsetInBytes;
        uint32_t dstOffset = destinationBuffer->GetOffsetInBytes(destinationElement) + destinationOffsetInBytes;

        m_CommandList->CopyBufferRegion(
            destinationBuffer->GetUnderlyingD3DResource(),
            static_cast<UINT64>(dstOffset),
            sourceBuffer->GetUnderlyingD3DResource(),
            static_cast<UINT64>(srcOffset),
            static_cast<UINT64>(sizeInBytes));
    }

    void GfxCommandContext::UpdateSubresources(RefCountPtr<GfxResource> destination, uint32_t firstSubresource, uint32_t numSubresources, const D3D12_SUBRESOURCE_DATA* srcData)
    {
        UINT64 tempBufferSize = GetRequiredIntermediateSize(destination->GetD3DResource(), static_cast<UINT>(firstSubresource), static_cast<UINT>(numSubresources));

        GfxBufferDesc tempBufferDesc{};
        tempBufferDesc.Stride = static_cast<uint32_t>(tempBufferSize);
        tempBufferDesc.Count = 1;
        tempBufferDesc.Usages = GfxBufferUsages::Copy;
        tempBufferDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        GfxBuffer tempBuffer{ m_Device, "TempUpdateSubresourcesBuffer", tempBufferDesc };

        TransitionResource(tempBuffer.GetUnderlyingResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
        TransitionResource(destination, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();

        ::UpdateSubresources(
            m_CommandList.Get(),
            destination->GetD3DResource(),
            tempBuffer.GetUnderlyingD3DResource(),
            static_cast<UINT64>(tempBuffer.GetOffsetInBytes(GfxBufferElement::RawData)),
            static_cast<UINT>(firstSubresource),
            static_cast<UINT>(numSubresources),
            srcData);
    }
}
