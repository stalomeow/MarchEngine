#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include "Rendering/Shader.h"
#include <stdint.h>

namespace dx12demo
{
    struct MeshRendererDesc
    {
        D3D12_INPUT_LAYOUT_DESC InputLayout;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;

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

    ID3D12PipelineState* GetGraphicsPipelineState(
        const ShaderPass& pass,
        const MeshRendererDesc& rendererDesc,
        const RenderPipelineDesc& pipelineDesc);

    namespace
    {
        // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Hash.h

        // FNV-1a hash
        inline size_t HashRange(const uint32_t* const Begin, const uint32_t* const End, size_t Hash)
        {
            for (const uint32_t* Iter = Begin; Iter < End; ++Iter)
            {
                Hash = 16777619U * Hash ^ *Iter;
            }

            return Hash;
        }
    }

    template <typename T>
    inline size_t HashState(const T* StateDesc, size_t Count = 1, size_t Hash = 2166136261U)
    {
        static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
        return HashRange((uint32_t*)StateDesc, (uint32_t*)(StateDesc + Count), Hash);
    }
}
