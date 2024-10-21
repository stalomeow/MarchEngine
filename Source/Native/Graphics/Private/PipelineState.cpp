#include "PipelineState.h"
#include "GfxDevice.h"
#include "GfxExcept.h"
#include "GfxSettings.h"
#include "Debug.h"
#include <unordered_map>
#include <wrl.h>

using namespace Microsoft::WRL;

namespace march
{
    namespace
    {
        std::unordered_map<size_t, ComPtr<ID3D12PipelineState>> g_PsoMap;

        inline void ValidateDepthFunc(D3D12_COMPARISON_FUNC& func)
        {
            if constexpr (!GfxSettings::UseReversedZBuffer())
            {
                return;
            }

            switch (func)
            {
            case D3D12_COMPARISON_FUNC_LESS:
                func = D3D12_COMPARISON_FUNC_GREATER;
                break;

            case D3D12_COMPARISON_FUNC_LESS_EQUAL:
                func = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
                break;

            case D3D12_COMPARISON_FUNC_GREATER:
                func = D3D12_COMPARISON_FUNC_LESS;
                break;

            case D3D12_COMPARISON_FUNC_GREATER_EQUAL:
                func = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                break;
            }
        }
    }

    size_t MeshDesc::GetHash() const
    {
        size_t hash = HashState(InputLayout.pInputElementDescs, InputLayout.NumElements);
        hash = HashState(&PrimitiveTopologyType, 1, hash);
        return hash;
    }

    ID3D12PipelineState* GetGraphicsPipelineState(
        const ShaderPass* pPass,
        const MeshDesc& meshDesc,
        const RenderPipelineDesc& pipelineDesc)
    {
        size_t hash = HashState(pPass, 1, meshDesc.GetHash());
        hash = HashState(&pipelineDesc, 1, hash);

        auto psoCache = g_PsoMap.find(hash);
        if (psoCache != g_PsoMap.end())
        {
            return psoCache->second.Get();
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = meshDesc.InputLayout;
        psoDesc.pRootSignature = pPass->GetRootSignature();
        psoDesc.VS =
        {
            pPass->GetProgram(ShaderProgramType::Vertex)->GetBinaryData(),
            pPass->GetProgram(ShaderProgramType::Vertex)->GetBinarySize()
        };
        psoDesc.PS =
        {
            pPass->GetProgram(ShaderProgramType::Pixel)->GetBinaryData(),
            pPass->GetProgram(ShaderProgramType::Pixel)->GetBinarySize()
        };

        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(static_cast<int>(pPass->GetCull()) + 1);
        psoDesc.RasterizerState.FillMode = pipelineDesc.Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.BlendState.IndependentBlendEnable = TRUE;

        for (int i = 0; i < pPass->GetBlends().size(); i++)
        {
            const ShaderPassBlendState& b = pPass->GetBlends()[i];
            D3D12_RENDER_TARGET_BLEND_DESC& blendDesc = psoDesc.BlendState.RenderTarget[i];
            blendDesc.BlendEnable = b.Enable;
            blendDesc.LogicOpEnable = FALSE;
            blendDesc.SrcBlend = static_cast<D3D12_BLEND>(static_cast<int>(b.Rgb.Src) + 1);
            blendDesc.DestBlend = static_cast<D3D12_BLEND>(static_cast<int>(b.Rgb.Dest) + 1);
            blendDesc.BlendOp = static_cast<D3D12_BLEND_OP>(static_cast<int>(b.Rgb.Op) + 1);
            blendDesc.SrcBlendAlpha = static_cast<D3D12_BLEND>(static_cast<int>(b.Alpha.Src) + 1);
            blendDesc.DestBlendAlpha = static_cast<D3D12_BLEND>(static_cast<int>(b.Alpha.Dest) + 1);
            blendDesc.BlendOpAlpha = static_cast<D3D12_BLEND_OP>(static_cast<int>(b.Alpha.Op) + 1);
            blendDesc.RenderTargetWriteMask = static_cast<D3D12_COLOR_WRITE_ENABLE>(b.WriteMask);
        }

        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = pPass->GetDepthState().Enable;
        psoDesc.DepthStencilState.DepthWriteMask = pPass->GetDepthState().Write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(pPass->GetDepthState().Compare) + 1);
        ValidateDepthFunc(psoDesc.DepthStencilState.DepthFunc);
        psoDesc.DepthStencilState.StencilEnable = pPass->GetStencilState().Enable;
        psoDesc.DepthStencilState.StencilReadMask = static_cast<UINT8>(pPass->GetStencilState().ReadMask);
        psoDesc.DepthStencilState.StencilWriteMask = static_cast<UINT8>(pPass->GetStencilState().WriteMask);
        psoDesc.DepthStencilState.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(pPass->GetStencilState().FrontFace.FailOp) + 1);
        psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(pPass->GetStencilState().FrontFace.DepthFailOp) + 1);
        psoDesc.DepthStencilState.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(pPass->GetStencilState().FrontFace.PassOp) + 1);
        psoDesc.DepthStencilState.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(pPass->GetStencilState().FrontFace.Compare) + 1);
        psoDesc.DepthStencilState.BackFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(pPass->GetStencilState().BackFace.FailOp) + 1);
        psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(pPass->GetStencilState().BackFace.DepthFailOp) + 1);
        psoDesc.DepthStencilState.BackFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(pPass->GetStencilState().BackFace.PassOp) + 1);
        psoDesc.DepthStencilState.BackFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(pPass->GetStencilState().BackFace.Compare) + 1);

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = meshDesc.PrimitiveTopologyType;
        psoDesc.NumRenderTargets = pipelineDesc.NumRenderTargets;
        memcpy(psoDesc.RTVFormats, pipelineDesc.RTVFormats, sizeof(pipelineDesc.RTVFormats));
        psoDesc.DSVFormat = pipelineDesc.DSVFormat;
        psoDesc.SampleDesc = pipelineDesc.SampleDesc;

        ComPtr<ID3D12PipelineState>& pso = g_PsoMap[hash];
        auto device = GetGfxDevice()->GetD3D12Device();
        GFX_HR(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
        DEBUG_LOG_INFO("Create new Graphics PSO");
        return pso.Get();
    }

    void DestroyAllPipelineStates()
    {
        g_PsoMap.clear();
    }
}
