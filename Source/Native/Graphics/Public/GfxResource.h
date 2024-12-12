#pragma once

#include "Allocator.h"
#include <directx/d3dx12.h>
#include <wrl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace march
{
    class GfxDevice;
    class GfxResource;

    union GfxResourceAllocation
    {
        BuddyAllocation Buddy;
    };

    class GfxResourceAllocator
    {
        friend GfxResource;

    public:
        virtual ~GfxResourceAllocator() = default;

        virtual std::unique_ptr<GfxResource> Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) = 0;

        GfxDevice* GetDevice() const { return m_Device; }
        D3D12_HEAP_PROPERTIES GetHeapProperties() const { return CD3DX12_HEAP_PROPERTIES(m_HeapType); }
        D3D12_HEAP_FLAGS GetHeapFlags() const { return m_HeapFlags; }

    protected:
        GfxResourceAllocator(GfxDevice* device, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags);

        std::unique_ptr<GfxResource> CreateResource(
            const std::string& name,
            Microsoft::WRL::ComPtr<ID3D12Resource> res,
            D3D12_RESOURCE_STATES initialState,
            const GfxResourceAllocation& allocation);

        virtual void Release(const GfxResourceAllocation& allocation) = 0;

    private:
        GfxDevice* m_Device;
        const D3D12_HEAP_TYPE m_HeapType;
        const D3D12_HEAP_FLAGS m_HeapFlags;
    };

    class GfxResource final
    {
        friend GfxResourceAllocator;

    public:
        ~GfxResource();

        GfxDevice* GetDevice() const { return m_Allocator->GetDevice(); }
        ID3D12Resource* GetD3DResource() const { return m_Resource.Get(); }
        D3D12_RESOURCE_STATES GetState() const { return m_State; }
        void SetState(D3D12_RESOURCE_STATES state) { m_State = state; }

        GfxResource() = default;

        GfxResource(const GfxResource&) = delete;
        GfxResource& operator=(const GfxResource&) = delete;

        GfxResource(GfxResource&&) = default;
        GfxResource& operator=(GfxResource&&) = default;

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        D3D12_RESOURCE_STATES m_State;
        GfxResourceAllocator* m_Allocator;
        GfxResourceAllocation m_Allocation;
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

        std::unique_ptr<GfxResource> Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) override;

    protected:
        void AppendNewAllocator(uint32_t maxBlockSize) override;
        void Release(const GfxResourceAllocation& allocation) override;

    private:
        const bool m_MSAA;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Heap>> m_Heaps;
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

        std::unique_ptr<GfxResource> Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) override;

    protected:
        void Release(const GfxResourceAllocation& allocation) override;
    };
}
