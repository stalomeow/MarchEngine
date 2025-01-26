#include "pch.h"
#include "Engine/Graphics/GfxCommand.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxResource.h"
#include "Engine/Graphics/GfxMesh.h"
#include "Engine/Graphics/Material.h"
#include "Engine/Graphics/MeshRenderer.h"
#include "Engine/Graphics/RenderDoc.h"
#include "Engine/MathUtils.h"
#include "Engine/Debug.h"
#include "Engine/Transform.h"
#include "Engine/StringUtils.h"
#include <assert.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace march
{
    GfxCommandContext::GfxCommandContext(GfxDevice* device, GfxCommandType type)
        : m_Device(device)
        , m_Type(type)
        , m_CommandAllocator(nullptr)
        , m_CommandList(nullptr)
        , m_ResourceBarriers{}
        , m_SyncPointsToWait{}
        , m_GraphicsSrvCbvBufferCache{}
        , m_GraphicsSrvUavCache{}
        , m_GraphicsSamplerCache{}
        , m_GraphicsViewResourceRequiredStates{}
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
        , m_CurrentGraphicsRootSignature(nullptr)
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
            GFX_HR(m_Device->GetD3DDevice4()->CreateCommandList(0, queue->GetType(),
                m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
        }
        else
        {
            GFX_HR(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));
        }
    }

    GfxSyncPoint GfxCommandContext::SubmitAndRelease()
    {
        GfxCommandManager* manager = m_Device->GetCommandManager();
        GfxCommandQueue* queue = manager->GetQueue(m_Type);

        // 准备好所有命令，然后关闭
        FlushResourceBarriers();
        GFX_HR(m_CommandList->Close());

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
        for (auto& cache : m_GraphicsSrvCbvBufferCache) cache.Reset();
        for (auto& cache : m_GraphicsSrvUavCache) cache.Reset();
        for (auto& cache : m_GraphicsSamplerCache) cache.Reset();
        m_GraphicsViewResourceRequiredStates.clear();
        m_ViewHeap = nullptr;
        m_SamplerHeap = nullptr;
        memset(m_ColorTargets, 0, sizeof(m_ColorTargets));
        m_DepthStencilTarget = nullptr;
        m_NumViewports = 0;
        m_NumScissorRects = 0;
        m_OutputDesc = {};
        m_CurrentPipelineState = nullptr;
        m_CurrentGraphicsRootSignature = nullptr;
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
        if (RenderDoc::IsLoaded())
        {
            std::wstring wName = StringUtils::Utf8ToUtf16(name);
            m_CommandList->BeginEvent(0, wName.c_str(), static_cast<UINT>(wName.size() * sizeof(wchar_t))); // 第一个参数貌似是颜色
        }
    }

    void GfxCommandContext::EndEvent()
    {
        if (RenderDoc::IsLoaded())
        {
            m_CommandList->EndEvent();
        }
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

    void GfxCommandContext::SetTexture(const std::string& name, GfxTexture* value, GfxTextureElement element)
    {
        SetTexture(Shader::GetNameId(name), value, element);
    }

    void GfxCommandContext::SetTexture(int32_t id, GfxTexture* value, GfxTextureElement element)
    {
        m_GlobalTextures[id] = std::make_pair(value, element);
    }

    void GfxCommandContext::UnsetTextures()
    {
        m_GlobalTextures.clear();
    }

    void GfxCommandContext::SetBuffer(const std::string& name, GfxBuffer* value, GfxBufferElement element)
    {
        SetBuffer(Shader::GetNameId(name), value, element);
    }

    void GfxCommandContext::SetBuffer(int32_t id, GfxBuffer* value, GfxBufferElement element)
    {
        m_GlobalBuffers[id] = std::make_pair(value, element);
    }

    void GfxCommandContext::UnsetBuffers()
    {
        m_GlobalBuffers.clear();
    }

    void GfxCommandContext::SetRenderTarget(GfxRenderTexture* colorTarget, GfxRenderTexture* depthStencilTarget)
    {
        if (colorTarget == nullptr)
        {
            SetRenderTargets(0, nullptr, depthStencilTarget);
        }
        else
        {
            SetRenderTargets(1, &colorTarget, depthStencilTarget);
        }
    }

    void GfxCommandContext::SetRenderTargets(uint32_t numColorTargets, GfxRenderTexture* const* colorTargets, GfxRenderTexture* depthStencilTarget)
    {
        assert(numColorTargets <= std::size(m_ColorTargets));

        if (numColorTargets == 0 && depthStencilTarget == nullptr)
        {
            LOG_WARNING("SetRenderTargets called with no render targets");
            return;
        }

        // Check if the render targets are dirty
        if (numColorTargets == m_OutputDesc.NumRTV && depthStencilTarget == m_DepthStencilTarget)
        {
            bool isDirty = false;

            for (uint32_t i = 0; i < numColorTargets; i++)
            {
                if (colorTargets[i] != m_ColorTargets[i])
                {
                    isDirty = true;
                    break;
                }
            }

            if (!isDirty)
            {
                return;
            }
        }

        m_OutputDesc.MarkDirty();
        m_OutputDesc.NumRTV = numColorTargets;
        D3D12_CPU_DESCRIPTOR_HANDLE rtv[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};

        for (uint32_t i = 0; i < std::size(m_ColorTargets); i++)
        {
            if (i < numColorTargets)
            {
                GfxTexture* target = colorTargets[i];
                TransitionResource(target->GetUnderlyingResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

                rtv[i] = target->GetRtvDsv();
                m_ColorTargets[i] = target;
                m_OutputDesc.RTVFormats[i] = target->GetDesc().GetRtvDsvDXGIFormat();
                m_OutputDesc.SampleCount = target->GetSampleCount();
                m_OutputDesc.SampleQuality = target->GetSampleQuality();
            }
            else
            {
                m_ColorTargets[i] = nullptr;
                m_OutputDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
            }
        }

        if (m_DepthStencilTarget = depthStencilTarget)
        {
            TransitionResource(depthStencilTarget->GetUnderlyingResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

            m_OutputDesc.DSVFormat = depthStencilTarget->GetDesc().GetRtvDsvDXGIFormat();
            m_OutputDesc.SampleCount = depthStencilTarget->GetSampleCount();
            m_OutputDesc.SampleQuality = depthStencilTarget->GetSampleQuality();

            D3D12_CPU_DESCRIPTOR_HANDLE dsv = depthStencilTarget->GetRtvDsv();
            m_CommandList->OMSetRenderTargets(static_cast<UINT>(numColorTargets), rtv, FALSE, &dsv);
        }
        else
        {
            m_OutputDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
            m_CommandList->OMSetRenderTargets(static_cast<UINT>(numColorTargets), rtv, FALSE, nullptr);
        }
    }

    void GfxCommandContext::ClearRenderTargets(GfxClearFlags flags, const float color[4], float depth, uint8_t stencil)
    {
        bool clearColor = (m_OutputDesc.NumRTV > 0 && (flags & GfxClearFlags::Color) == GfxClearFlags::Color);
        D3D12_CLEAR_FLAGS clearDepthStencil = static_cast<D3D12_CLEAR_FLAGS>(0);

        if (m_DepthStencilTarget != nullptr)
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
                    m_CommandList->ClearRenderTargetView(m_ColorTargets[i]->GetRtvDsv(), color, 0, nullptr);
                }
            }

            if (clearDepthStencil != 0)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_DepthStencilTarget->GetRtvDsv();
                m_CommandList->ClearDepthStencilView(dsv, clearDepthStencil, depth, static_cast<UINT8>(stencil), 0, nullptr);
            }
        }
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
        return m_OutputDesc.NumRTV > 0 ? m_ColorTargets[0] : m_DepthStencilTarget;
    }

    GfxTexture* GfxCommandContext::FindTexture(int32_t id, Material* material, GfxTextureElement* pOutElement)
    {
        if (GfxTexture* texture = nullptr; material->GetTexture(id, &texture))
        {
            *pOutElement = GfxTextureElement::Default;
            return texture;
        }

        if (auto it = m_GlobalTextures.find(id); it != m_GlobalTextures.end())
        {
            *pOutElement = it->second.second;
            return it->second.first;
        }

        return nullptr;
    }

    GfxBuffer* GfxCommandContext::FindBuffer(int32_t id, bool isConstantBuffer, Material* material, size_t passIndex, GfxBufferElement* pOutElement)
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
            static int32_t instanceBufferId = Shader::GetNameId("_InstanceBuffer");

            if (id == instanceBufferId)
            {
                *pOutElement = GfxBufferElement::StructuredData;
                return &m_InstanceBuffer;
            }
        }

        if (auto it = m_GlobalBuffers.find(id); it != m_GlobalBuffers.end())
        {
            *pOutElement = it->second.second;
            return it->second.first;
        }

        return nullptr;
    }

    ID3D12PipelineState* GfxCommandContext::GetGraphicsPipelineState(const GfxInputDesc& inputDesc, Material* material, size_t passIndex)
    {
        return GfxPipelineState::GetGraphicsPSO(material, passIndex, inputDesc, m_OutputDesc);
    }

    void GfxCommandContext::SetGraphicsSrvCbvBuffer(ShaderProgramType type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element, bool isConstantBuffer)
    {
        D3D12_GPU_VIRTUAL_ADDRESS address = buffer->GetGpuVirtualAddress(element);
        m_GraphicsSrvCbvBufferCache[static_cast<size_t>(type)].Set(static_cast<size_t>(index), address, isConstantBuffer);

        D3D12_RESOURCE_STATES state;

        if (isConstantBuffer)
        {
            state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
        else if (type == ShaderProgramType::Pixel)
        {
            state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
        else
        {
            state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        }

        // 记录状态，之后会统一使用 ResourceBarrier
        m_GraphicsViewResourceRequiredStates[buffer->GetUnderlyingResource()] |= state;
    }

    void GfxCommandContext::SetGraphicsSrvTexture(ShaderProgramType type, uint32_t index, GfxTexture* texture, GfxTextureElement element)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetSrv(element);
        m_GraphicsSrvUavCache[static_cast<size_t>(type)].Set(static_cast<size_t>(index), offlineDescriptor);

        D3D12_RESOURCE_STATES state;

        if (type == ShaderProgramType::Pixel)
        {
            state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        }
        else
        {
            state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        }

        // 记录状态，之后会统一使用 ResourceBarrier
        m_GraphicsViewResourceRequiredStates[texture->GetUnderlyingResource()] |= state;
    }

    void GfxCommandContext::SetGraphicsUavBuffer(ShaderProgramType type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = buffer->GetUav(element);
        m_GraphicsSrvUavCache[static_cast<size_t>(type)].Set(static_cast<size_t>(index), offlineDescriptor);

        // 记录状态，之后会统一使用 ResourceBarrier
        m_GraphicsViewResourceRequiredStates[buffer->GetUnderlyingResource()] |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    void GfxCommandContext::SetGraphicsUavTexture(ShaderProgramType type, uint32_t index, GfxTexture* texture, GfxTextureElement element)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetUav(element);
        m_GraphicsSrvUavCache[static_cast<size_t>(type)].Set(static_cast<size_t>(index), offlineDescriptor);

        // 记录状态，之后会统一使用 ResourceBarrier
        m_GraphicsViewResourceRequiredStates[texture->GetUnderlyingResource()] |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    void GfxCommandContext::SetGraphicsSampler(ShaderProgramType type, uint32_t index, GfxTexture* texture)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetSampler();
        m_GraphicsSamplerCache[static_cast<size_t>(type)].Set(static_cast<size_t>(index), offlineDescriptor);
    }

    void GfxCommandContext::SetGraphicsPipelineParameters(ID3D12PipelineState* pso, Material* material, size_t passIndex)
    {
        if (m_CurrentPipelineState != pso)
        {
            m_CurrentPipelineState = pso;
            m_CommandList->SetPipelineState(pso);
        }

        ShaderPass* pass = material->GetShader()->GetPass(passIndex);
        Shader::RootSignatureType* rootSignature = pass->GetRootSignature(material->GetKeywords());

        // Shader::RootSignatureType 本身不复用，但内部的 ID3D12RootSignature 是复用的
        // 如果 ID3D12RootSignature 变了，说明根签名发生了结构上的变化
        if (m_CurrentGraphicsRootSignature != rootSignature->GetD3DRootSignature())
        {
            // 删掉旧的 view
            for (auto& cache : m_GraphicsSrvCbvBufferCache) cache.Reset();
            for (auto& cache : m_GraphicsSrvUavCache) cache.Reset();
            for (auto& cache : m_GraphicsSamplerCache) cache.Reset();
            m_GraphicsViewResourceRequiredStates.clear();

            // 设置 root signature
            m_CurrentGraphicsRootSignature = rootSignature->GetD3DRootSignature();
            m_CommandList->SetGraphicsRootSignature(m_CurrentGraphicsRootSignature);
        }

        for (int32_t i = 0; i < Shader::NumProgramTypes; i++)
        {
            ShaderProgramType programType = static_cast<ShaderProgramType>(i);

            for (const GfxRootSignatureBufferBinding& buf : rootSignature->GetSrvCbvBufferRootParamIndices(programType))
            {
                GfxBufferElement element = GfxBufferElement::StructuredData;

                if (GfxBuffer* buffer = FindBuffer(buf.Id, buf.IsConstantBuffer, material, passIndex, &element))
                {
                    SetGraphicsSrvCbvBuffer(programType, buf.BindPoint, buffer, element, buf.IsConstantBuffer);
                }
            }

            for (const GfxRootSignatureTextureBinding& tex : rootSignature->GetSrvTextureTableSlots(programType))
            {
                GfxTextureElement element = GfxTextureElement::Default;

                if (GfxTexture* texture = FindTexture(tex.Id, material, &element))
                {
                    SetGraphicsSrvTexture(programType, tex.BindPointTexture, texture, element);

                    if (tex.BindPointSampler.has_value())
                    {
                        SetGraphicsSampler(programType, *tex.BindPointSampler, texture);
                    }
                }
            }

            for (const GfxRootSignatureUavBinding& buf : rootSignature->GetUavBufferTableSlots(programType))
            {
                GfxBufferElement element = GfxBufferElement::StructuredData;

                if (GfxBuffer* buffer = FindBuffer(buf.Id, false, material, passIndex, &element))
                {
                    SetGraphicsUavBuffer(programType, buf.BindPoint, buffer, element);
                }
            }

            for (const GfxRootSignatureUavBinding& tex : rootSignature->GetUavTextureTableSlots(programType))
            {
                GfxTextureElement element = GfxTextureElement::Default;

                if (GfxTexture* texture = FindTexture(tex.Id, material, &element))
                {
                    SetGraphicsUavTexture(programType, tex.BindPoint, texture, element);
                }
            }
        }

        TransitionGraphicsViewResources();
        SetGraphicsRootDescriptorTablesAndHeaps(rootSignature);
        SetGraphicsRootSrvCbvBuffers();
        SetResolvedRenderState(material->GetResolvedRenderState(passIndex));
    }

    void GfxCommandContext::SetGraphicsRootDescriptorTablesAndHeaps(Shader::RootSignatureType* rootSignature)
    {
        // ------------------------------------------------------------
        // SRV & UAV
        // ------------------------------------------------------------

        D3D12_GPU_DESCRIPTOR_HANDLE srvUavTables[Shader::NumProgramTypes]{};
        const D3D12_CPU_DESCRIPTOR_HANDLE* offlineSrvUav[Shader::NumProgramTypes]{};
        uint32_t numSrvUav[Shader::NumProgramTypes]{};

        GfxOnlineDescriptorMultiAllocator* viewAllocator = m_Device->GetOnlineViewDescriptorAllocator();
        GfxDescriptorHeap* viewHeap = nullptr;
        bool hasSrvUav = false;

        for (uint32_t numTry = 0; numTry < 2; numTry++)
        {
            uint32_t totalNumSrvUav = 0;

            for (int32_t i = 0; i < Shader::NumProgramTypes; i++)
            {
                ShaderProgramType programType = static_cast<ShaderProgramType>(i);
                std::optional<uint32_t> srvUavTableRootParamIndex = rootSignature->GetSrvUavTableRootParamIndex(programType);
                const auto& srvUavCache = m_GraphicsSrvUavCache[i];

                if (srvUavTableRootParamIndex.has_value() && srvUavCache.IsDirty() && !srvUavCache.IsEmpty())
                {
                    offlineSrvUav[i] = srvUavCache.GetDescriptors();
                    numSrvUav[i] = static_cast<uint32_t>(srvUavCache.GetNum());
                }
                else
                {
                    offlineSrvUav[i] = nullptr;
                    numSrvUav[i] = 0;
                }

                totalNumSrvUav += numSrvUav[i];
            }

            if (totalNumSrvUav > 0)
            {
                if (viewAllocator->AllocateMany(std::size(srvUavTables), offlineSrvUav, numSrvUav, srvUavTables, &viewHeap))
                {
                    hasSrvUav = true;
                    break;
                }

                // 当前的 heap 不够，切换 heap
                viewAllocator->Rollover();

                // 因为 heap 变了，所以 table 要全部重新分配
                for (auto& cache : m_GraphicsSrvUavCache)
                {
                    cache.SetDirty(true);
                }
            }
            else
            {
                // 没有 SRV & UAV，不需要分配
                break;
            }
        }

        // ------------------------------------------------------------
        // SAMPLER
        // ------------------------------------------------------------

        D3D12_GPU_DESCRIPTOR_HANDLE samplerTables[Shader::NumProgramTypes]{};
        const D3D12_CPU_DESCRIPTOR_HANDLE* offlineSamplers[Shader::NumProgramTypes]{};
        uint32_t numSamplers[Shader::NumProgramTypes]{};

        GfxOnlineDescriptorMultiAllocator* samplerAllocator = m_Device->GetOnlineSamplerDescriptorAllocator();
        GfxDescriptorHeap* samplerHeap = nullptr;
        bool hasSampler = false;

        for (uint32_t numTry = 0; numTry < 2; numTry++)
        {
            uint32_t totalNumSamplers = 0;

            for (int32_t i = 0; i < Shader::NumProgramTypes; i++)
            {
                ShaderProgramType programType = static_cast<ShaderProgramType>(i);
                std::optional<uint32_t> samplerTableRootParamIndex = rootSignature->GetSamplerTableRootParamIndex(programType);
                const auto& samplerCache = m_GraphicsSamplerCache[i];

                if (samplerTableRootParamIndex.has_value() && samplerCache.IsDirty() && !samplerCache.IsEmpty())
                {
                    offlineSamplers[i] = samplerCache.GetDescriptors();
                    numSamplers[i] = static_cast<uint32_t>(samplerCache.GetNum());
                }
                else
                {
                    offlineSamplers[i] = nullptr;
                    numSamplers[i] = 0;
                }

                totalNumSamplers += numSamplers[i];
            }

            if (totalNumSamplers > 0)
            {
                if (samplerAllocator->AllocateMany(std::size(samplerTables), offlineSamplers, numSamplers, samplerTables, &samplerHeap))
                {
                    hasSampler = true;
                    break;
                }

                // 当前的 heap 不够，切换 heap
                samplerAllocator->Rollover();

                // 因为 heap 变了，所以 table 要全部重新分配
                for (auto& cache : m_GraphicsSamplerCache)
                {
                    cache.SetDirty(true);
                }
            }
            else
            {
                // 没有 SAMPLER，不需要分配
                break;
            }
        }

        // ------------------------------------------------------------
        // Apply
        // ------------------------------------------------------------

        if (!hasSrvUav && !hasSampler)
        {
            return;
        }

        bool isHeapChanged = false;

        if (hasSrvUav && viewHeap != m_ViewHeap)
        {
            m_ViewHeap = viewHeap;
            isHeapChanged = true;
        }

        if (hasSampler && samplerHeap != m_SamplerHeap)
        {
            m_SamplerHeap = samplerHeap;
            isHeapChanged = true;
        }

        if (isHeapChanged)
        {
            SetDescriptorHeaps();
        }

        for (int32_t i = 0; i < Shader::NumProgramTypes; i++)
        {
            ShaderProgramType programType = static_cast<ShaderProgramType>(i);

            if (hasSrvUav && numSrvUav[i] > 0)
            {
                uint32_t rootParamIndex = *rootSignature->GetSrvUavTableRootParamIndex(programType);
                m_CommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(rootParamIndex), srvUavTables[i]);
            }

            if (hasSampler && numSamplers[i] > 0)
            {
                uint32_t rootParamIndex = *rootSignature->GetSamplerTableRootParamIndex(programType);
                m_CommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(rootParamIndex), samplerTables[i]);
            }
        }

        if (hasSrvUav)
        {
            for (auto& cache : m_GraphicsSrvUavCache)
            {
                cache.SetDirty(false);
            }
        }

        if (hasSampler)
        {
            for (auto& cache : m_GraphicsSamplerCache)
            {
                cache.SetDirty(false);
            }
        }
    }

    void GfxCommandContext::SetGraphicsRootSrvCbvBuffers()
    {
        for (auto& cache : m_GraphicsSrvCbvBufferCache)
        {
            for (size_t i = 0; i < cache.GetNum(); i++)
            {
                if (!cache.IsDirty(i))
                {
                    continue;
                }

                bool isConstantBuffer = false;
                D3D12_GPU_VIRTUAL_ADDRESS address = cache.Get(i, &isConstantBuffer);

                if (isConstantBuffer)
                {
                    m_CommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(i), address);
                }
                else
                {
                    m_CommandList->SetGraphicsRootShaderResourceView(static_cast<UINT>(i), address);
                }
            }

            cache.Apply();
        }
    }

    void GfxCommandContext::TransitionGraphicsViewResources()
    {
        for (const auto& [resource, state] : m_GraphicsViewResourceRequiredStates)
        {
            TransitionResource(resource, state);
        }

        m_GraphicsViewResourceRequiredStates.clear();
    }

    void GfxCommandContext::SetDescriptorHeaps()
    {
        if (m_ViewHeap != nullptr && m_SamplerHeap != nullptr)
        {
            ID3D12DescriptorHeap* heaps[] = { m_ViewHeap->GetD3DDescriptorHeap(), m_SamplerHeap->GetD3DDescriptorHeap() };
            m_CommandList->SetDescriptorHeaps(2, heaps);
        }
        else if (m_ViewHeap != nullptr)
        {
            ID3D12DescriptorHeap* heaps[] = { m_ViewHeap->GetD3DDescriptorHeap() };
            m_CommandList->SetDescriptorHeaps(1, heaps);
        }
        else if (m_SamplerHeap != nullptr)
        {
            ID3D12DescriptorHeap* heaps[] = { m_SamplerHeap->GetD3DDescriptorHeap() };
            m_CommandList->SetDescriptorHeaps(1, heaps);
        }
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
        SetInstanceBufferData(1, &CreateInstanceData(matrix));

        ID3D12PipelineState* pso = GetGraphicsPipelineState(subMesh.InputDesc, material, shaderPassIndex);
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

                ID3D12PipelineState* pso = GetGraphicsPipelineState(renderer->Mesh->GetInputDesc(), mat, *shaderPassIndex);
                DrawCall dc{ renderer->Mesh, j, mat, *shaderPassIndex };
                psoMap[pso][dc].emplace_back(CreateInstanceData(renderer->GetTransform()->GetLocalToWorldMatrix()));
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

    GfxCommandContext::InstanceData GfxCommandContext::CreateInstanceData(const XMFLOAT4X4& matrix)
    {
        XMFLOAT4X4 matrixIT{};
        XMStoreFloat4x4(&matrixIT, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&matrix))));
        return InstanceData{ matrix, matrixIT };
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
