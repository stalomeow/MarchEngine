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
        , m_StateDesc{}
        , m_StateDescHash{}
        , m_IsStateDescDirty(true)
        , m_CurrentPipelineState(nullptr)
        , m_CurrentRootSignature(nullptr)
        , m_CurrentPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
        , m_CurrentStencilRef(0)
        , m_IsStencilRefSet(false)
        , m_GlobalConstantBuffers{}
        , m_PassTextures{}
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

    void RenderGraphContext::SetGlobalConstantBuffer(const std::string& name, D3D12_GPU_VIRTUAL_ADDRESS address)
    {
        SetGlobalConstantBuffer(Shader::GetNameId(name), address);
    }

    void RenderGraphContext::SetGlobalConstantBuffer(int32_t id, D3D12_GPU_VIRTUAL_ADDRESS address)
    {
        m_GlobalConstantBuffers[id] = address;
    }

    void RenderGraphContext::SetTexture(const std::string& name, GfxTexture* texture)
    {
        SetTexture(Shader::GetNameId(name), texture);
    }

    void RenderGraphContext::SetTexture(int32_t id, GfxTexture* texture)
    {
        m_PassTextures[id] = texture;
    }

    struct PerObjectConstants
    {
        XMFLOAT4X4 WorldMatrix;
    };

    D3D12_VERTEX_BUFFER_VIEW RenderGraphContext::CreateTransientVertexBuffer(size_t vertexCount, size_t vertexStride, size_t vertexAlignment, const void* verticesData)
    {
        GfxDevice* device = GetDevice();
        GfxUploadMemory m = device->AllocateTransientUploadMemory(static_cast<uint32_t>(vertexStride), static_cast<uint32_t>(vertexCount));

        memcpy(m.GetMappedData(0), verticesData, m.GetSize());

        D3D12_VERTEX_BUFFER_VIEW view = {};
        view.BufferLocation = m.GetGpuVirtualAddress(0);
        view.SizeInBytes = static_cast<UINT>(m.GetSize());
        view.StrideInBytes = static_cast<UINT>(m.GetStride());
        return view;
    }

    D3D12_INDEX_BUFFER_VIEW RenderGraphContext::CreateTransientIndexBuffer(size_t indexCount, const uint16_t* indexData)
    {
        GfxDevice* device = GetDevice();
        GfxUploadMemory m = device->AllocateTransientUploadMemory(sizeof(uint16_t), static_cast<uint32_t>(indexCount));

        size_t sizeInBytes = indexCount * sizeof(uint16_t);
        memcpy(m.GetMappedData(0), indexData, sizeInBytes);

        D3D12_INDEX_BUFFER_VIEW view = {};
        view.BufferLocation = m.GetGpuVirtualAddress(0);
        view.SizeInBytes = static_cast<UINT>(sizeInBytes);
        view.Format = DXGI_FORMAT_R16_UINT;
        return view;
    }

    D3D12_INDEX_BUFFER_VIEW RenderGraphContext::CreateTransientIndexBuffer(size_t indexCount, const uint32_t* indexData)
    {
        GfxDevice* device = GetDevice();
        GfxUploadMemory m = device->AllocateTransientUploadMemory(sizeof(uint32_t), static_cast<uint32_t>(indexCount));

        size_t sizeInBytes = indexCount * sizeof(uint32_t);
        memcpy(m.GetMappedData(0), indexData, sizeInBytes);

        D3D12_INDEX_BUFFER_VIEW view = {};
        view.BufferLocation = m.GetGpuVirtualAddress(0);
        view.SizeInBytes = static_cast<UINT>(sizeInBytes);
        view.Format = DXGI_FORMAT_R32_UINT;
        return view;
    }

    static void DrawSubMeshes(ID3D12GraphicsCommandList* cmd, D3D12_PRIMITIVE_TOPOLOGY& currentPrimitiveTopology, GfxMesh* mesh, int32_t subMeshIndex)
    {
        D3D12_PRIMITIVE_TOPOLOGY topology = mesh->GetPrimitiveTopology();
        if (topology != currentPrimitiveTopology)
        {
            cmd->IASetPrimitiveTopology(topology);
            currentPrimitiveTopology = topology;
        }

        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        D3D12_INDEX_BUFFER_VIEW ibv = {};
        mesh->GetBufferViews(vbv, ibv);
        cmd->IASetVertexBuffers(0, 1, &vbv);
        cmd->IASetIndexBuffer(&ibv);

        if (subMeshIndex == -1)
        {
            for (uint32_t i = 0; i < mesh->GetSubMeshCount(); i++)
            {
                const GfxSubMesh& subMesh = mesh->GetSubMesh(i);
                cmd->DrawIndexedInstanced(static_cast<UINT>(subMesh.IndexCount), 1,
                    static_cast<UINT>(subMesh.StartIndexLocation), static_cast<INT>(subMesh.BaseVertexLocation), 0);
            }
        }
        else
        {
            const GfxSubMesh& subMesh = mesh->GetSubMesh(subMeshIndex);
            cmd->DrawIndexedInstanced(static_cast<UINT>(subMesh.IndexCount), 1,
                static_cast<UINT>(subMesh.StartIndexLocation), static_cast<INT>(subMesh.BaseVertexLocation), 0);
        }
    }

    void RenderGraphContext::DrawMesh(GfxMesh* mesh, Material* material, int32_t shaderPassIndex, int32_t subMeshIndex)
    {
        int32_t inputDescId = mesh->GetPipelineInputDescId();
        ShaderPass* pass = material->GetShader()->GetPass(shaderPassIndex);
        ID3D12PipelineState* pso = GetPipelineState(inputDescId, pass);

        SetPipelineStateAndRootSignature(pso, pass);
        BindResources(material, shaderPassIndex, 0);
        DrawSubMeshes(GetD3D12GraphicsCommandList(), m_CurrentPrimitiveTopology, mesh, subMeshIndex);
    }

    void RenderGraphContext::DrawMesh(const MeshDesc* meshDesc, Material* material, int32_t shaderPassIndex)
    {
        int32_t inputDescId = meshDesc->InputDescId;
        ShaderPass* pass = material->GetShader()->GetPass(shaderPassIndex);
        ID3D12PipelineState* pso = GetPipelineState(inputDescId, pass);

        SetPipelineStateAndRootSignature(pso, pass);
        BindResources(material, shaderPassIndex, 0);

        ID3D12GraphicsCommandList* cmd = GetD3D12GraphicsCommandList();

        D3D12_PRIMITIVE_TOPOLOGY topology = Shader::GetPipelineInputDescPrimitiveTopology(inputDescId);
        if (topology != m_CurrentPrimitiveTopology)
        {
            cmd->IASetPrimitiveTopology(topology);
            m_CurrentPrimitiveTopology = topology;
        }

        cmd->IASetVertexBuffers(0, 1, &meshDesc->VertexBufferView);
        cmd->IASetIndexBuffer(&meshDesc->IndexBufferView);

        UINT indexStride;

        switch (meshDesc->IndexBufferView.Format)
        {
        case DXGI_FORMAT_R16_UINT:
            indexStride = 2;
            break;

        case DXGI_FORMAT_R32_UINT:
            indexStride = 4;
            break;

        default:
            throw std::runtime_error("Invalid index buffer format");
        }

        UINT indexCount = meshDesc->IndexBufferView.SizeInBytes / indexStride;
        cmd->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }

    void RenderGraphContext::DrawObjects(size_t numObjects, const RenderObject* const* objects, const std::vector<int32_t>& passIndices)
    {
        if (numObjects <= 0)
        {
            return;
        }

        std::unordered_map<ID3D12PipelineState*, std::vector<size_t>> psoMap{}; // 优化 pso 切换
        uint32_t activeObjectCount = 0;

        for (size_t i = 0; i < numObjects; i++)
        {
            const RenderObject* obj = objects[i];

            if (obj->GetIsActiveAndEnabled() && obj->Mesh != nullptr && obj->Mat != nullptr)
            {
                int32_t shaderPassIndex = passIndices[i];

                if (shaderPassIndex < 0)
                {
                    continue;
                }

                ShaderPass* pass = obj->Mat->GetShader()->GetPass(shaderPassIndex);
                ID3D12PipelineState* pso = GetPipelineState(obj->Mesh->GetPipelineInputDescId(), pass);
                psoMap[pso].push_back(i);
                activeObjectCount++;
            }
        }

        if (activeObjectCount <= 0)
        {
            return;
        }

        GfxDevice* device = GetDevice();
        auto cbPerObj = device->AllocateTransientUploadMemory(sizeof(PerObjectConstants), activeObjectCount, GfxConstantBuffer::Alignment);
        uint32_t cbIndex = 0;

        for (auto& [pso, objIndices] : psoMap)
        {
            for (size_t i = 0; i < objIndices.size(); i++)
            {
                const RenderObject* obj = objects[objIndices[i]];
                int32_t shaderPassIndex = passIndices[objIndices[i]];
                ShaderPass* pass = obj->Mat->GetShader()->GetPass(shaderPassIndex);

                PerObjectConstants& consts = *reinterpret_cast<PerObjectConstants*>(cbPerObj.GetMappedData(cbIndex));
                XMStoreFloat4x4(&consts.WorldMatrix, obj->GetTransform()->LoadLocalToWorldMatrix());

                SetPipelineStateAndRootSignature(pso, pass);
                BindResources(obj->Mat, shaderPassIndex, cbPerObj.GetGpuVirtualAddress(cbIndex));
                DrawSubMeshes(GetD3D12GraphicsCommandList(), m_CurrentPrimitiveTopology, obj->Mesh, -1);

                cbIndex++;
            }
        }
    }

    void RenderGraphContext::DrawObjects(size_t numObjects, const RenderObject* const* objects, int32_t shaderPassIndex)
    {
        std::vector<int32_t> passIndices(numObjects, shaderPassIndex);
        DrawObjects(numObjects, objects, passIndices);
    }

    void RenderGraphContext::DrawMesh(GfxMesh* mesh, Material* material, const std::string& lightMode, int32_t subMeshIndex)
    {
        int32_t passIndex = material->GetShader()->GetFirstPassIndexWithTagValue("LightMode", lightMode);

        if (passIndex < 0)
        {
            return;
        }

        DrawMesh(mesh, material, passIndex, subMeshIndex);
    }

    void RenderGraphContext::DrawMesh(const MeshDesc* meshDesc, Material* material, const std::string& lightMode)
    {
        int32_t passIndex = material->GetShader()->GetFirstPassIndexWithTagValue("LightMode", lightMode);

        if (passIndex < 0)
        {
            return;
        }

        DrawMesh(meshDesc, material, passIndex);
    }

    void RenderGraphContext::DrawObjects(size_t numObjects, const RenderObject* const* objects, const std::string& lightMode)
    {
        // DrawObjects 时，如果 passIndex 为 -1，则不绘制
        std::vector<int32_t> passIndices(numObjects, -1);

        for (size_t i = 0; i < numObjects; i++)
        {
            const RenderObject* obj = objects[i];

            if (obj->Mat != nullptr && obj->Mat->GetShader() != nullptr)
            {
                passIndices[i] = obj->Mat->GetShader()->GetFirstPassIndexWithTagValue("LightMode", lightMode);
            }
        }

        DrawObjects(numObjects, objects, passIndices);
    }

    void RenderGraphContext::SetRenderTargets(int32_t numColorTargets, GfxRenderTexture* const* colorTargets,
        GfxRenderTexture* depthStencilTarget, const D3D12_VIEWPORT* viewport, const D3D12_RECT* scissorRect)
    {
        if (numColorTargets == 0 && depthStencilTarget == nullptr)
        {
            return;
        }

        if (numColorTargets < 0 || numColorTargets > 8)
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
            m_StateDesc.RTVFormats.clear();
            m_IsStateDescDirty = true;

            for (int32_t i = 0; i < numColorTargets; i++)
            {
                rtv[i] = colorTargets[i]->GetRtvDsvCpuDescriptorHandle();
                m_ColorTargets.push_back(colorTargets[i]);
                m_StateDesc.RTVFormats.push_back(colorTargets[i]->GetFormat());
            }

            if (depthStencilTarget == nullptr)
            {
                m_DepthStencilTarget = nullptr;
                m_StateDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
                cmd->OMSetRenderTargets(static_cast<UINT>(numColorTargets), rtv, FALSE, nullptr);
            }
            else
            {
                m_DepthStencilTarget = depthStencilTarget;
                m_StateDesc.DSVFormat = depthStencilTarget->GetFormat();
                D3D12_CPU_DESCRIPTOR_HANDLE dsv = depthStencilTarget->GetRtvDsvCpuDescriptorHandle();
                cmd->OMSetRenderTargets(static_cast<UINT>(numColorTargets), rtv, FALSE, &dsv);
            }

            GfxRenderTexture* anyRT = numColorTargets > 0 ? colorTargets[0] : depthStencilTarget;
            GfxRenderTextureDesc anyRTDesc = anyRT->GetDesc();
            m_StateDesc.SampleCount = anyRTDesc.SampleCount;
            m_StateDesc.SampleQuality = anyRTDesc.SampleQuality;
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

    void RenderGraphContext::SetWireframe(bool value)
    {
        if (m_StateDesc.Wireframe == value)
        {
            return;
        }

        m_StateDesc.Wireframe = value;
        m_IsStateDescDirty = true;
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

    ID3D12PipelineState* RenderGraphContext::GetPipelineState(int32_t inputDescId, ShaderPass* pass)
    {
        if (m_IsStateDescDirty)
        {
            m_StateDescHash = PipelineStateDesc::CalculateHash(m_StateDesc);
            m_IsStateDescDirty = false;
        }

        return pass->GetGraphicsPipelineState(inputDescId, m_StateDesc, m_StateDescHash);
    }

    void RenderGraphContext::SetPipelineStateAndRootSignature(ID3D12PipelineState* pso, ShaderPass* pass)
    {
        ID3D12GraphicsCommandList* cmd = GetD3D12GraphicsCommandList();

        if (pso != m_CurrentPipelineState)
        {
            cmd->SetPipelineState(pso);
            m_CurrentPipelineState = pso;
        }

        ID3D12RootSignature* rootSignature = pass->GetRootSignature();

        if (rootSignature != m_CurrentRootSignature)
        {
            cmd->SetGraphicsRootSignature(rootSignature); // TODO ??? 不是 PSO 里已经有了吗
            m_CurrentRootSignature = rootSignature;
        }
    }

    static void BindConstantBuffer(ID3D12GraphicsCommandList* cmd, const ShaderProgram* program, int32_t id, D3D12_GPU_VIRTUAL_ADDRESS address)
    {
        const auto& cbMap = program->GetConstantBuffers();

        if (auto it = cbMap.find(id); it != cbMap.end())
        {
            cmd->SetGraphicsRootConstantBufferView(it->second.RootParameterIndex, address);
        }
    }

    void RenderGraphContext::BindResources(Material* material, int32_t shaderPassIndex, D3D12_GPU_VIRTUAL_ADDRESS perObjectConstantBufferAddress)
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

                    if (auto it = m_PassTextures.find(kv.first); it != m_PassTextures.end())
                    {
                        texture = it->second;
                    }
                    else
                    {
                        material->GetTexture(kv.first, &texture);
                    }

                    if (texture != nullptr)
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
                    if (!kv.second.HasSampler)
                    {
                        continue;
                    }

                    GfxTexture* texture = nullptr;

                    if (auto it = m_PassTextures.find(kv.first); it != m_PassTextures.end())
                    {
                        texture = it->second;
                    }
                    else
                    {
                        material->GetTexture(kv.first, &texture);
                    }

                    if (texture != nullptr)
                    {
                        samplerTable.Copy(kv.second.SamplerDescriptorTableIndex, texture->GetSamplerCpuDescriptorHandle());
                    }
                }

                cmd->SetGraphicsRootDescriptorTable(program->GetSamplerRootParameterIndex(), samplerTable.GetGpuHandle(0));
            }
        }

        if (pass->GetStencilState().Enable)
        {
            if (!m_IsStencilRefSet || m_CurrentStencilRef != pass->GetStencilState().Ref)
            {
                cmd->OMSetStencilRef(static_cast<UINT>(pass->GetStencilState().Ref));
                m_CurrentStencilRef = pass->GetStencilState().Ref;
                m_IsStencilRefSet = true;
            }
        }
    }

    void RenderGraphContext::ClearPreviousPassData()
    {
        m_PassTextures.clear();
    }

    void RenderGraphContext::Reset()
    {
        m_ColorTargets.clear();
        m_DepthStencilTarget = nullptr;
        m_IsStateDescDirty = true;
        m_CurrentPipelineState = nullptr;
        m_CurrentRootSignature = nullptr;
        m_CurrentPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        m_IsStencilRefSet = false;
        m_GlobalConstantBuffers.clear();
        m_PassTextures.clear();
    }
}
