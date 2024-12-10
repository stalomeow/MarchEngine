#pragma once

#include <directx/d3dx12.h>
#include <wrl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include <optional>
#include <memory>

namespace march
{
    class GfxDevice;
    class BuddyAllocator;

    struct BuddyAllocation
    {
        BuddyAllocator* Owner;
        uint32_t Offset;
        uint32_t Order;
    };

    class BuddyAllocator
    {
    public:
        BuddyAllocator(uint32_t minBlockSize, uint32_t maxBlockSize);
        virtual ~BuddyAllocator() = default;

        void Reset();
        std::optional<uint32_t> Allocate(uint32_t sizeInBytes, uint32_t alignment, BuddyAllocation& outAllocation);
        void Release(const BuddyAllocation& allocation);

    private:
        const uint32_t m_MinBlockSize;
        const uint32_t m_MaxBlockSize;
        uint32_t m_MaxOrder;
        std::vector<std::set<uint32_t>> m_FreeBlocks;

        uint32_t SizeToUnitSize(uint32_t size) const;
        uint32_t UnitSizeToOrder(uint32_t size) const;
        uint32_t OrderToUnitSize(uint32_t order) const;
        uint32_t GetBuddyOffset(uint32_t offset, uint32_t size) const;

        std::optional<uint32_t> AllocateBlock(uint32_t order);
        void ReleaseBlock(uint32_t offset, uint32_t order);
    };

    class MultiBuddyAllocator
    {
    public:
        MultiBuddyAllocator(const std::string& name, uint32_t minBlockSize, uint32_t defaultMaxBlockSize);
        virtual ~MultiBuddyAllocator() = default;

        std::optional<uint32_t> Allocate(uint32_t sizeInBytes, uint32_t alignment, size_t& outAllocatorIndex, BuddyAllocation& outAllocation);

        const std::string& GetName() const { return m_Name; }

    protected:
        virtual void AppendNewAllocator(uint32_t maxBlockSize);

    private:
        std::string m_Name;
        uint32_t m_MinBlockSize;
        uint32_t m_DefaultMaxBlockSize;
        std::vector<std::unique_ptr<BuddyAllocator>> m_Allocators;
    };

    class GfxResource;

    union GfxResourceAllocation
    {
        BuddyAllocation Buddy;
    };

    class GfxResourceAllocator
    {
        friend GfxResource;

    public:
        GfxDevice* GetDevice() const { return m_Device; }

        virtual ~GfxResourceAllocator() = default;

        virtual std::unique_ptr<GfxResource> Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) = 0;

    protected:
        GfxResourceAllocator(GfxDevice* device) : m_Device(device) {}

        std::unique_ptr<GfxResource> CreateResource(
            const std::string& name,
            Microsoft::WRL::ComPtr<ID3D12Resource> res,
            D3D12_RESOURCE_STATES initialState,
            const GfxResourceAllocation& allocation);

        virtual void Release(const GfxResourceAllocation& allocation) = 0;

    private:
        GfxDevice* m_Device;
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
        const D3D12_HEAP_TYPE m_HeapType;
        const D3D12_HEAP_FLAGS m_HeapFlags;
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

    private:
        const D3D12_HEAP_TYPE m_HeapType;
        const D3D12_HEAP_FLAGS m_HeapFlags;
    };

    struct GfxSubBufferMultiBuddyAllocatorDesc
    {
        uint32_t MinBlockSize;
        uint32_t DefaultMaxBlockSize;
        D3D12_RESOURCE_FLAGS ResourceFlags;
        D3D12_RESOURCE_STATES InitialResourceState;
    };

    class GfxSubBufferMultiBuddyAllocator : protected MultiBuddyAllocator
    {
    public:
        GfxSubBufferMultiBuddyAllocator(const std::string& name, const GfxSubBufferMultiBuddyAllocatorDesc& desc, GfxResourceAllocator* bufferAllocator);

    protected:
        void AppendNewAllocator(uint32_t maxBlockSize) override;

    private:
        const D3D12_RESOURCE_FLAGS m_ResourceFlags;
        const D3D12_RESOURCE_STATES m_InitialResourceState;
        GfxResourceAllocator* m_BufferAllocator;
        std::vector<std::unique_ptr<GfxResource>> m_Buffers;
    };
}
