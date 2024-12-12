#include "GfxResource.h"
#include "GfxDevice.h"
#include "GfxUtils.h"
#include "Debug.h"

#undef min
#undef max

using namespace Microsoft::WRL;

namespace march
{
    GfxResourceAllocator::GfxResourceAllocator(GfxDevice* device, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags)
        : m_Device(device)
        , m_HeapType(heapType)
        , m_HeapFlags(heapFlags)
        , m_ReleaseQueue{}
    {
    }

    std::unique_ptr<GfxResource> GfxResourceAllocator::CreateResource(
        const std::string& name,
        ComPtr<ID3D12Resource> res,
        D3D12_RESOURCE_STATES initialState,
        const GfxResourceAllocation& allocation)
    {
        GfxUtils::SetName(res.Get(), name);

        std::unique_ptr<GfxResource> result = std::make_unique<GfxResource>(m_Device, res, initialState);
        result->m_Allocator = this;
        result->m_Allocation = allocation;
        return result;
    }

    void GfxResourceAllocator::DeferredRelease(const GfxResourceAllocation& allocation)
    {
        m_ReleaseQueue.emplace(m_Device->GetNextFrameFence(), allocation);
    }

    void GfxResourceAllocator::CleanUpAllocations()
    {
        while (!m_ReleaseQueue.empty() && m_Device->IsFrameFenceCompleted(m_ReleaseQueue.front().first, /* useCache */ true))
        {
            Release(m_ReleaseQueue.front().second);
            m_ReleaseQueue.pop();
        }
    }

    GfxResource::GfxResource(GfxDevice* device, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state)
        : m_Device(device)
        , m_Resource(resource)
        , m_State(state)
        , m_Allocator(nullptr)
        , m_Allocation{}
    {
    }

    GfxResource::~GfxResource()
    {
        if (m_Allocator != nullptr)
        {
            m_Allocator->DeferredRelease(m_Allocation);
        }

        m_Device->DeferredRelease(m_Resource);
    }

    static constexpr uint32_t GetResourcePlacementAlignment(bool msaa)
    {
        return msaa ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    }

    GfxPlacedResourceMultiBuddyAllocator::GfxPlacedResourceMultiBuddyAllocator(GfxDevice* device, const std::string& name, const GfxPlacedResourceMultiBuddyAllocatorDesc& desc)
        : GfxResourceAllocator(device, desc.HeapType, desc.HeapFlags)
        , MultiBuddyAllocator(name, GetResourcePlacementAlignment(desc.MSAA), desc.DefaultMaxBlockSize)
        , m_MSAA(desc.MSAA)
        , m_Heaps{}
    {
    }

    std::unique_ptr<GfxResource> GfxPlacedResourceMultiBuddyAllocator::Allocate(
        const std::string& name,
        const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue)
    {
        ID3D12Device4* device = GetDevice()->GetD3DDevice4();
        D3D12_RESOURCE_ALLOCATION_INFO info = device->GetResourceAllocationInfo(0, 1, pDesc);
        uint32_t sizeInBytes = static_cast<uint32_t>(info.SizeInBytes);
        uint32_t alignment = static_cast<uint32_t>(info.Alignment);

        size_t allocatorIndex = 0;
        GfxResourceAllocation allocation{};

        if (std::optional<uint32_t> offset = MultiBuddyAllocator::Allocate(sizeInBytes, alignment, allocatorIndex, allocation.Buddy))
        {
            ComPtr<ID3D12Resource> resource = nullptr;

            ID3D12Heap* heap = m_Heaps[allocatorIndex].Get();
            UINT64 heapOffset = static_cast<UINT64>(*offset);
            GFX_HR(device->CreatePlacedResource(heap, heapOffset, pDesc, initialState, pOptimizedClearValue, IID_PPV_ARGS(&resource)));
            return CreateResource(name, resource, initialState, allocation);
        }

        return nullptr;
    }

    void GfxPlacedResourceMultiBuddyAllocator::AppendNewAllocator(uint32_t maxBlockSize)
    {
        MultiBuddyAllocator::AppendNewAllocator(maxBlockSize); // Call base

        ID3D12Device4* device = GetDevice()->GetD3DDevice4();
        ComPtr<ID3D12Heap> heap = nullptr;

        D3D12_HEAP_DESC desc{};
        desc.SizeInBytes = static_cast<UINT64>(maxBlockSize);
        desc.Properties = GetHeapProperties();
        desc.Alignment = static_cast<UINT64>(GetResourcePlacementAlignment(m_MSAA));
        desc.Flags = GetHeapFlags();

        GFX_HR(device->CreateHeap(&desc, IID_PPV_ARGS(&heap)));
        m_Heaps.push_back(heap);
    }

    void GfxPlacedResourceMultiBuddyAllocator::Release(const GfxResourceAllocation& allocation)
    {
        BuddyAllocator* allocator = allocation.Buddy.Owner;
        allocator->Release(allocation.Buddy);
    }

    GfxCommittedResourceAllocator::GfxCommittedResourceAllocator(GfxDevice* device, const GfxCommittedResourceAllocatorDesc& desc)
        : GfxResourceAllocator(device, desc.HeapType, desc.HeapFlags)
    {
    }

    std::unique_ptr<GfxResource> GfxCommittedResourceAllocator::Allocate(
        const std::string& name,
        const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue)
    {
        ID3D12Device4* device = GetDevice()->GetD3DDevice4();
        D3D12_HEAP_PROPERTIES heapProperties = GetHeapProperties();
        ComPtr<ID3D12Resource> resource = nullptr;

        GFX_HR(device->CreateCommittedResource(&heapProperties, GetHeapFlags(), pDesc, initialState, pOptimizedClearValue, IID_PPV_ARGS(&resource)));
        return CreateResource(name, resource, initialState, GfxResourceAllocation{});
    }

    void GfxCommittedResourceAllocator::Release(const GfxResourceAllocation& allocation)
    {
        // Do nothing
    }

    GfxSubBufferMultiBuddyAllocator::GfxSubBufferMultiBuddyAllocator(const std::string& name, const GfxSubBufferMultiBuddyAllocatorDesc& desc, GfxResourceAllocator* bufferAllocator)
        : MultiBuddyAllocator(name, desc.MinBlockSize, desc.DefaultMaxBlockSize)
        , m_ResourceFlags(desc.ResourceFlags)
        , m_InitialResourceState(desc.InitialResourceState)
        , m_BufferAllocator(bufferAllocator)
        , m_Buffers{}
    {
    }

    void GfxSubBufferMultiBuddyAllocator::AppendNewAllocator(uint32_t maxBlockSize)
    {
        MultiBuddyAllocator::AppendNewAllocator(maxBlockSize); // Call base

        auto desc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(maxBlockSize), m_ResourceFlags);
        m_Buffers.push_back(m_BufferAllocator->Allocate(GetName() + "Buffer", &desc, m_InitialResourceState));
    }
}
