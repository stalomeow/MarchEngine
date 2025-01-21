#pragma once

#include "Engine/Object.h"
#include "Engine/Allocator.h"
#include <directx/d3dx12.h>
#include <wrl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <queue>

namespace march
{
    class GfxDevice;
    class GfxResourceAllocator;

    // 记录不同分配器分配的信息
    union GfxResourceAllocation
    {
        BuddyAllocation Buddy;
    };

    class GfxResource final : public ThreadSafeRefCountedObject
    {
    public:
        GfxResource(GfxDevice* device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state);
        GfxResource(GfxResourceAllocator* allocator, const GfxResourceAllocation& allocation, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state);
        ~GfxResource();

        GfxDevice* GetDevice() const { return m_Device; }
        GfxResourceAllocator* GetAllocator() const { return m_Allocator; }
        ID3D12Resource* GetD3DResource() const { return m_Resource.Get(); }
        D3D12_RESOURCE_DESC GetD3DResourceDesc() const { return m_Resource->GetDesc(); }

        D3D12_RESOURCE_STATES GetState() const { return m_State; }
        void SetState(D3D12_RESOURCE_STATES state) { m_State = state; }

    private:
        GfxDevice* m_Device;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        D3D12_RESOURCE_STATES m_State;

        GfxResourceAllocator* m_Allocator; // 可以没有
        GfxResourceAllocation m_Allocation;
    };

    class GfxResourceAllocator
    {
    public:
        virtual ~GfxResourceAllocator() = default;

        virtual RefCountPtr<GfxResource> Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) = 0;

        virtual void Release(const GfxResourceAllocation& allocation) = 0;

        GfxDevice* GetDevice() const { return m_Device; }
        D3D12_HEAP_PROPERTIES GetHeapProperties() const { return CD3DX12_HEAP_PROPERTIES(m_HeapType); }
        D3D12_HEAP_FLAGS GetHeapFlags() const { return m_HeapFlags; }
        bool IsHeapCpuAccessible() const { return CD3DX12_HEAP_PROPERTIES(GetHeapProperties()).IsCPUAccessible(); }

    protected:
        GfxResourceAllocator(GfxDevice* device, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags);

        RefCountPtr<GfxResource> MakeResource(
            const std::string& name,
            Microsoft::WRL::ComPtr<ID3D12Resource> resource,
            D3D12_RESOURCE_STATES initialState,
            const GfxResourceAllocation& allocation);

    private:
        GfxDevice* m_Device;
        const D3D12_HEAP_TYPE m_HeapType;
        const D3D12_HEAP_FLAGS m_HeapFlags;
    };

    struct GfxCommittedResourceAllocatorDesc
    {
        D3D12_HEAP_TYPE HeapType;
        D3D12_HEAP_FLAGS HeapFlags;
    };

    class GfxCommittedResourceAllocator : public GfxResourceAllocator
    {
    public:
        GfxCommittedResourceAllocator(GfxDevice* device, const GfxCommittedResourceAllocatorDesc& desc);

        RefCountPtr<GfxResource> Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) override;

        void Release(const GfxResourceAllocation& allocation) override;
    };

    struct GfxPlacedResourceMultiBuddyAllocatorDesc
    {
        uint32_t DefaultMaxBlockSize;
        D3D12_HEAP_TYPE HeapType;
        D3D12_HEAP_FLAGS HeapFlags;
        bool MSAA;
    };

    class GfxPlacedResourceMultiBuddyAllocator : public GfxResourceAllocator, protected MultiBuddyAllocator
    {
    public:
        GfxPlacedResourceMultiBuddyAllocator(GfxDevice* device, const std::string& name, const GfxPlacedResourceMultiBuddyAllocatorDesc& desc);

        RefCountPtr<GfxResource> Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) override;

        void Release(const GfxResourceAllocation& allocation) override;

    protected:
        void AppendNewAllocator(uint32_t maxBlockSize) override;

    private:
        const bool m_MSAA;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Heap>> m_Heaps;
    };

    struct GfxBufferMultiBuddySubAllocatorDesc
    {
        uint32_t MinBlockSize;
        uint32_t DefaultMaxBlockSize;
        bool UnorderedAccess;
        D3D12_RESOURCE_STATES InitialResourceState;
    };

    // 用于分配 SubBuffer
    class GfxBufferMultiBuddySubAllocator : public GfxBufferSubAllocator, protected MultiBuddyAllocator
    {
    public:
        GfxBufferMultiBuddySubAllocator(const std::string& name, const GfxBufferMultiBuddySubAllocatorDesc& desc, GfxCompleteResourceAllocator* bufferAllocator);

        GfxResourceSpan Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) override;

        void DeferredRelease(const GfxResourceAllocation& allocation) override;
        void CleanUpAllocations() override;

    protected:
        void AppendNewAllocator(uint32_t maxBlockSize) override;

    private:
        const bool m_UnorderedAccess;
        const D3D12_RESOURCE_STATES m_InitialResourceState;

        std::vector<GfxResourceSpan> m_Buffers;
        std::queue<std::pair<uint64_t, GfxResourceAllocation>> m_ReleaseQueue;
    };

    struct GfxBufferLinearSubAllocatorDesc
    {
        uint32_t PageSize;
        bool UnorderedAccess;
        D3D12_RESOURCE_STATES InitialResourceState;
    };

    // 用于分配 SubBuffer，但只有一帧有效
    class GfxBufferLinearSubAllocator : public GfxBufferSubAllocator, protected LinearAllocator
    {
    public:
        GfxBufferLinearSubAllocator(
            const std::string& name,
            const GfxBufferLinearSubAllocatorDesc& desc,
            GfxCompleteResourceAllocator* pageAllocator,
            GfxCompleteResourceAllocator* largePageAllocator);

        GfxResourceSpan Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) override;

        void DeferredRelease(const GfxResourceAllocation& allocation) override;
        void CleanUpAllocations() override;

    protected:
        size_t RequestPage(uint32_t sizeInBytes, bool large, bool* pOutIsNew) override;

    private:
        const bool m_UnorderedAccess;
        const D3D12_RESOURCE_STATES m_InitialResourceState;
        GfxCompleteResourceAllocator* m_LargePageAllocator;

        std::vector<GfxResourceSpan> m_Pages;
        std::vector<GfxResourceSpan> m_LargePages;
        std::queue<std::pair<uint64_t, GfxResourceSpan>> m_ReleaseQueue;
    };
}
