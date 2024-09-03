#include "Rendering/PipelineState.h"
#include "Rendering/GfxManager.h"
#include <unordered_map>
#include <wrl.h>

using namespace Microsoft::WRL;

namespace dx12demo
{
    namespace
    {
        std::unordered_map<size_t, ComPtr<ID3D12PipelineState>> g_PsoMap;

        // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Hash.h

        // FNV-1a hash
        size_t HashRange(const uint32_t* const Begin, const uint32_t* const End, size_t Hash)
        {
            for (const uint32_t* Iter = Begin; Iter < End; ++Iter)
            {
                Hash = 16777619U * Hash ^ *Iter;
            }

            return Hash;
        }

        template <typename T>
        size_t HashState(const T* StateDesc, size_t Count = 1, size_t Hash = 2166136261U)
        {
            static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
            return HashRange((uint32_t*)StateDesc, (uint32_t*)(StateDesc + Count), Hash);
        }
    }

    size_t MeshRendererDesc::GetHash() const
    {
        size_t hash = HashState(InputLayout.pInputElementDescs, InputLayout.NumElements);
        hash = HashState(&PrimitiveTopologyType, 1, hash);
        hash = HashState(&Pass, 1, hash); // Hash the pointer value
        return hash;
    }

    ID3D12PipelineState* GetGraphicsPipelineState(const MeshRendererDesc& rendererDesc, const RenderPipelineDesc& pipelineDesc)
    {
        size_t hash = HashState(&pipelineDesc, 1, rendererDesc.GetHash());
        auto psoCache = g_PsoMap.find(hash);

        if (psoCache != g_PsoMap.end())
        {
            return psoCache->second.Get();
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = rendererDesc.InputLayout;
        psoDesc.pRootSignature = rendererDesc.Pass->GetRootSignature();
        psoDesc.VS =
        {
            rendererDesc.Pass->VertexShader->GetBufferPointer(),
            rendererDesc.Pass->VertexShader->GetBufferSize()
        };
        psoDesc.PS =
        {
            rendererDesc.Pass->PixelShader->GetBufferPointer(),
            rendererDesc.Pass->PixelShader->GetBufferSize()
        };

        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(static_cast<int>(rendererDesc.Pass->Cull) + 1);
        psoDesc.RasterizerState.FillMode = pipelineDesc.Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.BlendState.IndependentBlendEnable = TRUE;

        for (int i = 0; i < rendererDesc.Pass->Blends.size(); i++)
        {
            const ShaderPassBlendState& b = rendererDesc.Pass->Blends[i];
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
        psoDesc.DepthStencilState.DepthEnable = rendererDesc.Pass->DepthState.Enable;
        psoDesc.DepthStencilState.DepthWriteMask = rendererDesc.Pass->DepthState.Write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(rendererDesc.Pass->DepthState.Compare) + 1);
        psoDesc.DepthStencilState.StencilEnable = rendererDesc.Pass->StencilState.Enable;
        psoDesc.DepthStencilState.StencilReadMask = static_cast<UINT8>(rendererDesc.Pass->StencilState.ReadMask);
        psoDesc.DepthStencilState.StencilWriteMask = static_cast<UINT8>(rendererDesc.Pass->StencilState.WriteMask);
        psoDesc.DepthStencilState.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rendererDesc.Pass->StencilState.FrontFace.FailOp) + 1);
        psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rendererDesc.Pass->StencilState.FrontFace.DepthFailOp) + 1);
        psoDesc.DepthStencilState.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rendererDesc.Pass->StencilState.FrontFace.PassOp) + 1);
        psoDesc.DepthStencilState.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(rendererDesc.Pass->StencilState.FrontFace.Compare) + 1);
        psoDesc.DepthStencilState.BackFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rendererDesc.Pass->StencilState.BackFace.FailOp) + 1);
        psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rendererDesc.Pass->StencilState.BackFace.DepthFailOp) + 1);
        psoDesc.DepthStencilState.BackFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rendererDesc.Pass->StencilState.BackFace.PassOp) + 1);
        psoDesc.DepthStencilState.BackFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(rendererDesc.Pass->StencilState.BackFace.Compare) + 1);

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = rendererDesc.PrimitiveTopologyType;
        psoDesc.NumRenderTargets = pipelineDesc.NumRenderTargets;
        memcpy(psoDesc.RTVFormats, pipelineDesc.RTVFormats, sizeof(pipelineDesc.RTVFormats));
        psoDesc.DSVFormat = pipelineDesc.DSVFormat;
        psoDesc.SampleDesc = pipelineDesc.SampleDesc;

        ComPtr<ID3D12PipelineState>& pso = g_PsoMap[hash];
        auto device = GetGfxManager().GetDevice();
        THROW_IF_FAILED(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
        return pso.Get();
    }
}
