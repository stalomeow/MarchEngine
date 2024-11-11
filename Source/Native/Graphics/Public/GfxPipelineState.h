#pragma once

#include <directx/d3dx12.h>
#include <stdint.h>
#include <vector>

namespace march
{
    class Material;

    enum class PipelineInputSematicName
    {
        Position,
        Normal,
        Tangent,
        TexCoord,
        Color,
    };

    struct PipelineInputElement
    {
        PipelineInputSematicName SemanticName;
        uint32_t SemanticIndex;
        DXGI_FORMAT Format;
        uint32_t InputSlot;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        uint32_t InstanceDataStepRate;

        PipelineInputElement(
            PipelineInputSematicName semanticName,
            uint32_t semanticIndex,
            DXGI_FORMAT format,
            uint32_t inputSlot = 0,
            D3D12_INPUT_CLASSIFICATION inputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            uint32_t instanceDataStepRate = 0);
    };

    struct PipelineStateDesc
    {
        std::vector<DXGI_FORMAT> RTVFormats;
        DXGI_FORMAT DSVFormat;

        uint32_t SampleCount;
        uint32_t SampleQuality;

        bool Wireframe;

        static size_t CalculateHash(const PipelineStateDesc& desc);
    };

    class GfxPipelineState
    {
    public:
        static constexpr int32_t GetInvalidInputDescId() { return -1; }
        static int32_t CreateInputDesc(const std::vector<PipelineInputElement>& inputLayout, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
        static D3D12_PRIMITIVE_TOPOLOGY GetInputDescPrimitiveTopology(int32_t inputDescId);

        static ID3D12PipelineState* GetGraphicsState(Material* material, int32_t passIndex, int32_t inputDescId, const PipelineStateDesc& stateDesc, size_t stateDescHash);
    };
}
