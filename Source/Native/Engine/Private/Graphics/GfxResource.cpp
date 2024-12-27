#include "pch.h"
#include "Engine/Graphics/GfxResource.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxUtils.h"
#include <assert.h>
#include <stdexcept>

using namespace Microsoft::WRL;

namespace march
{
    GfxResource::GfxResource(GfxDevice* device, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state)
        : m_Device(device)
        , m_Resource(resource)
        , m_State(state)
        , m_Allocator(nullptr)
        , m_Allocation{}
    {
    }

    GfxResource::GfxResource(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state, GfxResourceAllocator* allocator, const GfxResourceAllocation& allocation)
        : m_Device(allocator->GetDevice())
        , m_Resource(resource)
        , m_State(state)
        , m_Allocator(allocator)
        , m_Allocation(allocation)
    {
    }

    GfxResource::GfxResource(GfxResource&& other) noexcept
        : m_Device(std::exchange(other.m_Device, nullptr))
        , m_Resource(std::move(other.m_Resource))
        , m_State(other.m_State)
        , m_Allocator(std::exchange(other.m_Allocator, nullptr))
        , m_Allocation(other.m_Allocation)
    {
    }

    GfxResource& GfxResource::operator=(GfxResource&& other)
    {
        if (this != &other)
        {
            Release();

            m_Device = std::exchange(other.m_Device, nullptr);
            m_Resource = std::move(other.m_Resource);
            m_State = other.m_State;
            m_Allocator = std::exchange(other.m_Allocator, nullptr);
            m_Allocation = other.m_Allocation;
        }

        return *this;
    }

    void GfxResource::Release()
    {
        if (m_Device && m_Resource)
        {
            m_Device->DeferredRelease(m_Resource);
        }

        m_Device = nullptr;
        m_Resource = nullptr;

        if (m_Allocator)
        {
            m_Allocator->DeferredRelease(m_Allocation);
            m_Allocator = nullptr;
        }
    }

    GfxResourceSpan::GfxResourceSpan(std::shared_ptr<GfxResource> resource, uint32_t bufferOffset, uint32_t bufferSize)
        : m_Resource(resource)
        , m_Allocator(nullptr)
        , m_Allocation{}
        , m_BufferOffset(bufferOffset)
        , m_BufferSize(bufferSize)
    {
    }

    GfxResourceSpan::GfxResourceSpan(GfxResourceSpan&& other) noexcept
        : m_Resource(std::move(other.m_Resource))
        , m_Allocator(std::exchange(other.m_Allocator, nullptr))
        , m_Allocation(other.m_Allocation)
        , m_BufferOffset(std::exchange(other.m_BufferOffset, 0))
        , m_BufferSize(std::exchange(other.m_BufferSize, 0))
    {
    }

    GfxResourceSpan& GfxResourceSpan::operator=(GfxResourceSpan&& other)
    {
        if (this != &other)
        {
            Release();

            m_Resource = std::move(other.m_Resource);
            m_Allocator = std::exchange(other.m_Allocator, nullptr);
            m_Allocation = other.m_Allocation;
            m_BufferOffset = std::exchange(other.m_BufferOffset, 0);
            m_BufferSize = std::exchange(other.m_BufferSize, 0);
        }

        return *this;
    }

    void GfxResourceSpan::Release()
    {
        if (m_Allocator)
        {
            m_Allocator->DeferredRelease(m_Allocation);
            m_Allocator = nullptr;
        }

        m_Resource = nullptr;
        m_BufferOffset = 0;
        m_BufferSize = 0;
    }

    GfxResourceSpan GfxResourceSpan::MakeBufferSlice(uint32_t offset, uint32_t size, GfxResourceAllocator* allocator, const GfxResourceAllocation& allocation) const
    {
        if (offset + size > m_BufferSize)
        {
            throw std::out_of_range("GfxResourceSpan::MakeBufferSlice: size out of range");
        }

        GfxResourceSpan span{ m_Resource, m_BufferOffset + offset, size };
        span.m_Allocator = allocator;
        span.m_Allocation = allocation;
        return span;
    }

    GfxCompleteResourceAllocator::GfxCompleteResourceAllocator(GfxDevice* device, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags)
        : GfxResourceAllocator(device)
        , m_HeapType(heapType)
        , m_HeapFlags(heapFlags)
        , m_ReleaseQueue{}
    {
    }

    void GfxCompleteResourceAllocator::DeferredRelease(const GfxResourceAllocation& allocation)
    {
        m_ReleaseQueue.emplace(GetDevice()->GetNextFence(), allocation);
    }

    void GfxCompleteResourceAllocator::CleanUpAllocations()
    {
        while (!m_ReleaseQueue.empty() && GetDevice()->IsFenceCompleted(m_ReleaseQueue.front().first, /* useCache */ true))
        {
            Release(m_ReleaseQueue.front().second);
            m_ReleaseQueue.pop();
        }
    }

    GfxResourceSpan GfxCompleteResourceAllocator::CreateResourceSpan(
        const std::string& name,
        ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES initialState,
        const GfxResourceAllocation& allocation)
    {
        GfxUtils::SetName(resource.Get(), name);

        uint32_t bufferSize;

        if (D3D12_RESOURCE_DESC desc = resource->GetDesc(); desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            bufferSize = static_cast<uint32_t>(desc.Width);
        }
        else
        {
            bufferSize = 0;
        }

        return GfxResourceSpan{ std::make_shared<GfxResource>(resource, initialState, this, allocation), 0, bufferSize };
    }

    static constexpr uint32_t GetResourcePlacementAlignment(bool msaa)
    {
        return msaa ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    }

    GfxPlacedResourceMultiBuddyAllocator::GfxPlacedResourceMultiBuddyAllocator(GfxDevice* device, const std::string& name, const GfxPlacedResourceMultiBuddyAllocatorDesc& desc)
        : GfxCompleteResourceAllocator(device, desc.HeapType, desc.HeapFlags)
        , MultiBuddyAllocator(name, GetResourcePlacementAlignment(desc.MSAA), desc.DefaultMaxBlockSize)
        , m_MSAA(desc.MSAA)
        , m_Heaps{}
    {
    }

    GfxResourceSpan GfxPlacedResourceMultiBuddyAllocator::Allocate(
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

            return CreateResourceSpan(name, resource, initialState, allocation);
        }

        return GfxResourceSpan{};
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
        : GfxCompleteResourceAllocator(device, desc.HeapType, desc.HeapFlags)
    {
    }

    GfxResourceSpan GfxCommittedResourceAllocator::Allocate(
        const std::string& name,
        const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue)
    {
        ID3D12Device4* device = GetDevice()->GetD3DDevice4();
        D3D12_HEAP_PROPERTIES heapProperties = GetHeapProperties();
        ComPtr<ID3D12Resource> resource = nullptr;

        GFX_HR(device->CreateCommittedResource(&heapProperties, GetHeapFlags(), pDesc, initialState, pOptimizedClearValue, IID_PPV_ARGS(&resource)));
        return CreateResourceSpan(name, resource, initialState, {});
    }

    void GfxCommittedResourceAllocator::Release(const GfxResourceAllocation& allocation)
    {
        // Do nothing
    }

    GfxBufferSubAllocator::GfxBufferSubAllocator(GfxCompleteResourceAllocator* bufferAllocator)
        : GfxResourceAllocator(bufferAllocator->GetDevice())
        , m_BufferAllocator(bufferAllocator)
    {
    }

    GfxBufferMultiBuddySubAllocator::GfxBufferMultiBuddySubAllocator(const std::string& name, const GfxBufferMultiBuddySubAllocatorDesc& desc, GfxCompleteResourceAllocator* bufferAllocator)
        : GfxBufferSubAllocator(bufferAllocator)
        , MultiBuddyAllocator(name, desc.MinBlockSize, desc.DefaultMaxBlockSize)
        , m_UnorderedAccess(desc.UnorderedAccess)
        , m_InitialResourceState(desc.InitialResourceState)
        , m_Buffers{}
        , m_ReleaseQueue{}
    {
    }

    void GfxBufferMultiBuddySubAllocator::AppendNewAllocator(uint32_t maxBlockSize)
    {
        MultiBuddyAllocator::AppendNewAllocator(maxBlockSize); // Call base

        GfxCompleteResourceAllocator* allocator = GetBufferAllocator();
        UINT64 width = static_cast<UINT64>(maxBlockSize);
        D3D12_RESOURCE_FLAGS flags = m_UnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        m_Buffers.push_back(allocator->Allocate(GetName() + "Buffer", &CD3DX12_RESOURCE_DESC::Buffer(width, flags), m_InitialResourceState));
    }

    GfxResourceSpan GfxBufferMultiBuddySubAllocator::Allocate(uint32_t sizeInBytes, uint32_t dataPlacementAlignment)
    {
        size_t allocatorIndex = 0;
        GfxResourceAllocation allocation{};

        if (std::optional<uint32_t> offset = MultiBuddyAllocator::Allocate(sizeInBytes, dataPlacementAlignment, allocatorIndex, allocation.Buddy))
        {
            return m_Buffers[allocatorIndex].MakeBufferSlice(*offset, sizeInBytes, this, allocation);
        }

        return GfxResourceSpan{};
    }

    void GfxBufferMultiBuddySubAllocator::DeferredRelease(const GfxResourceAllocation& allocation)
    {
        m_ReleaseQueue.emplace(GetDevice()->GetNextFence(), allocation);
    }

    void GfxBufferMultiBuddySubAllocator::CleanUpAllocations()
    {
        while (!m_ReleaseQueue.empty() && GetDevice()->IsFenceCompleted(m_ReleaseQueue.front().first, /* useCache */ true))
        {
            const GfxResourceAllocation& allocation = m_ReleaseQueue.front().second;
            BuddyAllocator* allocator = allocation.Buddy.Owner;
            allocator->Release(allocation.Buddy);
            m_ReleaseQueue.pop();
        }
    }

    GfxBufferLinearSubAllocator::GfxBufferLinearSubAllocator(
        const std::string& name,
        const GfxBufferLinearSubAllocatorDesc& desc,
        GfxCompleteResourceAllocator* pageAllocator,
        GfxCompleteResourceAllocator* largePageAllocator)
        : GfxBufferSubAllocator(pageAllocator)
        , LinearAllocator(name, desc.PageSize)
        , m_UnorderedAccess(desc.UnorderedAccess)
        , m_InitialResourceState(desc.InitialResourceState)
        , m_LargePageAllocator(largePageAllocator)
        , m_Pages{}
        , m_LargePages{}
        , m_ReleaseQueue{}
    {
    }

    GfxResourceSpan GfxBufferLinearSubAllocator::Allocate(uint32_t sizeInBytes, uint32_t dataPlacementAlignment)
    {
        size_t pageIndex = 0;
        bool large = false;
        uint32_t offset = LinearAllocator::Allocate(sizeInBytes, dataPlacementAlignment, pageIndex, large);

        std::vector<GfxResourceSpan>& pages = large ? m_LargePages : m_Pages;
        return pages[pageIndex].MakeBufferSlice(offset, sizeInBytes, this, {});
    }

    void GfxBufferLinearSubAllocator::DeferredRelease(const GfxResourceAllocation& allocation)
    {
        // Do nothing
    }

    void GfxBufferLinearSubAllocator::CleanUpAllocations()
    {
        for (GfxResourceSpan& page : m_Pages)
        {
            m_ReleaseQueue.emplace(GetDevice()->GetNextFence(), std::move(page));
        }

        m_Pages.clear();
        m_LargePages.clear();
        LinearAllocator::Reset();
    }

    size_t GfxBufferLinearSubAllocator::RequestPage(uint32_t sizeInBytes, bool large, bool* pOutIsNew)
    {
        std::vector<GfxResourceSpan>& pages = large ? m_LargePages : m_Pages;

        if (!large && !m_ReleaseQueue.empty() && GetDevice()->IsFenceCompleted(m_ReleaseQueue.front().first, /* useCache */ true))
        {
            *pOutIsNew = false;

            GfxResourceSpan& p = m_ReleaseQueue.front().second;
            assert(p.GetBufferSize() == sizeInBytes);

            pages.push_back(std::move(p));
            m_ReleaseQueue.pop();
        }
        else
        {
            *pOutIsNew = true;

            GfxCompleteResourceAllocator* allocator;
            std::string name;

            if (large)
            {
                allocator = m_LargePageAllocator;
                name = GetName() + "Page (Large)";
            }
            else
            {
                allocator = GetBufferAllocator();
                name = GetName() + "Page";
            }

            D3D12_RESOURCE_FLAGS flags = m_UnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
            pages.push_back(allocator->Allocate(name, &CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, flags), m_InitialResourceState));
        }

        return pages.size() - 1;
    }
}
