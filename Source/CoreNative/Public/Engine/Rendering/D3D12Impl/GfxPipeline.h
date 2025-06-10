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

    template <GfxPipelineType _PipelineType>
    class GfxPipelineParameterCache final
    {
        using PipelineTraits = GfxPipelineTraits<_PipelineType>;

        // https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html
        // The maximum size of a root arguments is 64 DWORDs.
        // Descriptor tables cost 1 DWORD each.
        // Root constants cost 1 DWORD * NumConstants, by definition since they are collections of 32-bit values.
        // Raw/Structured Buffer SRVs/UAVs and CBVs cost 2 DWORDs.

        // Shader Stage 的数量
        static constexpr size_t NumProgramTypes = PipelineTraits::NumProgramTypes;

        // 每个 Shader Stage 固定有 srv/uav 和 sampler 两个 descriptor table
        static constexpr size_t NumDescriptorTables = 2 * NumProgramTypes;

        // 剩下的空间都分给 root srv/cbv buffer
        // 在创建 root signature 时，root srv/cbv buffer 是排在 descriptor table 前面的
        static constexpr size_t NumMaxRootSrvCbvBuffers = (64 - NumDescriptorTables) / 2;

        using RootSignatureType = ShaderRootSignature<NumProgramTypes>;

        GfxDevice* m_Device;
        RootSignatureType* m_RootSignature;
        bool m_IsRootSignatureDirty;

        // 如果 root signature 变化，root parameter cache 全部清空
        // 如果 root signature 没变，只有 dirty 时才重新设置 root descriptor table
        // 设置 root descriptor table 后，需要清除 dirty 标记
        // 切换 heap 时，需要强制设置为 dirty，重新设置所有 root descriptor table

        GfxRootSrvCbvBufferCache<NumMaxRootSrvCbvBuffers> m_SrvCbvBufferCache[NumProgramTypes];
        GfxOfflineDescriptorTable<64> m_SrvUavCache[NumProgramTypes];
        GfxOfflineDescriptorTable<16> m_SamplerCache[NumProgramTypes];

        struct ResourceStateKey
        {
            RefCountPtr<GfxResource> Resource;
            uint32_t SubresourceIndex; // -1 表示整个资源

            bool operator ==(const ResourceStateKey& other) const
            {
                return Resource == other.Resource && SubresourceIndex == other.SubresourceIndex;
            }
        };

        struct ResourceStateKeyHash
        {
            size_t operator()(const ResourceStateKey& key) const
            {
                return std::hash<RefCountPtr<GfxResource>>()(key.Resource) ^ std::hash<uint32_t>()(key.SubresourceIndex);
            }
        };

        // 暂存 srv/uav/cbv 资源需要的状态
        std::unordered_map<ResourceStateKey, D3D12_RESOURCE_STATES, ResourceStateKeyHash> m_StagedResourceStates;

        static constexpr bool AllowPixelProgram = PipelineTraits::PixelProgramType < NumProgramTypes;
        static constexpr bool IsPixelProgram(size_t type) { return type == PipelineTraits::PixelProgramType; }

        void StageResourceState(RefCountPtr<GfxResource> resource, D3D12_RESOURCE_STATES state)
        {
            uint32_t subresourceIndex = -1;
            m_StagedResourceStates[ResourceStateKey{ resource, subresourceIndex }] |= state;
        }

        void StageTextureMipSliceSubresourceState(GfxTexture* texture, GfxTextureElement element, uint32_t mipSlice, D3D12_RESOURCE_STATES state)
        {
            GfxTextureDimension dimension = texture->GetDesc().Dimension;
            RefCountPtr<GfxResource> resource = texture->GetUnderlyingResource();

            for (uint32_t arraySlice = 0; arraySlice < texture->GetDesc().DepthOrArraySize; arraySlice++)
            {
                if (dimension == GfxTextureDimension::Cube || dimension == GfxTextureDimension::CubeArray)
                {
                    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
                    {
                        GfxCubemapFace face = static_cast<GfxCubemapFace>(faceIndex);
                        uint32_t subresourceIndex = texture->GetSubresourceIndex(element, face, arraySlice, mipSlice);
                        m_StagedResourceStates[ResourceStateKey{ resource, subresourceIndex }] |= state;
                    }
                }
                else
                {
                    uint32_t subresourceIndex = texture->GetSubresourceIndex(element, arraySlice, mipSlice);
                    m_StagedResourceStates[ResourceStateKey{ resource, subresourceIndex }] |= state;
                }
            }
        }

        void SetSrvCbvBuffer(size_t type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element, bool isConstantBuffer)
        {
            D3D12_GPU_VIRTUAL_ADDRESS address = buffer->GetGpuVirtualAddress(element);
            m_SrvCbvBufferCache[type].Set(static_cast<size_t>(index), address, isConstantBuffer);

            D3D12_RESOURCE_STATES state;

            if (isConstantBuffer)
            {
                state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            }
            else if constexpr (AllowPixelProgram)
            {
                state = IsPixelProgram(type)
                    ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                    : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }
            else
            {
                state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }

            // 记录状态，之后会统一使用 ResourceBarrier
            StageResourceState(buffer->GetUnderlyingResource(), state);
        }

        void SetSrvTexture(size_t type, uint32_t index, GfxTexture* texture, GfxTextureElement element, std::optional<uint32_t> mipSlice)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetSrv(element, mipSlice);
            m_SrvUavCache[type].Set(static_cast<size_t>(index), offlineDescriptor);

            D3D12_RESOURCE_STATES state;

            if constexpr (AllowPixelProgram)
            {
                state = IsPixelProgram(type)
                    ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                    : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }
            else
            {
                state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }

            // 记录状态，之后会统一使用 ResourceBarrier
            if (mipSlice)
            {
                StageTextureMipSliceSubresourceState(texture, element, *mipSlice, state);
            }
            else
            {
                StageResourceState(texture->GetUnderlyingResource(), state);
            }
        }

        void SetUavBuffer(size_t type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = buffer->GetUav(element);
            m_SrvUavCache[type].Set(static_cast<size_t>(index), offlineDescriptor);

            // 记录状态，之后会统一使用 ResourceBarrier
            StageResourceState(buffer->GetUnderlyingResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }

        void SetUavTexture(size_t type, uint32_t index, GfxTexture* texture, GfxTextureElement element, uint32_t mipSlice)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetUav(element, mipSlice);
            m_SrvUavCache[type].Set(static_cast<size_t>(index), offlineDescriptor);

            // 记录状态，之后会统一使用 ResourceBarrier
            StageTextureMipSliceSubresourceState(texture, element, mipSlice, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }

        void SetSampler(size_t type, uint32_t index, GfxTexture* texture)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetSampler();
            m_SamplerCache[type].Set(static_cast<size_t>(index), offlineDescriptor);
        }

        void SetRootSrvCbvBuffers(ID3D12GraphicsCommandList* cmd)
        {
            for (auto& cache : m_SrvCbvBufferCache)
            {
                for (size_t i = 0; i < cache.GetNum(); i++)
                {
                    if (!cache.IsDirty(i))
                    {
                        continue;
                    }

                    bool isConstantBuffer = false;
                    D3D12_GPU_VIRTUAL_ADDRESS address = cache.Get(i, &isConstantBuffer);

                    if (isConstantBuffer)
                    {
                        (cmd->*PipelineTraits::SetRootConstantBufferView)(static_cast<UINT>(i), address);
                    }
                    else
                    {
                        (cmd->*PipelineTraits::SetRootShaderResourceView)(static_cast<UINT>(i), address);
                    }
                }

                cache.Apply();
            }
        }

        void SetDescriptorHeaps(
            ID3D12GraphicsCommandList* cmd,
            GfxDescriptorHeap* viewHeap,
            GfxDescriptorHeap* samplerHeap);

        void SetRootDescriptorTablesAndHeaps(
            ID3D12GraphicsCommandList* cmd,
            GfxDescriptorHeap** ppViewHeap,
            GfxDescriptorHeap** ppSamplerHeap);

    public:
        GfxPipelineParameterCache(GfxDevice* device)
            : m_Device(device)
            , m_RootSignature(nullptr)
            , m_IsRootSignatureDirty(true)
            , m_SrvCbvBufferCache{}
            , m_SrvUavCache{}
            , m_SamplerCache{}
            , m_StagedResourceStates{}
        {
        }

        void Reset()
        {
            m_RootSignature = nullptr;
            m_IsRootSignatureDirty = false;
            for (auto& cache : m_SrvCbvBufferCache) cache.Reset();
            for (auto& cache : m_SrvUavCache) cache.Reset();
            for (auto& cache : m_SamplerCache) cache.Reset();
            m_StagedResourceStates.clear();
        }

        void SetRootSignature(RootSignatureType* rootSignature)
        {
            // 内部的 ID3D12RootSignature 是复用的，如果 ID3D12RootSignature 变了，说明根签名发生了结构上的变化
            if (m_RootSignature == nullptr || m_RootSignature->GetD3DRootSignature() != rootSignature->GetD3DRootSignature())
            {
                m_IsRootSignatureDirty = true;

                // 删掉旧的 view
                for (auto& cache : m_SrvCbvBufferCache) cache.Reset();
                for (auto& cache : m_SrvUavCache) cache.Reset();
                for (auto& cache : m_SamplerCache) cache.Reset();
                m_StagedResourceStates.clear();
            }

            m_RootSignature = rootSignature;
        }

        template <typename FindBufferFunc>
        void SetSrvCbvBuffers(FindBufferFunc fn)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& buf : m_RootSignature->GetSrvCbvBuffers(i))
                {
                    GfxBufferElement element = GfxBufferElement::StructuredData;

                    if (GfxBuffer* buffer = fn(buf, &element))
                    {
                        SetSrvCbvBuffer(i, buf.RootParameterIndex, buffer, element, buf.IsConstantBuffer);
                    }
                    else if (buf.IsConstantBuffer)
                    {
                        throw GfxException(StringUtils::Format("Failed to find root cbv buffer parameter '{}'", ShaderUtils::GetStringFromId(buf.Id)));
                    }
                    else
                    {
                        throw GfxException(StringUtils::Format("Failed to find root srv buffer parameter '{}'", ShaderUtils::GetStringFromId(buf.Id)));
                    }
                }
            }
        }

        void UpdateSrvCbvBuffer(int32_t id, GfxBuffer* buffer, GfxBufferElement element)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& buf : m_RootSignature->GetSrvCbvBuffers(i))
                {
                    if (buf.Id != id)
                    {
                        continue;
                    }

                    SetSrvCbvBuffer(i, buf.RootParameterIndex, buffer, element, buf.IsConstantBuffer);
                }
            }
        }

        template <typename FindTextureFunc>
        void SetSrvTexturesAndSamplers(FindTextureFunc fn)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& tex : m_RootSignature->GetSrvTextures(i))
                {
                    GfxTextureElement element = GfxTextureElement::Default;
                    std::optional<uint32_t> mipSlice = std::nullopt;

                    if (GfxTexture* texture = fn(tex, &element, &mipSlice))
                    {
                        SetSrvTexture(i, tex.DescriptorTableSlotTexture, texture, element, mipSlice);

                        if (tex.DescriptorTableSlotSampler.has_value())
                        {
                            SetSampler(i, *tex.DescriptorTableSlotSampler, texture);
                        }
                    }
                    else
                    {
                        throw GfxException(StringUtils::Format("Failed to find root srv texture parameter '{}'", ShaderUtils::GetStringFromId(tex.Id)));
                    }
                }
            }
        }

        template <typename FindBufferFunc>
        void SetUavBuffers(FindBufferFunc fn)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& buf : m_RootSignature->GetUavBuffers(i))
                {
                    GfxBufferElement element = GfxBufferElement::StructuredData;

                    if (GfxBuffer* buffer = fn(buf, &element))
                    {
                        SetUavBuffer(i, buf.DescriptorTableSlot, buffer, element);
                    }
                    else
                    {
                        throw GfxException(StringUtils::Format("Failed to find root uav buffer parameter '{}'", ShaderUtils::GetStringFromId(buf.Id)));
                    }
                }
            }
        }

        template <typename FindTextureFunc>
        void SetUavTextures(FindTextureFunc fn)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& tex : m_RootSignature->GetUavTextures(i))
                {
                    GfxTextureElement element = GfxTextureElement::Default;
                    std::optional<uint32_t> mipSlice = std::nullopt;

                    if (GfxTexture* texture = fn(tex, &element, &mipSlice))
                    {
                        SetUavTexture(i, tex.DescriptorTableSlot, texture, element, mipSlice.value_or(0));
                    }
                    else
                    {
                        throw GfxException(StringUtils::Format("Failed to find root uav texture parameter '{}'", ShaderUtils::GetStringFromId(tex.Id)));
                    }
                }
            }
        }

        template <typename TransitionFunc>
        void TransitionResources(TransitionFunc fn)
        {
            for (const auto& [key, state] : m_StagedResourceStates)
            {
                fn(key.Resource, key.SubresourceIndex, state);
            }

            m_StagedResourceStates.clear();
        }

        void Apply(ID3D12GraphicsCommandList* cmd, GfxDescriptorHeap** ppViewHeap, GfxDescriptorHeap** ppSamplerHeap)
        {
            if (m_IsRootSignatureDirty)
            {
                (cmd->*PipelineTraits::SetRootSignature)(m_RootSignature->GetD3DRootSignature());
                m_IsRootSignatureDirty = false;
            }

            SetRootSrvCbvBuffers(cmd);
            SetRootDescriptorTablesAndHeaps(cmd, ppViewHeap, ppSamplerHeap);
        }
    };

    template <GfxPipelineType _PipelineType>
    void GfxPipelineParameterCache<_PipelineType>::SetDescriptorHeaps(ID3D12GraphicsCommandList* cmd, GfxDescriptorHeap* viewHeap, GfxDescriptorHeap* samplerHeap)
    {
        if (viewHeap != nullptr && samplerHeap != nullptr)
        {
            ID3D12DescriptorHeap* heaps[] = { viewHeap->GetD3DDescriptorHeap(), samplerHeap->GetD3DDescriptorHeap() };
            cmd->SetDescriptorHeaps(2, heaps);
        }
        else if (viewHeap != nullptr)
        {
            ID3D12DescriptorHeap* heaps[] = { viewHeap->GetD3DDescriptorHeap() };
            cmd->SetDescriptorHeaps(1, heaps);
        }
        else if (samplerHeap != nullptr)
        {
            ID3D12DescriptorHeap* heaps[] = { samplerHeap->GetD3DDescriptorHeap() };
            cmd->SetDescriptorHeaps(1, heaps);
        }
    }

    template <GfxPipelineType _PipelineType>
    void GfxPipelineParameterCache<_PipelineType>::SetRootDescriptorTablesAndHeaps(
        ID3D12GraphicsCommandList* cmd,
        GfxDescriptorHeap** ppViewHeap,
        GfxDescriptorHeap** ppSamplerHeap)
    {
        // ------------------------------------------------------------
        // SRV & UAV
        // ------------------------------------------------------------

        D3D12_GPU_DESCRIPTOR_HANDLE srvUavTables[NumProgramTypes]{};
        const D3D12_CPU_DESCRIPTOR_HANDLE* offlineSrvUav[NumProgramTypes]{};
        uint32_t numSrvUav[NumProgramTypes]{};

        GfxOnlineDescriptorMultiAllocator* viewAllocator = m_Device->GetOnlineViewDescriptorAllocator();
        GfxDescriptorHeap* viewHeap = nullptr;
        bool hasSrvUav = false;

        for (uint32_t numTry = 0; numTry < 2; numTry++)
        {
            uint32_t totalNumSrvUav = 0;

            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                std::optional<uint32_t> srvUavTableRootParamIndex = m_RootSignature->GetSrvUavTableRootParamIndex(i);
                const auto& srvUavCache = m_SrvUavCache[i];

                if (srvUavTableRootParamIndex.has_value() && srvUavCache.IsDirty() && !srvUavCache.IsEmpty())
                {
                    offlineSrvUav[i] = srvUavCache.GetDescriptors();
                    numSrvUav[i] = static_cast<uint32_t>(srvUavCache.GetNum());
                }
                else
                {
                    offlineSrvUav[i] = nullptr;
                    numSrvUav[i] = 0;
                }

                totalNumSrvUav += numSrvUav[i];
            }

            if (totalNumSrvUav > 0)
            {
                if (viewAllocator->AllocateMany(std::size(srvUavTables), offlineSrvUav, numSrvUav, srvUavTables, &viewHeap))
                {
                    hasSrvUav = true;
                    break;
                }

                // 当前的 heap 不够，切换 heap
                viewAllocator->Rollover();

                // 因为 heap 变了，所以 table 要全部重新分配
                for (auto& cache : m_SrvUavCache)
                {
                    cache.SetDirty(true);
                }
            }
            else
            {
                // 没有 SRV & UAV，不需要分配
                break;
            }
        }

        // ------------------------------------------------------------
        // SAMPLER
        // ------------------------------------------------------------

        D3D12_GPU_DESCRIPTOR_HANDLE samplerTables[NumProgramTypes]{};
        const D3D12_CPU_DESCRIPTOR_HANDLE* offlineSamplers[NumProgramTypes]{};
        uint32_t numSamplers[NumProgramTypes]{};

        GfxOnlineDescriptorMultiAllocator* samplerAllocator = m_Device->GetOnlineSamplerDescriptorAllocator();
        GfxDescriptorHeap* samplerHeap = nullptr;
        bool hasSampler = false;

        for (uint32_t numTry = 0; numTry < 2; numTry++)
        {
            uint32_t totalNumSamplers = 0;

            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                std::optional<uint32_t> samplerTableRootParamIndex = m_RootSignature->GetSamplerTableRootParamIndex(i);
                const auto& samplerCache = m_SamplerCache[i];

                if (samplerTableRootParamIndex.has_value() && samplerCache.IsDirty() && !samplerCache.IsEmpty())
                {
                    offlineSamplers[i] = samplerCache.GetDescriptors();
                    numSamplers[i] = static_cast<uint32_t>(samplerCache.GetNum());
                }
                else
                {
                    offlineSamplers[i] = nullptr;
                    numSamplers[i] = 0;
                }

                totalNumSamplers += numSamplers[i];
            }

            if (totalNumSamplers > 0)
            {
                if (samplerAllocator->AllocateMany(std::size(samplerTables), offlineSamplers, numSamplers, samplerTables, &samplerHeap))
                {
                    hasSampler = true;
                    break;
                }

                // 当前的 heap 不够，切换 heap
                samplerAllocator->Rollover();

                // 因为 heap 变了，所以 table 要全部重新分配
                for (auto& cache : m_SamplerCache)
                {
                    cache.SetDirty(true);
                }
            }
            else
            {
                // 没有 SAMPLER，不需要分配
                break;
            }
        }

        // ------------------------------------------------------------
        // Apply
        // ------------------------------------------------------------

        if (!hasSrvUav && !hasSampler)
        {
            return;
        }

        bool isHeapChanged = false;

        if (hasSrvUav && *ppViewHeap != viewHeap)
        {
            *ppViewHeap = viewHeap;
            isHeapChanged = true;
        }

        if (hasSampler && *ppSamplerHeap != samplerHeap)
        {
            *ppSamplerHeap = samplerHeap;
            isHeapChanged = true;
        }

        if (isHeapChanged)
        {
            // 以 *ppViewHeap 和 *ppSamplerHeap 的值为准
            SetDescriptorHeaps(cmd, *ppViewHeap, *ppSamplerHeap);
        }

        for (size_t i = 0; i < NumProgramTypes; i++)
        {
            if (hasSrvUav && numSrvUav[i] > 0)
            {
                uint32_t rootParamIndex = *m_RootSignature->GetSrvUavTableRootParamIndex(i);
                (cmd->*PipelineTraits::SetRootDescriptorTable)(static_cast<UINT>(rootParamIndex), srvUavTables[i]);
            }

            if (hasSampler && numSamplers[i] > 0)
            {
                uint32_t rootParamIndex = *m_RootSignature->GetSamplerTableRootParamIndex(i);
                (cmd->*PipelineTraits::SetRootDescriptorTable)(static_cast<UINT>(rootParamIndex), samplerTables[i]);
            }
        }

        if (hasSrvUav)
        {
            for (auto& cache : m_SrvUavCache)
            {
                cache.SetDirty(false);
            }
        }

        if (hasSampler)
        {
            for (auto& cache : m_SamplerCache)
            {
                cache.SetDirty(false);
            }
        }
    }
}
