#include "GfxResource.h"
#include "GfxDevice.h"
#include "GfxUtils.h"
#include "MathUtils.h"
#include "Debug.h"
#include <assert.h>

#undef min
#undef max

using namespace Microsoft::WRL;

namespace march
{
    BuddyAllocator::BuddyAllocator(uint32_t minBlockSize, uint32_t maxBlockSize)
        : m_MinBlockSize(minBlockSize)
        , m_MaxBlockSize(maxBlockSize)
        , m_FreeBlocks{}
    {
        assert(MathUtils::IsDivisible(maxBlockSize, minBlockSize));
        assert(MathUtils::IsPowerOfTwo(maxBlockSize / minBlockSize));

        m_MaxOrder = UnitSizeToOrder(SizeToUnitSize(maxBlockSize));
        Reset();
    }

    uint32_t BuddyAllocator::SizeToUnitSize(uint32_t size) const
    {
        return (size + (m_MinBlockSize - 1)) / m_MinBlockSize;
    }

    uint32_t BuddyAllocator::UnitSizeToOrder(uint32_t size) const
    {
        return static_cast<uint32_t>(MathUtils::Log2(size)); // ceil(log2(size))
    }

    uint32_t BuddyAllocator::OrderToUnitSize(uint32_t order) const
    {
        return static_cast<uint32_t>(1u) << order;
    }

    uint32_t BuddyAllocator::GetBuddyOffset(uint32_t offset, uint32_t size) const
    {
        return offset ^ size;
    }

    void BuddyAllocator::Reset()
    {
        m_FreeBlocks.clear();
        m_FreeBlocks.reserve(static_cast<size_t>(m_MaxOrder) + 1);
        m_FreeBlocks[m_MaxOrder].insert(0);
    }

    std::optional<uint32_t> BuddyAllocator::AllocateBlock(uint32_t order)
    {
        if (order > m_MaxOrder)
        {
            return std::nullopt;
        }

        if (auto it = m_FreeBlocks[order].begin(); it != m_FreeBlocks[order].end())
        {
            uint32_t offset = *it;
            m_FreeBlocks[order].erase(it);
            return offset;
        }

        std::optional<uint32_t> left = AllocateBlock(order + 1);

        if (left)
        {
            uint32_t size = OrderToUnitSize(order); // unit 为 m_MinBlockSize 的个数
            uint32_t right = *left + size;
            m_FreeBlocks[order].insert(right);
        }

        return left;
    }

    void BuddyAllocator::ReleaseBlock(uint32_t offset, uint32_t order)
    {
        uint32_t size = OrderToUnitSize(order);
        uint32_t buddy = GetBuddyOffset(offset, size);

        if (auto it = m_FreeBlocks[order].find(buddy); it != m_FreeBlocks[order].end())
        {
            ReleaseBlock(std::min(offset, buddy), order + 1);
            m_FreeBlocks[order].erase(it);
        }
        else
        {
            m_FreeBlocks[order].insert(offset);
        }
    }

    std::optional<uint32_t> BuddyAllocator::Allocate(uint32_t sizeInBytes, uint32_t alignment, BuddyAllocation& outAllocation)
    {
        uint32_t sizeToAllocate = sizeInBytes;

        // If the alignment doesn't match the block size
        if (alignment != 0 && m_MinBlockSize % alignment != 0)
        {
            sizeToAllocate += alignment;
        }

        uint32_t unitSize = SizeToUnitSize(sizeToAllocate);
        uint32_t order = UnitSizeToOrder(unitSize);
        std::optional<uint32_t> offset = AllocateBlock(order); // This is the offset in m_MinBlockSize units

        if (!offset)
        {
            return std::nullopt;
        }

        uint32_t byteOffset = *offset * m_MinBlockSize;

        if (alignment != 0 && byteOffset % alignment != 0)
        {
            uint32_t newOffset = MathUtils::AlignUp(byteOffset, alignment);

            uint32_t padding = newOffset - byteOffset;
            uint32_t allocatedSize = OrderToUnitSize(order) * m_MinBlockSize;
            assert(padding + sizeInBytes <= allocatedSize);

            byteOffset = newOffset;
        }

        outAllocation.Owner = this;
        outAllocation.Offset = *offset;
        outAllocation.Order = order;
        return byteOffset;
    }

    void BuddyAllocator::Release(const BuddyAllocation& allocation)
    {
        assert(allocation.Owner == this);
        ReleaseBlock(allocation.Offset, allocation.Order);
    }

    MultiBuddyAllocator::MultiBuddyAllocator(const std::string& name, uint32_t minBlockSize, uint32_t defaultMaxBlockSize)
        : m_Name(name)
        , m_MinBlockSize(minBlockSize)
        , m_DefaultMaxBlockSize(defaultMaxBlockSize)
        , m_Allocators{}
    {
    }

    std::optional<uint32_t> MultiBuddyAllocator::Allocate(uint32_t sizeInBytes, uint32_t alignment, size_t& outAllocatorIndex, BuddyAllocation& outAllocation)
    {
        for (size_t i = 0; i < m_Allocators.size(); i++)
        {
            if (std::optional<uint32_t> result = m_Allocators[i]->Allocate(sizeInBytes, alignment, outAllocation))
            {
                outAllocatorIndex = i;
                return result;
            }
        }

        uint32_t maxBlockSize = sizeInBytes;

        // If the alignment doesn't match the block size
        if (alignment != 0 && m_MinBlockSize % alignment != 0)
        {
            maxBlockSize += alignment;
        }

        if (maxBlockSize <= m_DefaultMaxBlockSize)
        {
            maxBlockSize = m_DefaultMaxBlockSize;
        }
        else
        {
            maxBlockSize = MathUtils::AlignPowerOfTwo(maxBlockSize / m_MinBlockSize) * m_MinBlockSize;
        }

        AppendNewAllocator(maxBlockSize);

        if (std::optional<uint32_t> result = m_Allocators.back()->Allocate(sizeInBytes, alignment, outAllocation))
        {
            outAllocatorIndex = m_Allocators.size() - 1;
            return result;
        }

        return std::nullopt;
    }

    void MultiBuddyAllocator::AppendNewAllocator(uint32_t maxBlockSize)
    {
        LOG_TRACE("%s creates new buddy allocator; MinBlockSize=%u; MaxBlockSize=%u", m_Name.c_str(), m_MinBlockSize, maxBlockSize);
        m_Allocators.emplace_back(std::make_unique<BuddyAllocator>(m_MinBlockSize, maxBlockSize));
    }

    std::unique_ptr<GfxResource> GfxResourceAllocator::CreateResource(
        const std::string& name,
        Microsoft::WRL::ComPtr<ID3D12Resource> res,
        D3D12_RESOURCE_STATES initialState,
        const GfxResourceAllocation& allocation)
    {
        std::unique_ptr<GfxResource> result = std::make_unique<GfxResource>();
        GfxUtils::SetName(result->GetD3DResource(), name);
        result->m_Resource = res;
        result->m_State = initialState;
        result->m_Allocator = this;
        result->m_Allocation = allocation;
        return result;
    }

    GfxResource::~GfxResource()
    {
        GfxResourceAllocator* allocator = m_Allocator;
        GfxResourceAllocation allocation = m_Allocation;

        // 等 GPU 使用完成后释放资源
        GetDevice()->DeferredRelease([allocator, allocation]() { allocator->Release(allocation); }, m_Resource);
    }

    static constexpr uint32_t GetResourcePlacementAlignment(bool msaa)
    {
        return msaa ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    }

    GfxPlacedResourceMultiBuddyAllocator::GfxPlacedResourceMultiBuddyAllocator(GfxDevice* device, const std::string& name, const GfxPlacedResourceMultiBuddyAllocatorDesc& desc)
        : GfxResourceAllocator(device)
        , MultiBuddyAllocator(name, GetResourcePlacementAlignment(desc.MSAA), desc.DefaultMaxBlockSize)
        , m_HeapType(desc.HeapType)
        , m_HeapFlags(desc.HeapFlags)
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
        desc.Properties = CD3DX12_HEAP_PROPERTIES(m_HeapType);
        desc.Alignment = static_cast<UINT64>(GetResourcePlacementAlignment(m_MSAA));
        desc.Flags = m_HeapFlags;

        GFX_HR(device->CreateHeap(&desc, IID_PPV_ARGS(&heap)));
        m_Heaps.push_back(heap);
    }

    void GfxPlacedResourceMultiBuddyAllocator::Release(const GfxResourceAllocation& allocation)
    {
        BuddyAllocator* allocator = allocation.Buddy.Owner;
        allocator->Release(allocation.Buddy);
    }

    GfxCommittedResourceAllocator::GfxCommittedResourceAllocator(GfxDevice* device, const GfxCommittedResourceAllocatorDesc& desc)
        : GfxResourceAllocator(device)
        , m_HeapType(desc.HeapType)
        , m_HeapFlags(desc.HeapFlags)
    {
    }

    std::unique_ptr<GfxResource> GfxCommittedResourceAllocator::Allocate(
        const std::string& name,
        const D3D12_RESOURCE_DESC* pDesc,
        D3D12_RESOURCE_STATES initialState,
        const D3D12_CLEAR_VALUE* pOptimizedClearValue)
    {
        ID3D12Device4* device = GetDevice()->GetD3DDevice4();
        D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(m_HeapType);
        ComPtr<ID3D12Resource> resource = nullptr;

        GFX_HR(device->CreateCommittedResource(&heapProperties, m_HeapFlags, pDesc, initialState, pOptimizedClearValue, IID_PPV_ARGS(&resource)));
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
