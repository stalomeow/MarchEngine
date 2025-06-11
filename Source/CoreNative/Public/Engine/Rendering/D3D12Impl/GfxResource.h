#pragma once

#include "Engine/Memory/Allocator.h"
#include "Engine/Memory/RefCounting.h"
#include <d3dx12.h>
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
        GfxDevice* m_Device;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        uint32_t m_SubresourceCount;

        GfxResourceAllocator* m_Allocator; // 可以没有
        GfxResourceAllocation m_Allocation;

        bool m_IsStateLocked;
        bool m_AllStatesSame;
        D3D12_RESOURCE_STATES m_State; // 整个 Resource 的 State，当所有 Subresource State 一致时才有效
        std::unique_ptr<D3D12_RESOURCE_STATES[]> m_SubresourceStates;

        void* m_NsightAftermathHandle;

    public:
        GfxResource(GfxDevice* device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state);
        GfxResource(GfxResourceAllocator* allocator, const GfxResourceAllocation& allocation, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state);
        ~GfxResource();

        bool IsHeapCpuAccessible() const;

        void LockState(bool lock);
        D3D12_RESOURCE_STATES GetState(uint32_t subresource) const;
        bool AreAllStatesEqualTo(D3D12_RESOURCE_STATES states) const;
        bool HasAllStates(D3D12_RESOURCE_STATES states) const;
        bool HasAnyStates(D3D12_RESOURCE_STATES states) const;
        void SetState(D3D12_RESOURCE_STATES state);
        void SetState(D3D12_RESOURCE_STATES state, uint32_t subresource);

        GfxDevice* GetDevice() const { return m_Device; }
        GfxResourceAllocator* GetAllocator() const { return m_Allocator; }
        ID3D12Resource* GetD3DResource() const { return m_Resource.Get(); }
        D3D12_RESOURCE_DESC GetD3DResourceDesc() const { return m_Resource->GetDesc(); }
        uint32_t GetSubresourceCount() const { return m_SubresourceCount; }
        bool IsStateLocked() const { return m_IsStateLocked; }
        bool AreAllSubresourceStatesSame() const { return m_AllStatesSame; }
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
