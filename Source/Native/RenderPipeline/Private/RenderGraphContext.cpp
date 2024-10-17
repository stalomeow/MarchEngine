#include "RenderGraphContext.h"
#include "GfxDevice.h"
#include "GfxCommandList.h"
#include "GfxMesh.h"
#include "GfxDescriptorHeap.h"
#include "GfxBuffer.h"
#include "Shader.h"
#include "Material.h"
#include "RenderObject.h"
#include "Transform.h"
#include <stdexcept>
#include <DirectXMath.h>

using namespace DirectX;

namespace march
{
    RenderTargetClearFlags operator|(RenderTargetClearFlags lhs, RenderTargetClearFlags rhs)
    {
        return static_cast<RenderTargetClearFlags>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    RenderTargetClearFlags& operator|=(RenderTargetClearFlags& lhs, RenderTargetClearFlags rhs)
    {
        return lhs = lhs | rhs;
    }

    RenderTargetClearFlags operator&(RenderTargetClearFlags lhs, RenderTargetClearFlags rhs)
    {
        return static_cast<RenderTargetClearFlags>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }

    RenderTargetClearFlags& operator&=(RenderTargetClearFlags& lhs, RenderTargetClearFlags rhs)
    {
        return lhs = lhs & rhs;
    }

    RenderGraphContext::RenderGraphContext()
        : m_ColorTargets{}
        , m_DepthStencilTarget(nullptr)
        , m_Viewport{}
        , m_ScissorRect{}
        , m_GlobalConstantBuffers{}
    {
    }

    GfxDevice* RenderGraphContext::GetDevice() const
    {
        return GetGfxDevice();
    }

    GfxCommandList* RenderGraphContext::GetGraphicsCommandList() const
    {
        return GetDevice()->GetGraphicsCommandList();
    }

    ID3D12GraphicsCommandList* RenderGraphContext::GetD3D12GraphicsCommandList() const
    {
        return GetGraphicsCommandList()->GetD3D12CommandList();
    }

    RenderPipelineDesc RenderGraphContext::GetRenderPipelineDesc(bool wireframe) const
    {
        RenderPipelineDesc rpDesc = {};

        rpDesc.NumRenderTargets = static_cast<UINT>(m_ColorTargets.size());

        for (size_t i = 0; i < m_ColorTargets.size(); i++)
        {
            rpDesc.RTVFormats[i] = m_ColorTargets[i]->GetFormat();
        }

        rpDesc.DSVFormat = m_DepthStencilTarget != nullptr ? m_DepthStencilTarget->GetFormat() : DXGI_FORMAT_UNKNOWN;
        rpDesc.Wireframe = wireframe;

        GfxRenderTexture* tex = m_ColorTargets.empty() ? m_DepthStencilTarget : m_ColorTargets[0];

        if (tex != nullptr)
        {
            GfxRenderTextureDesc desc = tex->GetDesc();
            rpDesc.SampleDesc.Count = desc.SampleCount;
            rpDesc.SampleDesc.Quality = desc.SampleQuality;
        }
        else
        {
            rpDesc.SampleDesc.Count = 1;
            rpDesc.SampleDesc.Quality = 0;
        }

        return rpDesc;
    }

    void RenderGraphContext::SetGlobalConstantBuffer(const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address)
    {
        SetGlobalConstantBuffer(Shader::GetNameId(name), address);
    }

    void RenderGraphContext::SetGlobalConstantBuffer(int32_t id, D3D12_GPU_VIRTUAL_ADDRESS address)
    {
        m_GlobalConstantBuffers[id] = address;
    }

    void RenderGraphContext::DrawMesh(GfxMesh* mesh, Material* material, bool wireframe,
        int32_t subMeshIndex, int32_t shaderPassIndex,
        size_t numConstantBuffers, int32_t* constantBufferIds, D3D12_GPU_VIRTUAL_ADDRESS* constantBufferAddresses)
    {
        SetPipelineStateAndRootSignature(mesh, material, wireframe, shaderPassIndex);
        BindResources(material, shaderPassIndex, 0, numConstantBuffers, constantBufferIds, constantBufferAddresses);

        if (subMeshIndex == -1)
        {
            mesh->Draw();
        }
        else
        {
            mesh->Draw(subMeshIndex);
        }
    }

    struct PerObjectConstants
    {
        XMFLOAT4X4 WorldMatrix;
    };

    void RenderGraphContext::DrawObjects(size_t numObjects, const RenderObject* const* objects, bool wireframe, int32_t shaderPassIndex,
        size_t numConstantBuffers, const int32_t* constantBufferIds, const D3D12_GPU_VIRTUAL_ADDRESS* constantBufferAddresses)
    {
        if (numObjects <= 0)
        {
            return;
        }

        std::unordered_map<size_t, std::vector<size_t>> psoHashMap{}; // 优化 pso 切换
        uint32_t activeObjectCount = 0;

        for (size_t i = 0; i < numObjects; i++)
        {
            const RenderObject* obj = objects[i];

            if (obj->GetIsActiveAndEnabled() && obj->Mesh != nullptr && obj->Mat != nullptr)
            {
                size_t hash = GetPipelineStateHash(obj->Mesh, obj->Mat, shaderPassIndex);
                psoHashMap[hash].push_back(i);
                activeObjectCount++;
            }
        }

        if (activeObjectCount <= 0)
        {
            return;
        }

        GfxDevice* device = GetDevice();
        auto cbPerObj = device->AllocateTransientUploadMemory(sizeof(PerObjectConstants),
            activeObjectCount, GfxConstantBuffer::Alignment);
        uint32_t cbIndex = 0;

        for (std::pair<const size_t, std::vector<size_t>>& it : psoHashMap)
        {
            bool isFirst = true;

            for (size_t i = 0; i < it.second.size(); i++)
            {
                const RenderObject* obj = objects[it.second[i]];

                if (isFirst)
                {
                    isFirst = false;
                    SetPipelineStateAndRootSignature(obj->Mesh, obj->Mat, wireframe, shaderPassIndex);
                }

                PerObjectConstants& consts = *reinterpret_cast<PerObjectConstants*>(cbPerObj.GetMappedData(cbIndex));
                XMStoreFloat4x4(&consts.WorldMatrix, obj->GetTransform()->LoadLocalToWorldMatrix());
                BindResources(obj->Mat, shaderPassIndex, cbPerObj.GetGpuVirtualAddress(cbIndex),
                    numConstantBuffers, constantBufferIds, constantBufferAddresses);

                obj->Mesh->Draw();
                cbIndex++;
            }
        }
    }

    void RenderGraphContext::SetRenderTargets(size_t numColorTargets, GfxRenderTexture* const* colorTargets,
        GfxRenderTexture* depthStencilTarget, const D3D12_VIEWPORT* viewport, const D3D12_RECT* scissorRect)
    {
        if (numColorTargets <= 0 || numColorTargets > 8)
        {
            throw std::out_of_range("Invalid number of color targets");
        }

        bool isTargetDirty = (numColorTargets != m_ColorTargets.size()) || (depthStencilTarget != m_DepthStencilTarget);

        if (!isTargetDirty)
        {
            for (int i = 0; i < numColorTargets; i++)
            {
                if (colorTargets[i] != m_ColorTargets[i])
                {
                    isTargetDirty = true;
                    break;
                }
            }
        }

        ID3D12GraphicsCommandList* cmd = GetGraphicsCommandList()->GetD3D12CommandList();

        if (isTargetDirty)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtv[8]{}; // 目前最多 8 个
            m_ColorTargets.clear();

            for (int i = 0; i < numColorTargets; i++)
            {
                rtv[i] = colorTargets[i]->GetRtvDsvCpuDescriptorHandle();
                m_ColorTargets.push_back(colorTargets[i]);
            }

            if (depthStencilTarget == nullptr)
            {
                m_DepthStencilTarget = nullptr;
                cmd->OMSetRenderTargets(static_cast<UINT>(numColorTargets), rtv, FALSE, nullptr);
            }
            else
            {
                m_DepthStencilTarget = depthStencilTarget;
                D3D12_CPU_DESCRIPTOR_HANDLE dsv = depthStencilTarget->GetRtvDsvCpuDescriptorHandle();
                cmd->OMSetRenderTargets(static_cast<UINT>(numColorTargets), rtv, FALSE, &dsv);
            }
        }

        const D3D12_VIEWPORT& viewportValue = viewport != nullptr ? *viewport : GetDefaultViewport();
        const D3D12_RECT& scissorRectValue = scissorRect != nullptr ? *scissorRect : GetDefaultScissorRect();

        if (isTargetDirty || memcmp(&viewportValue, &m_Viewport, sizeof(D3D12_VIEWPORT)) != 0)
        {
            m_Viewport = viewportValue;
            cmd->RSSetViewports(1, &m_Viewport);
        }

        if (isTargetDirty || memcmp(&scissorRectValue, &m_ScissorRect, sizeof(D3D12_RECT)) != 0)
        {
            m_ScissorRect = scissorRectValue;
            cmd->RSSetScissorRects(1, &m_ScissorRect);
        }
    }

    void RenderGraphContext::ClearRenderTargets(RenderTargetClearFlags flags, const float color[4], float depth, uint8_t stencil)
    {
        ID3D12GraphicsCommandList* cmd = GetGraphicsCommandList()->GetD3D12CommandList();

        if ((flags & RenderTargetClearFlags::Color) == RenderTargetClearFlags::Color)
        {
            for (GfxRenderTexture* target : m_ColorTargets)
            {
                cmd->ClearRenderTargetView(target->GetRtvDsvCpuDescriptorHandle(), color, 0, nullptr);
            }
        }

        if (m_DepthStencilTarget != nullptr)
        {
            D3D12_CLEAR_FLAGS depthStencilClearFlags{};

            if ((flags & RenderTargetClearFlags::Depth) == RenderTargetClearFlags::Depth)
            {
                depthStencilClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
            }

            if ((flags & RenderTargetClearFlags::Stencil) == RenderTargetClearFlags::Stencil)
            {
                depthStencilClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
            }

            if (depthStencilClearFlags != 0)
            {
                D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_DepthStencilTarget->GetRtvDsvCpuDescriptorHandle();
                cmd->ClearDepthStencilView(dsv, depthStencilClearFlags, depth, static_cast<UINT8>(stencil), 0, nullptr);
            }
        }
    }

    D3D12_VIEWPORT RenderGraphContext::GetDefaultViewport() const
    {
        GfxRenderTextureDesc desc = m_ColorTargets.empty() ? m_DepthStencilTarget->GetDesc() : m_ColorTargets[0]->GetDesc();

        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = static_cast<float>(desc.Width);
        viewport.Height = static_cast<float>(desc.Height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        return viewport;
    }

    D3D12_RECT RenderGraphContext::GetDefaultScissorRect() const
    {
        GfxRenderTextureDesc desc = m_ColorTargets.empty() ? m_DepthStencilTarget->GetDesc() : m_ColorTargets[0]->GetDesc();

        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = static_cast<LONG>(desc.Width);
        scissorRect.bottom = static_cast<LONG>(desc.Height);
        return scissorRect;
    }

    size_t RenderGraphContext::GetPipelineStateHash(GfxMesh* mesh, Material* material, int32_t shaderPassIndex)
    {
        ShaderPass* pass = material->GetShader()->GetPass(shaderPassIndex);
        return HashState(&pass, 1, mesh->GetDesc().GetHash());
    }

    void RenderGraphContext::SetPipelineStateAndRootSignature(GfxMesh* mesh, Material* material, bool wireframe, int32_t shaderPassIndex)
    {
        ShaderPass* pass = material->GetShader()->GetPass(shaderPassIndex);
        ID3D12PipelineState* pso = GetGraphicsPipelineState(pass, mesh->GetDesc(), GetRenderPipelineDesc(wireframe));
        ID3D12GraphicsCommandList* cmd = GetGraphicsCommandList()->GetD3D12CommandList();

        cmd->SetPipelineState(pso);
        cmd->SetGraphicsRootSignature(pass->GetRootSignature()); // TODO ??? 不是 PSO 里已经有了吗
    }

    static void BindConstantBuffer(ID3D12GraphicsCommandList* cmd, const ShaderProgram* program, int32_t id, D3D12_GPU_VIRTUAL_ADDRESS address)
    {
        const auto& cbMap = program->GetConstantBuffers();

        if (auto it = cbMap.find(id); it != cbMap.end())
        {
            cmd->SetGraphicsRootConstantBufferView(it->second.RootParameterIndex, address);
        }
    }

    void RenderGraphContext::BindResources(Material* material, int32_t shaderPassIndex, D3D12_GPU_VIRTUAL_ADDRESS perObjectConstantBufferAddress,
        size_t numExtraConstantBuffers, const int32_t* extraConstantBufferIds, const D3D12_GPU_VIRTUAL_ADDRESS* extraConstantBufferAddresses)
    {
        GfxDevice* device = GetDevice();
        ID3D12GraphicsCommandList* cmd = GetGraphicsCommandList()->GetD3D12CommandList();
        ShaderPass* pass = material->GetShader()->GetPass(shaderPassIndex);

        for (int32_t i = 0; i < static_cast<int32_t>(ShaderProgramType::NumTypes); i++)
        {
            ShaderProgram* program = pass->GetProgram(static_cast<ShaderProgramType>(i));

            if (program == nullptr)
            {
                continue;
            }

            if (perObjectConstantBufferAddress != 0)
            {
                BindConstantBuffer(cmd, program, Shader::GetNameId("cbObject"), perObjectConstantBufferAddress);
            }

            for (size_t i = 0; i < numExtraConstantBuffers; i++)
            {
                BindConstantBuffer(cmd, program, extraConstantBufferIds[i], extraConstantBufferAddresses[i]);
            }

            for (const std::pair<int32_t, D3D12_GPU_VIRTUAL_ADDRESS>& kv : m_GlobalConstantBuffers)
            {
                BindConstantBuffer(cmd, program, kv.first, kv.second);
            }

            if (GfxConstantBuffer* cbMat = material->GetConstantBuffer(pass); cbMat != nullptr)
            {
                BindConstantBuffer(cmd, program, Shader::GetMaterialConstantBufferId(), cbMat->GetGpuVirtualAddress(0));
            }

            uint32_t srvUavCount = static_cast<uint32_t>(program->GetTextures().size());
            uint32_t samplerCount = 0;

            if (srvUavCount > 0)
            {
                GfxDescriptorTable viewTable = device->AllocateTransientDescriptorTable(GfxDescriptorTableType::CbvSrvUav, srvUavCount);

                for (const auto& kv : program->GetTextures())
                {
                    GfxTexture* texture = nullptr;

                    if (material->GetTexture(kv.first, &texture))
                    {
                        viewTable.Copy(kv.second.TextureDescriptorTableIndex, texture->GetSrvCpuDescriptorHandle());

                        if (kv.second.HasSampler)
                        {
                            samplerCount++;
                        }
                    }
                }

                cmd->SetGraphicsRootDescriptorTable(program->GetSrvUavRootParameterIndex(), viewTable.GetGpuHandle(0));
            }

            if (samplerCount > 0)
            {
                GfxDescriptorTable samplerTable = device->AllocateTransientDescriptorTable(GfxDescriptorTableType::Sampler, samplerCount);

                for (const auto& kv : program->GetTextures())
                {
                    GfxTexture* texture = nullptr;

                    if (material->GetTexture(kv.first, &texture))
                    {
                        if (kv.second.HasSampler)
                        {
                            samplerTable.Copy(kv.second.SamplerDescriptorTableIndex, texture->GetSamplerCpuDescriptorHandle());
                        }
                    }
                }

                cmd->SetGraphicsRootDescriptorTable(program->GetSamplerRootParameterIndex(), samplerTable.GetGpuHandle(0));
            }
        }
    }

    void RenderGraphContext::Reset()
    {
        m_ColorTargets.clear();
        m_DepthStencilTarget = nullptr;
        m_GlobalConstantBuffers.clear();
    }
}
