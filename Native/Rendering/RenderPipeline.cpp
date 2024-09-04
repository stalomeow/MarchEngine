#include "Rendering/RenderPipeline.h"
#include "Rendering/DxException.h"
#include "Rendering/GfxManager.h"
#include "Core/Debug.h"
#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include <vector>
#include <array>
#include <fstream>

using Microsoft::WRL::ComPtr;

namespace dx12demo
{
    RenderPipeline::RenderPipeline(int width, int height)
    {
        CheckMSAAQuailty();
        CreateDescriptorHeaps();
        Resize(width, height);
    }

    void RenderPipeline::CheckMSAAQuailty()
    {
        auto device = GetGfxManager().GetDevice();

        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};
        msQualityLevels.Format = GetGfxManager().GetBackBufferFormat();
        msQualityLevels.SampleCount = m_MSAASampleCount;
        msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        THROW_IF_FAILED(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &msQualityLevels, sizeof(msQualityLevels)));

        m_MSAAQuality = msQualityLevels.NumQualityLevels - 1;
    }

    void RenderPipeline::CreateDescriptorHeaps()
    {
        m_RtvHeap = std::make_unique<DescriptorHeap>(L"Rtv Heap", D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 0, 1, false);
        m_DsvHeap = std::make_unique<DescriptorHeap>(L"Dsv Heap", D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 0, 1, false);
        m_SrvHeap = std::make_unique<DescriptorHeap>(L"Srv Heap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_SamplerHeap = std::make_unique<DescriptorHeap>(L"Sampler Heap", D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 20);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetColorRenderTargetView()
    {
        return m_RtvHeap->GetCpuHandleForFixedDescriptor(0);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetDepthStencilTargetView()
    {
        return m_DsvHeap->GetCpuHandleForFixedDescriptor(0);
    }

    void RenderPipeline::SetEnableMSAA(bool value)
    {
        m_EnableMSAA = value;

        GetGfxManager().WaitForGpuIdle();
        CreateColorAndDepthStencilTarget(m_RenderTargetWidth, m_RenderTargetHeight);
    }

    void RenderPipeline::CreateColorAndDepthStencilTarget(int width, int height)
    {
        auto device = GetGfxManager().GetDevice();

        D3D12_CLEAR_VALUE colorClearValue = {};
        colorClearValue.Format = GetGfxManager().GetBackBufferFormat();
        memcpy(colorClearValue.Color, DirectX::Colors::Black, sizeof(colorClearValue.Color));
        THROW_IF_FAILED(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(GetGfxManager().GetBackBufferFormat(), width, height, 1, 1,
                m_EnableMSAA ? m_MSAASampleCount : 1,
                m_EnableMSAA ? m_MSAAQuality : 0,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
            D3D12_RESOURCE_STATE_COMMON,
            &colorClearValue, IID_PPV_ARGS(&m_ColorTarget)));
        m_LastColorTargetState = D3D12_RESOURCE_STATE_COMMON;
        device->CreateRenderTargetView(m_ColorTarget.Get(), nullptr,
            m_RtvHeap->GetCpuHandleForFixedDescriptor(0));

        D3D12_CLEAR_VALUE dsClearValue = {};
        dsClearValue.Format = m_DepthStencilFormat;
        dsClearValue.DepthStencil.Depth = 1.0f;
        dsClearValue.DepthStencil.Stencil = 0;
        THROW_IF_FAILED(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(m_DepthStencilFormat, width, height, 1, 1,
                m_EnableMSAA ? m_MSAASampleCount : 1,
                m_EnableMSAA ? m_MSAAQuality : 0,
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &dsClearValue, IID_PPV_ARGS(&m_DepthStencilTarget)));
        device->CreateDepthStencilView(m_DepthStencilTarget.Get(), nullptr,
            m_DsvHeap->GetCpuHandleForFixedDescriptor(0));
    }

    void RenderPipeline::Resize(int width, int height)
    {
        GetGfxManager().WaitForGpuIdle();

        width = max(width, 10);
        height = max(height, 10);
        CreateColorAndDepthStencilTarget(width, height);

        THROW_IF_FAILED(GetGfxManager().GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(GetGfxManager().GetBackBufferFormat(), width, height, 1, 1, 1, 0),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr, IID_PPV_ARGS(&m_ResolvedColorTarget)));
        m_LastResolvedColorTargetState = D3D12_RESOURCE_STATE_COMMON;

        m_RenderTargetWidth = width;
        m_RenderTargetHeight = height;

        m_Viewport.TopLeftX = 0;
        m_Viewport.TopLeftY = 0;
        m_Viewport.Width = static_cast<float>(width);
        m_Viewport.Height = static_cast<float>(height);
        m_Viewport.MinDepth = 0.0f;
        m_Viewport.MaxDepth = 1.0f;

        m_ScissorRect = { 0, 0, width, height };
    }

    static void BindRootCbv(ID3D12GraphicsCommandList* cmdList, const ShaderPass& pass,
        const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address)
    {
        UINT cbvIndex = 0;
        if (pass.TryGetRootCbvIndex(name, &cbvIndex))
        {
            cmdList->SetGraphicsRootConstantBufferView(cbvIndex, address);
        }
    }

    void RenderPipeline::Render(CommandBuffer* cmd)
    {
        if (m_RenderObjects.empty())
        {
            return;
        }

        PerPassConstants passConsts = {};

        // Convert Spherical to Cartesian coordinates.
        float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
        float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
        float y = m_Radius * cosf(m_Phi);

        // Build the view matrix.
        DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
        DirectX::XMVECTOR target = DirectX::XMVectorZero();
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
        DirectX::XMStoreFloat4x4(&passConsts.ViewMatrix, view);
        DirectX::XMStoreFloat4x4(&passConsts.InvViewMatrix, DirectX::XMMatrixInverse(&XMMatrixDeterminant(view), view));

        // The window resized, so update the aspect ratio and recompute the projection matrix.
        float asp = static_cast<float>(m_RenderTargetWidth) / static_cast<float>(m_RenderTargetHeight);
        DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, asp, 1.0f, 1000.0f);
        DirectX::XMStoreFloat4x4(&passConsts.ProjectionMatrix, proj);
        DirectX::XMStoreFloat4x4(&passConsts.InvProjectionMatrix, DirectX::XMMatrixInverse(&XMMatrixDeterminant(proj), proj));

        DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);
        DirectX::XMStoreFloat4x4(&passConsts.ViewProjectionMatrix, viewProj);
        DirectX::XMStoreFloat4x4(&passConsts.InvViewProjectionMatrix, DirectX::XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj));

        DirectX::XMStoreFloat4(&passConsts.CameraPositionWS, pos);

        passConsts.LightCount = m_Lights.size();
        for (int i = 0; i < m_Lights.size(); i++)
        {
            if (!m_Lights[i]->IsActive)
            {
                continue;
            }

            m_Lights[i]->FillLightData(passConsts.Lights[i]);
        }

        auto cbPass = cmd->AllocateTempUploadHeap<PerPassConstants>(1, ConstantBufferAlignment);
        cbPass.SetData(0, passConsts);

        auto cbPerObj = cmd->AllocateTempUploadHeap<PerObjConstants>(m_RenderObjects.size(), ConstantBufferAlignment);
        std::unordered_map<size_t, std::vector<int>> objs; // 优化 pso 切换

        for (int i = 0; i < m_RenderObjects.size(); i++)
        {
            PerObjConstants consts = {};
            consts.WorldMatrix = m_RenderObjects[i]->GetWorldMatrix();
            cbPerObj.SetData(i, consts);

            if (m_RenderObjects[i]->Mat != nullptr)
            {
                ShaderPass* pass = &(m_RenderObjects[i]->Mat->GetShader()->Passes[0]);
                size_t hash = HashState(&pass, 1, m_RenderObjects[i]->Desc.GetHash());
                objs[hash].push_back(i);
            }
        }

        RenderPipelineDesc rpDesc = {};
        rpDesc.NumRenderTargets = 1;
        rpDesc.RTVFormats[0] = GetGfxManager().GetBackBufferFormat();
        rpDesc.DSVFormat = m_DepthStencilFormat;
        rpDesc.Wireframe = m_IsWireframe;

        if (m_EnableMSAA)
        {
            rpDesc.SampleDesc.Count = m_MSAASampleCount;
            rpDesc.SampleDesc.Quality = m_MSAAQuality;
        }
        else
        {
            rpDesc.SampleDesc.Count = 1;
            rpDesc.SampleDesc.Quality = 0;
        }

        // Indicate a state transition on the resource usage.
        if (m_LastColorTargetState != D3D12_RESOURCE_STATE_RENDER_TARGET)
        {
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ColorTarget.Get(),
                m_LastColorTargetState, D3D12_RESOURCE_STATE_RENDER_TARGET));
        }

        // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
        cmd->GetList()->RSSetViewports(1, &m_Viewport);
        cmd->GetList()->RSSetScissorRects(1, &m_ScissorRect);

        // Clear the back buffer and depth buffer.
        cmd->GetList()->ClearRenderTargetView(GetColorRenderTargetView(), DirectX::Colors::Black, 0, nullptr);
        cmd->GetList()->ClearDepthStencilView(GetDepthStencilTargetView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        // Specify the buffers we are going to render to.
        cmd->GetList()->OMSetRenderTargets(1, &GetColorRenderTargetView(), true, &GetDepthStencilTargetView());

        for (auto& it : objs)
        {
            for (int i = 0; i < it.second.size(); i++)
            {
                int index = it.second[i];
                RenderObject* obj = m_RenderObjects[index];
                ShaderPass& pass = obj->Mat->GetShader()->Passes[0];

                if (i == 0)
                {
                    ID3D12PipelineState* pso = GetGraphicsPipelineState(pass, obj->Desc, rpDesc);
                    cmd->GetList()->SetPipelineState(pso);

                    // ??? 不是 PSO 里已经有了吗
                    cmd->GetList()->SetGraphicsRootSignature(pass.GetRootSignature());
                    BindRootCbv(cmd->GetList(), pass, "cbPass", cbPass.GetGpuVirtualAddress());

                    ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvHeap->GetHeapPointer(), m_SamplerHeap->GetHeapPointer() };
                    cmd->GetList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
                }

                if (!obj->IsActive || obj->Mesh == nullptr || obj->Mat == nullptr)
                {
                    continue;
                }

                m_SrvHeap->Clear();
                m_SamplerHeap->Clear();

                for (const ShaderPassTextureProperty& texProp : pass.TextureProperties)
                {
                    Texture* texture = nullptr;
                    if (obj->Mat->GetTexture(texProp.Name, &texture))
                    {
                        m_SrvHeap->Append(texture->GetTextureCpuDescriptorHandle());

                        if (texProp.HasSampler)
                        {
                            m_SamplerHeap->Append(texture->GetSamplerCpuDescriptorHandle());
                        }
                    }
                }

                cmd->GetList()->SetGraphicsRootDescriptorTable(pass.GetRootSrvDescriptorTableIndex(),
                    m_SrvHeap->GetGpuHandleForDynamicDescriptor(0));
                cmd->GetList()->SetGraphicsRootDescriptorTable(pass.GetRootSamplerDescriptorTableIndex(),
                    m_SamplerHeap->GetGpuHandleForDynamicDescriptor(0));

                BindRootCbv(cmd->GetList(), pass, "cbObject", cbPerObj.GetGpuVirtualAddress(index));

                ConstantBuffer* cbMat = obj->Mat->GetConstantBuffer(&pass);
                if (cbMat != nullptr)
                {
                    BindRootCbv(cmd->GetList(), pass, "cbMaterial", cbMat->GetGpuVirtualAddress());
                }

                obj->Mesh->Draw(cmd);
            }
        }

        if (m_EnableMSAA)
        {
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ColorTarget.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));

            if (m_LastResolvedColorTargetState != D3D12_RESOURCE_STATE_RESOLVE_DEST)
            {
                cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                    m_LastResolvedColorTargetState, D3D12_RESOURCE_STATE_RESOLVE_DEST));
            }

            cmd->GetList()->ResolveSubresource(m_ResolvedColorTarget.Get(), 0, m_ColorTarget.Get(), 0, GetGfxManager().GetBackBufferFormat());
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

            m_LastColorTargetState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
            m_LastResolvedColorTargetState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        else
        {
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ColorTarget.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

            if (m_LastResolvedColorTargetState != D3D12_RESOURCE_STATE_COPY_DEST)
            {
                cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                    m_LastResolvedColorTargetState, D3D12_RESOURCE_STATE_COPY_DEST));
            }

            cmd->GetList()->CopyResource(m_ResolvedColorTarget.Get(), m_ColorTarget.Get());
            cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ResolvedColorTarget.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

            m_LastColorTargetState = D3D12_RESOURCE_STATE_COPY_SOURCE;
            m_LastResolvedColorTargetState = D3D12_RESOURCE_STATE_GENERIC_READ;
        }
    }
}
