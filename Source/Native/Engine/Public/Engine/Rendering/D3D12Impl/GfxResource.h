#pragma once

#include "Engine/Object.h"
#include "Engine/Memory/Allocator.h"
#include <directx/d3dx12.h>
#include <wrl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

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

        bool IsHeapCpuAccessible() const;

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

    struct GfxPlacedResourceAllocatorDesc
    {
        uint32_t DefaultMaxBlockSize;
        D3D12_HEAP_TYPE HeapType;
        D3D12_HEAP_FLAGS HeapFlags;
        bool MSAA;
    };

    class GfxPlacedResourceAllocator : public GfxResourceAllocator
    {
    public:
        GfxPlacedResourceAllocator(GfxDevice* device, const std::string& name, const GfxPlacedResourceAllocatorDesc& desc);

        RefCountPtr<GfxResource> Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) override;

        void Release(const GfxResourceAllocation& allocation) override;

    private:
        bool m_MSAA;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Heap>> m_HeapPages;
        std::unique_ptr<MultiBuddyAllocator> m_Allocator;
    };
}
