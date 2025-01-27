#pragma once

#include "Engine/Object.h"
#include "Engine/Graphics/GfxResource.h"
#include "Engine/Graphics/GfxDescriptor.h"
#include "Engine/Graphics/GfxBuffer.h"
#include "Engine/Graphics/GfxTexture.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/Shader.h"
#include <directx/d3dx12.h>
#include <stdint.h>
#include <assert.h>
#include <bitset>
#include <unordered_map>
#include <limits>

namespace march
{
    template <size_t Capacity>
    class GfxOfflineDescriptorTable
    {
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

    private:
        size_t m_Num; // 设置的最大 index + 1
        D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptors[Capacity];
        bool m_IsDirty;
    };

    template <size_t Capacity>
    class GfxRootSrvCbvBufferCache
    {
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

    private:
        size_t m_Num{}; // 设置的最大 index + 1
        D3D12_GPU_VIRTUAL_ADDRESS m_Addresses[Capacity]{};
        std::bitset<Capacity> m_IsConstantBuffer{};
        std::bitset<Capacity> m_IsDirty{};
    };

    struct GraphicsPipelineTraits
    {
        static constexpr size_t NumProgramTypes = Shader::NumProgramTypes;
        static constexpr size_t PixelProgramType = static_cast<size_t>(ShaderProgramType::Pixel);

        static constexpr auto SetRootSignature = &ID3D12GraphicsCommandList::SetGraphicsRootSignature;
        static constexpr auto SetRootConstantBufferView = &ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView;
        static constexpr auto SetRootShaderResourceView = &ID3D12GraphicsCommandList::SetGraphicsRootShaderResourceView;
        static constexpr auto SetRootDescriptorTable = &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable;
    };

    struct ComputePipelineTraits
    {
        static constexpr size_t NumProgramTypes = ComputeShader::NumProgramTypes;
        static constexpr size_t PixelProgramType = std::numeric_limits<size_t>::max(); // no pixel program

        static constexpr auto SetRootSignature = &ID3D12GraphicsCommandList::SetComputeRootSignature;
        static constexpr auto SetRootConstantBufferView = &ID3D12GraphicsCommandList::SetComputeRootConstantBufferView;
        static constexpr auto SetRootShaderResourceView = &ID3D12GraphicsCommandList::SetComputeRootShaderResourceView;
        static constexpr auto SetRootDescriptorTable = &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable;
    };

    template <typename _PipelineTraits>
    class GfxViewCache final
    {
        // https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html
        // The maximum size of a root arguments is 64 DWORDs.
        // Descriptor tables cost 1 DWORD each.
        // Root constants cost 1 DWORD * NumConstants, by definition since they are collections of 32-bit values.
        // Raw/Structured Buffer SRVs/UAVs and CBVs cost 2 DWORDs.

        // 即 Shader Stage 的数量
        static constexpr size_t NumProgramTypes = _PipelineTraits::NumProgramTypes;

        // 固定有 2 * shader-stage 个 descriptor table
        static constexpr size_t NumDescriptorTables = 2 * NumProgramTypes;

        // 最多可以有 24 个 root srv/cbv (在创建 root signature 时，root srv/cbv 是排在 descriptor table 前面的)
        static constexpr size_t NumMaxRootSrvCbvBuffers = (64 - NumDescriptorTables) / 2;

        GfxRootSrvCbvBufferCache<NumMaxRootSrvCbvBuffers> m_SrvCbvBufferCache[NumProgramTypes]; // root srv/cbv buffer

        // 如果 root signature 变化，root parameter cache 全部清空
        // 如果 root signature 没变，只有 dirty 时才重新设置 root descriptor table
        // 设置 root descriptor table 后，需要清除 dirty 标记
        // 切换 heap 时，需要强制设置为 dirty，重新设置所有 root descriptor table

        GfxOfflineDescriptorTable<64> m_SrvUavCache[NumProgramTypes];
        GfxOfflineDescriptorTable<16> m_SamplerCache[NumProgramTypes];

        ID3D12RootSignature* m_CurrentRootSignature;

        // 暂存 srv/uav/cbv 资源需要的状态
        std::unordered_map<RefCountPtr<GfxResource>, D3D12_RESOURCE_STATES> m_StagedResourceStates;

        GfxDevice* m_Device;

        static constexpr bool AllowPixelProgram = _PipelineTraits::PixelProgramType < NumProgramTypes;
        static constexpr bool IsPixelProgram(size_t type) { return type == _PipelineTraits::PixelProgramType; }

        using RootSignatureType = GfxRootSignature<NumProgramTypes>;

    public:
        void SetSrvCbvBuffer(size_t type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element, bool isConstantBuffer);

        void SetSrvTexture(size_t type, uint32_t index, GfxTexture* texture, GfxTextureElement element);

        void SetUavBuffer(size_t type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element);

        void SetUavTexture(size_t type, uint32_t index, GfxTexture* texture, GfxTextureElement element);

        void SetSampler(size_t type, uint32_t index, GfxTexture* texture);

        void SetRootSignature(ID3D12GraphicsCommandList* cmd, RootSignatureType* rootSignature);

        void SetRootDescriptorTablesAndHeaps(
            ID3D12GraphicsCommandList* cmd,
            RootSignatureType* rootSignature,
            GfxDescriptorHeap** ppViewHeap,
            GfxDescriptorHeap** ppSamplerHeap);

        void SetRootSrvCbvBuffers(ID3D12GraphicsCommandList* cmd);

        template <typename Func>
        void TransitionResources(Func fn);

        void SetDescriptorHeaps(ID3D12GraphicsCommandList* cmd, GfxDescriptorHeap* viewHeap, GfxDescriptorHeap* samplerHeap);
    };

    template <typename _PipelineTraits>
    void GfxViewCache<_PipelineTraits>::SetSrvCbvBuffer(size_t type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element, bool isConstantBuffer)
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
        m_StagedResourceStates[buffer->GetUnderlyingResource()] |= state;
    }

    template <typename _PipelineTraits>
    void GfxViewCache<_PipelineTraits>::SetSrvTexture(size_t type, uint32_t index, GfxTexture* texture, GfxTextureElement element)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetSrv(element);
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
        m_StagedResourceStates[texture->GetUnderlyingResource()] |= state;
    }

    template <typename _PipelineTraits>
    void GfxViewCache<_PipelineTraits>::SetUavBuffer(size_t type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = buffer->GetUav(element);
        m_SrvUavCache[type].Set(static_cast<size_t>(index), offlineDescriptor);

        // 记录状态，之后会统一使用 ResourceBarrier
        m_StagedResourceStates[buffer->GetUnderlyingResource()] |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    template <typename _PipelineTraits>
    void GfxViewCache<_PipelineTraits>::SetUavTexture(size_t type, uint32_t index, GfxTexture* texture, GfxTextureElement element)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetUav(element);
        m_SrvUavCache[type].Set(static_cast<size_t>(index), offlineDescriptor);

        // 记录状态，之后会统一使用 ResourceBarrier
        m_StagedResourceStates[texture->GetUnderlyingResource()] |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    template <typename _PipelineTraits>
    void GfxViewCache<_PipelineTraits>::SetSampler(size_t type, uint32_t index, GfxTexture* texture)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetSampler();
        m_SamplerCache[type].Set(static_cast<size_t>(index), offlineDescriptor);
    }

    template <typename _PipelineTraits>
    void GfxViewCache<_PipelineTraits>::SetRootSrvCbvBuffers(ID3D12GraphicsCommandList* cmd)
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
                    (cmd->*_PipelineTraits::SetRootConstantBufferView)(static_cast<UINT>(i), address);
                }
                else
                {
                    (cmd->*_PipelineTraits::SetRootShaderResourceView)(static_cast<UINT>(i), address);
                }
            }

            cache.Apply();
        }
    }

    template <typename _PipelineTraits>
    template <typename Func>
    void GfxViewCache<_PipelineTraits>::TransitionResources(Func fn)
    {
        for (const auto& [resource, state] : m_StagedResourceStates)
        {
            fn(resource, state);
        }

        m_StagedResourceStates.clear();
    }

    template <typename _PipelineTraits>
    void GfxViewCache<_PipelineTraits>::SetDescriptorHeaps(ID3D12GraphicsCommandList* cmd, GfxDescriptorHeap* viewHeap, GfxDescriptorHeap* samplerHeap)
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

    template <typename _PipelineTraits>
    void GfxViewCache<_PipelineTraits>::SetRootDescriptorTablesAndHeaps(
        ID3D12GraphicsCommandList* cmd,
        RootSignatureType* rootSignature,
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
                std::optional<uint32_t> srvUavTableRootParamIndex = rootSignature->GetSrvUavTableRootParamIndex(i);
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
                std::optional<uint32_t> samplerTableRootParamIndex = rootSignature->GetSamplerTableRootParamIndex(i);
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
                uint32_t rootParamIndex = *rootSignature->GetSrvUavTableRootParamIndex(i);
                (cmd->*_PipelineTraits::SetRootDescriptorTable)(static_cast<UINT>(rootParamIndex), srvUavTables[i]);
            }

            if (hasSampler && numSamplers[i] > 0)
            {
                uint32_t rootParamIndex = *rootSignature->GetSamplerTableRootParamIndex(i);
                (cmd->*_PipelineTraits::SetRootDescriptorTable)(static_cast<UINT>(rootParamIndex), samplerTables[i]);
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

    template <typename _PipelineTraits>
    void GfxViewCache<_PipelineTraits>::SetRootSignature(ID3D12GraphicsCommandList* cmd, RootSignatureType* rootSignature)
    {
        // 内部的 ID3D12RootSignature 是复用的，如果 ID3D12RootSignature 变了，说明根签名发生了结构上的变化
        if (m_CurrentRootSignature != rootSignature->GetD3DRootSignature())
        {
            // 删掉旧的 view
            for (auto& cache : m_SrvCbvBufferCache) cache.Reset();
            for (auto& cache : m_SrvUavCache) cache.Reset();
            for (auto& cache : m_SamplerCache) cache.Reset();
            m_StagedResourceStates.clear();

            // 设置 root signature
            m_CurrentRootSignature = rootSignature->GetD3DRootSignature();
            (cmd->*_PipelineTraits::SetRootSignature)(m_CurrentRootSignature);
        }
    }
}
