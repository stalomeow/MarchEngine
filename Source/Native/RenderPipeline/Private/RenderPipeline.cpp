#include "RenderPipeline.h"
#include "Debug.h"
#include "GfxTexture.h"
#include "GfxDevice.h"
#include "GfxMesh.h"
#include "GfxDescriptorHeap.h"
#include "GfxExcept.h"
#include "GfxBuffer.h"
#include "GfxCommandList.h"
#include "Transform.h"
#include "Material.h"
#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include <vector>
#include <array>
#include <fstream>

using Microsoft::WRL::ComPtr;

namespace march
{
    RenderPipeline::RenderPipeline(int width, int height)
    {
        CheckMSAAQuailty();
        Resize(width, height);
    }

    void RenderPipeline::CheckMSAAQuailty()
    {
        auto device = GetGfxDevice()->GetD3D12Device();

        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};
        msQualityLevels.Format = GetGfxDevice()->GetBackBufferFormat();
        msQualityLevels.SampleCount = m_MSAASampleCount;
        msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        GFX_HR(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
            &msQualityLevels, sizeof(msQualityLevels)));

        m_MSAAQuality = msQualityLevels.NumQualityLevels - 1;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetColorRenderTargetView() const
    {
        return m_ColorTarget->GetRtvDsvCpuDescriptorHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetColorShaderResourceView() const
    {
        return m_ResolvedColorTarget->GetSrvCpuDescriptorHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RenderPipeline::GetDepthStencilTargetView() const
    {
        return m_DepthStencilTarget->GetRtvDsvCpuDescriptorHandle();
    }

    void RenderPipeline::SetEnableMSAA(bool value)
    {
        m_EnableMSAA = value;

        GetGfxDevice()->WaitForIdle();
        CreateColorAndDepthStencilTarget(m_RenderTargetWidth, m_RenderTargetHeight);
    }

    void RenderPipeline::CreateColorAndDepthStencilTarget(int width, int height)
    {
        GfxDevice* device = GetGfxDevice();

        m_ColorTarget = std::make_unique<GfxRenderTexture>(device, "ColorTarget",
            device->GetBackBufferFormat(), width, height,
            m_EnableMSAA ? m_MSAASampleCount : 1,
            m_EnableMSAA ? m_MSAAQuality : 0);

        m_DepthStencilTarget = std::make_unique<GfxRenderTexture>(device, "DepthStencilTarget",
            m_DepthStencilFormat, width, height,
            m_EnableMSAA ? m_MSAASampleCount : 1,
            m_EnableMSAA ? m_MSAAQuality : 0);
    }

    void RenderPipeline::Resize(int width, int height)
    {
        GetGfxDevice()->WaitForIdle();

        width = max(width, 10);
        height = max(height, 10);

        CreateColorAndDepthStencilTarget(width, height);
        m_ResolvedColorTarget = std::make_unique<GfxRenderTexture>(GetGfxDevice(), "ResolvedColorTarget",
            GetGfxDevice()->GetBackBufferFormat(), width, height);

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

    static void BindCbv(const GfxDescriptorTable& table, const ShaderPass* pass,
        const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address)
    {
        auto it = pass->ConstantBuffers.find(name);

        if (it == pass->ConstantBuffers.end())
        {
            return;
        }

        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = address;
        desc.SizeInBytes = GfxConstantBuffer::GetAlignedSize(it->second.Size);

        auto device = GetGfxDevice()->GetD3D12Device();
        device->CreateConstantBufferView(&desc, table.GetCpuHandle(it->second.DescriptorTableIndex));
    }

    void RenderPipeline::Render()
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
            if (!m_Lights[i]->GetIsActive())
            {
                continue;
            }

            m_Lights[i]->FillLightData(passConsts.Lights[i]);
        }

        auto cbPass = GetGfxDevice()->AllocateTransientUploadMemory(sizeof(PerPassConstants), 1, GfxConstantBuffer::Alignment);
        *reinterpret_cast<PerPassConstants*>(cbPass.GetMappedData(0)) = passConsts;

        auto cbPerObj = GetGfxDevice()->AllocateTransientUploadMemory(sizeof(PerObjConstants), m_RenderObjects.size(), GfxConstantBuffer::Alignment);
        std::unordered_map<size_t, std::vector<int>> objs; // 优化 pso 切换

        for (int i = 0; i < m_RenderObjects.size(); i++)
        {
            PerObjConstants consts = {};
            consts.WorldMatrix = m_RenderObjects[i]->GetTransform()->GetLocalToWorldMatrix();
            *reinterpret_cast<PerObjConstants*>(cbPerObj.GetMappedData(i)) = consts;

            if (m_RenderObjects[i]->Mat != nullptr)
            {
                ShaderPass* pass = m_RenderObjects[i]->Mat->GetShader()->Passes[0].get();
                size_t hash = HashState(&pass, 1, m_RenderObjects[i]->Desc.GetHash());
                objs[hash].push_back(i);
            }
        }

        RenderPipelineDesc rpDesc = {};
        rpDesc.NumRenderTargets = 1;
        rpDesc.RTVFormats[0] = GetGfxDevice()->GetBackBufferFormat();
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

        GfxCommandList* cmd = GetGfxDevice()->GetGraphicsCommandList();

        cmd->ResourceBarrier(m_ColorTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);

        // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
        cmd->GetD3D12CommandList()->RSSetViewports(1, &m_Viewport);
        cmd->GetD3D12CommandList()->RSSetScissorRects(1, &m_ScissorRect);

        // Clear the back buffer and depth buffer.
        cmd->GetD3D12CommandList()->ClearRenderTargetView(GetColorRenderTargetView(), DirectX::Colors::Black, 0, nullptr);
        cmd->GetD3D12CommandList()->ClearDepthStencilView(GetDepthStencilTargetView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

        // Specify the buffers we are going to render to.
        cmd->GetD3D12CommandList()->OMSetRenderTargets(1, &GetColorRenderTargetView(), true, &GetDepthStencilTargetView());

        for (auto& it : objs)
        {
            bool isFirst = true;

            for (int i = 0; i < it.second.size(); i++)
            {
                int index = it.second[i];
                RenderObject* obj = m_RenderObjects[index];
                if (!obj->GetIsActive() || obj->Mesh == nullptr || obj->Mat == nullptr)
                {
                    continue;
                }

                ShaderPass* pass = obj->Mat->GetShader()->Passes[0].get();

                if (isFirst)
                {
                    ID3D12PipelineState* pso = GetGraphicsPipelineState(pass, obj->Desc, rpDesc);
                    cmd->GetD3D12CommandList()->SetPipelineState(pso);
                    cmd->GetD3D12CommandList()->SetGraphicsRootSignature(pass->GetRootSignature()); // ??? 不是 PSO 里已经有了吗
                }

                isFirst = false;

                if (pass->GetCbvSrvUavCount() > 0)
                {
                    GfxDescriptorTable viewTable = GetGfxDevice()->AllocateTransientDescriptorTable(GfxDescriptorTableType::CbvSrvUav, pass->GetCbvSrvUavCount());

                    for (auto& kv : pass->TextureProperties)
                    {
                        GfxTexture* texture = nullptr;
                        if (obj->Mat->GetTexture(kv.first, &texture))
                        {
                            viewTable.Copy(kv.second.TextureDescriptorTableIndex, texture->GetSrvCpuDescriptorHandle());
                        }
                    }

                    BindCbv(viewTable, pass, "cbPass", cbPass.GetGpuVirtualAddress(0));
                    BindCbv(viewTable, pass, "cbObject", cbPerObj.GetGpuVirtualAddress(index));

                    GfxConstantBuffer* cbMat = obj->Mat->GetConstantBuffer(pass);
                    if (cbMat != nullptr)
                    {
                        BindCbv(viewTable, pass, "cbMaterial", cbMat->GetGpuVirtualAddress(0));
                    }

                    cmd->GetD3D12CommandList()->SetGraphicsRootDescriptorTable(pass->GetCbvSrvUavRootParamIndex(),
                        viewTable.GetGpuHandle(0));
                }

                if (pass->GetSamplerCount() > 0)
                {
                    GfxDescriptorTable samplerTable = GetGfxDevice()->AllocateTransientDescriptorTable(GfxDescriptorTableType::Sampler, pass->GetSamplerCount());

                    for (auto& kv : pass->TextureProperties)
                    {
                        GfxTexture* texture = nullptr;
                        if (obj->Mat->GetTexture(kv.first, &texture))
                        {
                            if (kv.second.HasSampler)
                            {
                                samplerTable.Copy(kv.second.SamplerDescriptorTableIndex, texture->GetSamplerCpuDescriptorHandle());
                            }
                        }
                    }

                    cmd->GetD3D12CommandList()->SetGraphicsRootDescriptorTable(pass->GetSamplerRootParamIndex(),
                        samplerTable.GetGpuHandle(0));
                }

                obj->Mesh->Draw();
            }
        }

        if (m_EnableMSAA)
        {
            cmd->ResourceBarrier(m_ColorTarget.get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            cmd->ResourceBarrier(m_ResolvedColorTarget.get(), D3D12_RESOURCE_STATE_RESOLVE_DEST);
            cmd->FlushResourceBarriers();

            cmd->GetD3D12CommandList()->ResolveSubresource(m_ResolvedColorTarget->GetD3D12Resource(), 0, m_ColorTarget->GetD3D12Resource(), 0, GetGfxDevice()->GetBackBufferFormat());
        }
        else
        {
            cmd->ResourceBarrier(m_ColorTarget.get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
            cmd->ResourceBarrier(m_ResolvedColorTarget.get(), D3D12_RESOURCE_STATE_COPY_DEST);
            cmd->FlushResourceBarriers();

            cmd->GetD3D12CommandList()->CopyResource(m_ResolvedColorTarget->GetD3D12Resource(), m_ColorTarget->GetD3D12Resource());
        }

        cmd->ResourceBarrier(m_ResolvedColorTarget.get(), D3D12_RESOURCE_STATE_GENERIC_READ, true);
    }
}
