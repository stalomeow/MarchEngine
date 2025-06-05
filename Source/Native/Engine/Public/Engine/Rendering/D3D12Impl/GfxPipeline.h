#pragma once

#include "Engine/Ints.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Memory/RefCounting.h"
#include "Engine/Rendering/D3D12Impl/GfxResource.h"
#include "Engine/Rendering/D3D12Impl/GfxDescriptor.h"
#include "Engine/Rendering/D3D12Impl/GfxBuffer.h"
#include "Engine/Rendering/D3D12Impl/GfxTexture.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/ShaderGraphics.h"
#include "Engine/Rendering/D3D12Impl/ShaderCompute.h"
#include <directx/d3dx12.h>
#include <assert.h>
#include <vector>
#include <bitset>
#include <unordered_map>
#include <limits>
#include <optional>

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
            , InstanceDataStepRate(instanceDataStepRate)
        {
        }
    };

    class GfxInputDesc final
    {
        D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_Layout;
        size_t m_Hash;

    public:
        GfxInputDesc(D3D12_PRIMITIVE_TOPOLOGY topology, const std::vector<GfxInputElement>& elements);

        D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType() const;

        D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetLayout() const { return m_Layout; }
        size_t GetHash() const { return m_Hash; }
    };

    class GfxOutputDesc final
    {
        mutable bool m_IsDirty;
        mutable size_t m_Hash;

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

        GfxOutputDesc();

        void MarkDirty();
        size_t GetHash() const;

        bool IsDirty() const { return m_IsDirty; }
    };

    enum class GfxPipelineType
    {
        Graphics,
        Compute,
    };

    template <GfxPipelineType _PipelineType>
    struct GfxPipelineTraits;

    template <>
    struct GfxPipelineTraits<GfxPipelineType::Graphics>
    {
        static constexpr size_t NumProgramTypes = Shader::NumProgramTypes;
        static constexpr size_t PixelProgramType = static_cast<size_t>(ShaderProgramType::Pixel);

        static constexpr auto SetRootSignature = &ID3D12GraphicsCommandList::SetGraphicsRootSignature;
        static constexpr auto SetRootConstantBufferView = &ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView;
        static constexpr auto SetRootShaderResourceView = &ID3D12GraphicsCommandList::SetGraphicsRootShaderResourceView;
        static constexpr auto SetRootDescriptorTable = &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable;
    };

    template <>
    struct GfxPipelineTraits<GfxPipelineType::Compute>
    {
        static constexpr size_t NumProgramTypes = ComputeShader::NumProgramTypes;
        static constexpr size_t PixelProgramType = std::numeric_limits<size_t>::max(); // no pixel program

        static constexpr auto SetRootSignature = &ID3D12GraphicsCommandList::SetComputeRootSignature;
        static constexpr auto SetRootConstantBufferView = &ID3D12GraphicsCommandList::SetComputeRootConstantBufferView;
        static constexpr auto SetRootShaderResourceView = &ID3D12GraphicsCommandList::SetComputeRootShaderResourceView;
        static constexpr auto SetRootDescriptorTable = &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable;
    };

    template <size_t Capacity>
    class GfxOfflineDescriptorTable
    {
        size_t m_Num; // 设置的最大 index + 1
        D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptors[Capacity];
        bool m_IsDirty;

    public:
        void Reset()
        {
            m_Num = 0;
            memset(m_Descriptors, 0, sizeof(m_Descriptors));
            m_IsDirty = false;
        }

        void Set(size_t index, D3D12_CPU_DESCRIPTOR_HANDLE handle)
        {
            if (index < m_Num && m_Descriptors[index].ptr == handle.ptr)
            {
                return;
            }

            m_Num = std::max(m_Num, index + 1);
            m_Descriptors[index] = handle;
            m_IsDirty = true;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE* GetDescriptors() const { return m_Descriptors; }
        size_t GetNum() const { return m_Num; }
        bool IsEmpty() const { return m_Num == 0; }
        constexpr size_t GetCapacity() const { return Capacity; }
        bool IsDirty() const { return m_IsDirty; }
        void SetDirty(bool value) { m_IsDirty = value; }
    };

    template <size_t Capacity>
    class GfxRootSrvCbvBufferCache
    {
        size_t m_Num{}; // 设置的最大 index + 1
        D3D12_GPU_VIRTUAL_ADDRESS m_Addresses[Capacity]{};
        std::bitset<Capacity> m_IsConstantBuffer{};
        std::bitset<Capacity> m_IsDirty{};

    public:
        void Reset()
        {
            m_Num = 0;
            memset(m_Addresses, 0, sizeof(m_Addresses));
            m_IsConstantBuffer.reset();
            m_IsDirty.reset();
        }

        void Set(size_t index, D3D12_GPU_VIRTUAL_ADDRESS address, bool isConstantBuffer)
        {
            if (index < m_Num && m_Addresses[index] == address && m_IsConstantBuffer.test(index) == isConstantBuffer)
            {
                return;
            }

            m_Num = std::max(m_Num, index + 1);
            m_Addresses[index] = address;
            m_IsConstantBuffer.set(index, isConstantBuffer);
            m_IsDirty.set(index);
        }

        D3D12_GPU_VIRTUAL_ADDRESS Get(size_t index, bool* pOutIsConstantBuffer) const
        {
            assert(index < m_Num);
            *pOutIsConstantBuffer = m_IsConstantBuffer.test(index);
            return m_Addresses[index];
        }

        // 清空 dirty 标记
        void Apply() { m_IsDirty.reset(); }

        size_t GetNum() const { return m_Num; }
        bool IsEmpty() const { return m_Num == 0; }
        constexpr size_t GetCapacity() const { return Capacity; }
        bool IsDirty(size_t index) const { return m_IsDirty.test(index); }
    };
}
