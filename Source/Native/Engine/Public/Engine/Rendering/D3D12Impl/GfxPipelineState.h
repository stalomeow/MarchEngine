#pragma once

#include <directx/d3dx12.h>
#include <stdint.h>
#include <vector>
#include <functional>

namespace march
{
    enum class GfxSemantic
    {
        Position,
        Normal,
        Tangent,
        Color,
        TexCoord0,
        TexCoord1,
        TexCoord2,
        TexCoord3,
        TexCoord4,
        TexCoord5,
        TexCoord6,
        TexCoord7,

        // Aliases
        TexCoord = TexCoord0,
    };

    struct GfxInputElement
    {
        GfxSemantic Semantic;
        DXGI_FORMAT Format;
        uint32_t InputSlot;
        D3D12_INPUT_CLASSIFICATION InputSlotClass;
        uint32_t InstanceDataStepRate;

        constexpr GfxInputElement(
            GfxSemantic semantic,
            DXGI_FORMAT format,
            uint32_t inputSlot = 0,
            D3D12_INPUT_CLASSIFICATION inputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            uint32_t instanceDataStepRate = 0) noexcept
            : Semantic(semantic)
            , Format(format)
            , InputSlot(inputSlot)
            , InputSlotClass(inputSlotClass)
            , InstanceDataStepRate(instanceDataStepRate) {}
    };

    class GfxInputDesc final
    {
    public:
        GfxInputDesc(D3D12_PRIMITIVE_TOPOLOGY topology, const std::vector<GfxInputElement>& elements);

        D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const;

        D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetLayout() const { return m_Layout; }
        size_t GetHash() const { return m_Hash; }

    private:
        D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_Layout;
        size_t m_Hash;
    };

    class GfxOutputDesc final
    {
    public:
        GfxOutputDesc();

        void MarkDirty();
        size_t GetHash() const;

        bool IsDirty() const { return m_IsDirty; }

    public:
        uint32_t NumRTV;
        DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
        DXGI_FORMAT DSVFormat;

        uint32_t SampleCount;
        uint32_t SampleQuality;

        int32_t DepthBias;
        float DepthBiasClamp;
        float SlopeScaledDepthBias;

        bool Wireframe;

    private:
        mutable bool m_IsDirty;
        mutable size_t m_Hash;
    };

    struct ShaderPassRenderState;
    class Material;
    class ComputeShader;
    class ComputeShaderKernel;
    class ShaderKeywordSet;

    struct GfxPipelineState final
    {
        static size_t ResolveShaderPassRenderState(ShaderPassRenderState& state,
            const std::function<bool(int32_t, int32_t*)>& intResolver,
            const std::function<bool(int32_t, float*)>& floatResolver);

        static ID3D12PipelineState* GetGraphicsPSO(Material* material, size_t passIndex,
            const GfxInputDesc& inputDesc, const GfxOutputDesc& outputDesc);

        static ID3D12PipelineState* GetComputePSO(ComputeShader* shader, ComputeShaderKernel* kernel, const ShaderKeywordSet& keywords);
    };
}
