#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include "Rendering/Shader.h"

namespace dx12demo
{
    struct MeshRendererDesc
    {
        D3D12_INPUT_LAYOUT_DESC InputLayout;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
        const ShaderPass* Pass;

        size_t GetHash() const;
    };

    struct RenderPipelineDesc
    {
        UINT NumRenderTargets;
        DXGI_FORMAT RTVFormats[8];
        DXGI_FORMAT DSVFormat;
        DXGI_SAMPLE_DESC SampleDesc;
        bool Wireframe;
    };

    ID3D12PipelineState* GetGraphicsPipelineState(const MeshRendererDesc& rendererDesc, const RenderPipelineDesc& pipelineDesc);
}
