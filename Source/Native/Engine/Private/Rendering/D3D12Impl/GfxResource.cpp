#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxResource.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Debug.h"
#include <stdexcept>
#include <assert.h>

using namespace Microsoft::WRL;

namespace march
{
    static uint32_t CalcSubresourceCount(ID3D12Device* device, ID3D12Resource* resource)
    {
        D3D12_RESOURCE_DESC desc = resource->GetDesc();

        switch (desc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_UNKNOWN:
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            return 1;
        }

        uint32_t mipLevels = static_cast<uint32_t>(desc.MipLevels);
        uint32_t arraySize = static_cast<uint32_t>(desc.DepthOrArraySize);
        uint32_t planeCount = static_cast<uint32_t>(D3D12GetFormatPlaneCount(device, desc.Format));
        return mipLevels * arraySize * planeCount;
    }

    GfxResource::GfxResource(GfxDevice* device, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state)
        : m_Device(device)
        , m_Resource(resource)
        , m_Allocator(nullptr)
        , m_Allocation{}
        , m_IsStateLocked(false)
        , m_AllStatesSame(true)
        , m_State(state)
        , m_SubresourceStates(nullptr)
    {
        m_SubresourceCount = CalcSubresourceCount(m_Device->GetD3DDevice4(), m_Resource.Get());
        assert(m_SubresourceCount >= 1);
    }

    GfxResource::GfxResource(GfxResourceAllocator* allocator, const GfxResourceAllocation& allocation, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state)
        : m_Device(allocator->GetDevice())
        , m_Resource(resource)
        , m_Allocator(allocator)
        , m_Allocation(allocation)
        , m_IsStateLocked(false)
        , m_AllStatesSame(true)
        , m_State(state)
        , m_SubresourceStates(nullptr)
    {
        m_SubresourceCount = CalcSubresourceCount(m_Device->GetD3DDevice4(), m_Resource.Get());
        assert(m_SubresourceCount >= 1);
    }

    GfxResource::~GfxResource()
    {
#ifdef ENABLE_GFX_DEBUG_NAME
        //if (m_Resource)
        //{
        //    wchar_t name[256];
        //    UINT size = sizeof(name);
        //    if (SUCCEEDED(m_Resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name)))
        //    {
        //        LOG_TRACE(L"Release D3D12 Resource {}", name);
        //    }
        //}
#endif

        m_Device = nullptr;
        m_Resource = nullptr;

        if (m_Allocator)
        {
            m_Allocator->Release(m_Allocation);
            m_Allocator = nullptr;
        }
    }

    bool GfxResource::IsHeapCpuAccessible() const
    {
        D3D12_HEAP_PROPERTIES heapProperties{};

        if (SUCCEEDED(m_Resource->GetHeapProperties(&heapProperties, nullptr)))
        {
            return CD3DX12_HEAP_PROPERTIES(heapProperties).IsCPUAccessible();
        }

        return false;
    }

    void GfxResource::LockState(bool lock)
    {
        m_IsStateLocked = lock;
    }

    D3D12_RESOURCE_STATES GfxResource::GetState(uint32_t subresource) const
    {
        return m_AllStatesSame ? m_State : m_SubresourceStates[subresource];
    }

    bool GfxResource::AreAllStatesEqualTo(D3D12_RESOURCE_STATES states) const
    {
        if (m_AllStatesSame)
        {
            return m_State == states;
        }

        for (uint32_t i = 0; i < m_SubresourceCount; i++)
        {
            if (m_SubresourceStates[i] != states)
            {
                return false;
            }
        }

        return true;
    }

    bool GfxResource::HasAllStates(D3D12_RESOURCE_STATES states) const
    {
        if (m_AllStatesSame)
        {
            return (m_State & states) == states;
        }

        for (uint32_t i = 0; i < m_SubresourceCount; i++)
        {
            if ((m_SubresourceStates[i] & states) != states)
            {
                return false;
            }
        }

        return true;
    }

    bool GfxResource::HasAnyStates(D3D12_RESOURCE_STATES states) const
    {
        if (m_AllStatesSame)
        {
            return (m_State & states) != 0;
        }

        for (uint32_t i = 0; i < m_SubresourceCount; i++)
        {
            if ((m_SubresourceStates[i] & states) != 0)
            {
                return true;
            }
        }

        return false;
    }

    void GfxResource::SetState(D3D12_RESOURCE_STATES state)
    {
        if (m_IsStateLocked)
        {
            throw GfxException("Resource state is locked");
        }

        m_AllStatesSame = true;
        m_State = state;

        if (m_SubresourceStates)
        {
            std::fill_n(m_SubresourceStates.get(), m_SubresourceCount, state);
        }
    }

    void GfxResource::SetState(D3D12_RESOURCE_STATES state, uint32_t subresource)
    {
        if (subresource >= m_SubresourceCount)
        {
            throw GfxException("Subresource index out of range");
        }

        if (m_SubresourceCount == 1)
        {
            SetState(state);
            return;
        }

        if (m_IsStateLocked)
        {
            throw GfxException("Resource state is locked");
        }

        if (m_AllStatesSame)
        {
            if (m_State == state)
            {
                return;
            }

            if (!m_SubresourceStates)
            {
                m_SubresourceStates = std::make_unique<D3D12_RESOURCE_STATES[]>(m_SubresourceCount);
                std::fill_n(m_SubresourceStates.get(), m_SubresourceCount, m_State);
            }

            m_AllStatesSame = false;
        }

        m_SubresourceStates[subresource] = state;
    }

    GfxResourceAllocator::GfxResourceAllocator(GfxDevice* device, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags)
        : m_Device(device)
        , m_HeapType(heapType)
        , m_HeapFlags(heapFlags)
    {
    }

    RefCountPtr<GfxResource> GfxResourceAllocator::MakeResource(
        const std::string& name,
        ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES initialState,
        const GfxResourceAllocation& allocation)
    {
        GfxUtils::SetName(resource.Get(), name);
        return MARCH_MAKE_REF(GfxResource, this, allocation, resource, initialState);
    }

    GfxCommittedResourceAllocator::GfxCommittedResourceAllocator(GfxDevice* device, const GfxCommittedResourceAllocatorDesc& desc)
        : GfxResourceAllocator(device, desc.HeapType, desc.HeapFlags)
    {
    }

    RefCountPtr<GfxResource> GfxCommittedResourceAllocator::Allocate(
        const std::string& name,
        const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue)
    {
        ID3D12Device4* device = GetDevice()->GetD3DDevice4();
        D3D12_HEAP_PROPERTIES heapProperties = GetHeapProperties();

        ComPtr<ID3D12Resource> resource = nullptr;
        CHECK_HR(device->CreateCommittedResource(&heapProperties, GetHeapFlags(), pDesc, initialState, pOptimizedClearValue, IID_PPV_ARGS(&resource)));
        return MakeResource(name, resource, initialState, GfxResourceAllocation{});
    }

    void GfxCommittedResourceAllocator::Release(const GfxResourceAllocation& allocation)
    {
        // Do Nothing
    }

    static constexpr uint32_t GetResourcePlacementAlignment(bool msaa)
    {
        return msaa ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    }

    GfxPlacedResourceAllocator::GfxPlacedResourceAllocator(GfxDevice* device, const std::string& name, const GfxPlacedResourceAllocatorDesc& desc)
        : GfxResourceAllocator(device, desc.HeapType, desc.HeapFlags)
        , m_MSAA(desc.MSAA)
        , m_HeapPages{}
    {
        auto appendPageFunc = [this](uint32_t sizeInBytes)
        {
            D3D12_HEAP_DESC desc{};
            desc.SizeInBytes = static_cast<UINT64>(sizeInBytes);
            desc.Properties = GetHeapProperties();
            desc.Alignment = static_cast<UINT64>(GetResourcePlacementAlignment(m_MSAA));
            desc.Flags = GetHeapFlags();

            ComPtr<ID3D12Heap> heap = nullptr;
            CHECK_HR(GetDevice()->GetD3DDevice4()->CreateHeap(&desc, IID_PPV_ARGS(&heap)));
            m_HeapPages.push_back(heap);
        };

        uint32_t minBlockSize = GetResourcePlacementAlignment(desc.MSAA);
        m_Allocator = std::make_unique<MultiBuddyAllocator>(name, minBlockSize, desc.DefaultMaxBlockSize, appendPageFunc);
    }

    RefCountPtr<GfxResource> GfxPlacedResourceAllocator::Allocate(
        const std::string& name,
        const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue)
    {
        ID3D12Device4* device = GetDevice()->GetD3DDevice4();
        D3D12_RESOURCE_ALLOCATION_INFO info = device->GetResourceAllocationInfo(0, 1, pDesc);
        uint32_t sizeInBytes = static_cast<uint32_t>(info.SizeInBytes);
        uint32_t alignment = static_cast<uint32_t>(info.Alignment);

        size_t pageIndex = 0;
        GfxResourceAllocation allocation{};

        if (std::optional<uint32_t> offset = m_Allocator->Allocate(sizeInBytes, alignment, &pageIndex, &allocation.Buddy))
        {
            ID3D12Heap* heap = m_HeapPages[pageIndex].Get();
            UINT64 heapOffset = static_cast<UINT64>(*offset);

            ComPtr<ID3D12Resource> resource = nullptr;
            CHECK_HR(device->CreatePlacedResource(heap, heapOffset, pDesc, initialState, pOptimizedClearValue, IID_PPV_ARGS(&resource)));
            return MakeResource(name, resource, initialState, allocation);
        }

        return nullptr;
    }

    void GfxPlacedResourceAllocator::Release(const GfxResourceAllocation& allocation)
    {
        m_Allocator->Release(allocation.Buddy);
    }
}
