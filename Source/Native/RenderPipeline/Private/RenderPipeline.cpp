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
#include "Camera.h"
#include "RenderObject.h"
#include "Display.h"
#include "GfxSupportInfo.h"
#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include <vector>
#include <array>
#include <fstream>

using namespace DirectX;

namespace march
{
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

    RenderPipeline::RenderPipeline()
    {
        m_FullScreenTriangleMesh.reset(CreateSimpleGfxMesh(GetGfxDevice()));
        m_FullScreenTriangleMesh->AddFullScreenTriangle();
    }

    void RenderPipeline::Render(Camera* camera, Material* gridGizmoMaterial)
    {
        if (!camera->GetIsActiveAndEnabled())
        {
            return;
        }

        Display* display = camera->GetTargetDisplay();
        XMMATRIX view = camera->LoadViewMatrix();
        XMMATRIX proj = camera->LoadProjectionMatrix();
        XMMATRIX viewProj = XMMatrixMultiply(view, proj);

        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = static_cast<float>(display->GetPixelWidth());
        viewport.Height = static_cast<float>(display->GetPixelHeight());
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = static_cast<LONG>(display->GetPixelWidth());
        scissorRect.bottom = static_cast<LONG>(display->GetPixelHeight());

        PerPassConstants passConsts = {};
        XMStoreFloat4x4(&passConsts.ViewMatrix, view);
        XMStoreFloat4x4(&passConsts.InvViewMatrix, XMMatrixInverse(nullptr, view));
        XMStoreFloat4x4(&passConsts.ProjectionMatrix, proj);
        XMStoreFloat4x4(&passConsts.InvProjectionMatrix, XMMatrixInverse(nullptr, proj));
        XMStoreFloat4x4(&passConsts.ViewProjectionMatrix, viewProj);
        XMStoreFloat4x4(&passConsts.InvViewProjectionMatrix, XMMatrixInverse(nullptr, viewProj));
        XMStoreFloat4(&passConsts.CameraPositionWS, camera->GetTransform()->LoadPosition());

        passConsts.LightCount = static_cast<int>(m_Lights.size());
        for (int i = 0; i < m_Lights.size(); i++)
        {
            if (!m_Lights[i]->GetIsActiveAndEnabled())
            {
                continue;
            }

            m_Lights[i]->FillLightData(passConsts.Lights[i]);
        }

        auto cbPass = GetGfxDevice()->AllocateTransientUploadMemory(sizeof(PerPassConstants), 1, GfxConstantBuffer::Alignment);
        *reinterpret_cast<PerPassConstants*>(cbPass.GetMappedData(0)) = passConsts;

        RenderPipelineDesc rpDesc = {};
        rpDesc.NumRenderTargets = 1;
        rpDesc.RTVFormats[0] = display->GetColorFormat();
        rpDesc.DSVFormat = display->GetDepthStencilFormat();
        rpDesc.Wireframe = camera->GetEnableWireframe();
        rpDesc.SampleDesc.Count = display->GetCurrentMSAASampleCount();
        rpDesc.SampleDesc.Quality = display->GetCurrentMSAAQuality();

        GfxCommandList* cmd = GetGfxDevice()->GetGraphicsCommandList();
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = display->GetColorBuffer()->GetRtvDsvCpuDescriptorHandle();
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = display->GetDepthStencilBuffer()->GetRtvDsvCpuDescriptorHandle();

        cmd->ResourceBarrier(display->GetColorBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);

        // Set the viewport and scissor rect.  This needs to be reset whenever the command list is reset.
        cmd->GetD3D12CommandList()->RSSetViewports(1, &viewport);
        cmd->GetD3D12CommandList()->RSSetScissorRects(1, &scissorRect);

        // Clear the back buffer and depth buffer.
        cmd->GetD3D12CommandList()->ClearRenderTargetView(rtv, Colors::Black, 0, nullptr);
        cmd->GetD3D12CommandList()->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, GfxSupportInfo::GetFarClipPlaneDepth(), 0, 0, nullptr);

        // Specify the buffers we are going to render to.
        cmd->GetD3D12CommandList()->OMSetRenderTargets(1, &rtv, true, &dsv);

        if (!m_RenderObjects.empty())
        {
            auto cbPerObj = GetGfxDevice()->AllocateTransientUploadMemory(sizeof(PerObjConstants), static_cast<uint32_t>(m_RenderObjects.size()), GfxConstantBuffer::Alignment);
            std::unordered_map<size_t, std::vector<int>> objs; // 优化 pso 切换

            for (int i = 0; i < m_RenderObjects.size(); i++)
            {
                PerObjConstants consts = {};
                XMStoreFloat4x4(&consts.WorldMatrix, m_RenderObjects[i]->GetTransform()->LoadLocalToWorldMatrix());
                *reinterpret_cast<PerObjConstants*>(cbPerObj.GetMappedData(i)) = consts;

                if (m_RenderObjects[i]->Mat != nullptr)
                {
                    ShaderPass* pass = m_RenderObjects[i]->Mat->GetShader()->Passes[0].get();
                    size_t hash = HashState(&pass, 1, m_RenderObjects[i]->Mesh->GetDesc().GetHash());
                    objs[hash].push_back(i);
                }
            }

            for (auto& it : objs)
            {
                bool isFirst = true;

                for (int i = 0; i < it.second.size(); i++)
                {
                    int index = it.second[i];
                    RenderObject* obj = m_RenderObjects[index];
                    if (!obj->GetIsActiveAndEnabled() || obj->Mesh == nullptr || obj->Mat == nullptr)
                    {
                        continue;
                    }

                    ShaderPass* pass = obj->Mat->GetShader()->Passes[0].get();

                    if (isFirst)
                    {
                        ID3D12PipelineState* pso = GetGraphicsPipelineState(pass, obj->Mesh->GetDesc(), rpDesc);
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
        }

        if (camera->GetEnableGizmos() && gridGizmoMaterial != nullptr)
        {
            // Scene Grid
            ShaderPass* pass = gridGizmoMaterial->GetShader()->Passes[0].get();
            ID3D12PipelineState* pso = GetGraphicsPipelineState(pass, m_FullScreenTriangleMesh->GetDesc(), rpDesc);
            cmd->GetD3D12CommandList()->SetPipelineState(pso);
            cmd->GetD3D12CommandList()->SetGraphicsRootSignature(pass->GetRootSignature()); // ??? 不是 PSO 里已经有了吗

            if (pass->GetCbvSrvUavCount() > 0)
            {
                GfxDescriptorTable viewTable = GetGfxDevice()->AllocateTransientDescriptorTable(GfxDescriptorTableType::CbvSrvUav, pass->GetCbvSrvUavCount());

                BindCbv(viewTable, pass, "cbPass", cbPass.GetGpuVirtualAddress(0));

                GfxConstantBuffer* cbMat = gridGizmoMaterial->GetConstantBuffer(pass);
                if (cbMat != nullptr)
                {
                    BindCbv(viewTable, pass, "cbMaterial", cbMat->GetGpuVirtualAddress(0));
                }

                cmd->GetD3D12CommandList()->SetGraphicsRootDescriptorTable(pass->GetCbvSrvUavRootParamIndex(),
                    viewTable.GetGpuHandle(0));
            }

            m_FullScreenTriangleMesh->Draw();
        }

        if (display->GetEnableMSAA())
        {
            cmd->ResourceBarrier(display->GetColorBuffer(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            //cmd->ResourceBarrier(display->GetDepthStencilBuffer(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            cmd->ResourceBarrier(display->GetResolvedColorBuffer(), D3D12_RESOURCE_STATE_RESOLVE_DEST);
            //cmd->ResourceBarrier(display->GetResolvedDepthStencilBuffer(), D3D12_RESOURCE_STATE_RESOLVE_DEST);
            cmd->FlushResourceBarriers();

            cmd->GetD3D12CommandList()->ResolveSubresource(display->GetResolvedColorBuffer()->GetD3D12Resource(),
                0, display->GetColorBuffer()->GetD3D12Resource(), 0, display->GetColorFormat());
            /*cmd->GetD3D12CommandList()->ResolveSubresource(display->GetResolvedDepthStencilBuffer()->GetD3D12Resource(),
                0, display->GetDepthStencilBuffer()->GetD3D12Resource(), 0, display->GetDepthStencilFormat());*/

            cmd->ResourceBarrier(display->GetResolvedColorBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ, true);
        }
        else
        {
            cmd->ResourceBarrier(display->GetColorBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ, true);
        }
    }
}
