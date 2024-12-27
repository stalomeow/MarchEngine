#include "pch.h"
#include "Engine/Allocator.h"
#include "Engine/Debug.h"
#include "Engine/MathUtils.h"
#include <limits>
#include <assert.h>

namespace march
{
    LinearAllocator::LinearAllocator(const std::string& name, uint32_t pageSize)
        : m_Name(name)
        , m_PageSize(pageSize)
        , m_CurrentPageIndex(std::numeric_limits<size_t>::max())
        , m_NextAllocOffset(0)
    {
    }

    void LinearAllocator::Reset()
    {
        m_CurrentPageIndex = std::numeric_limits<size_t>::max();
        m_NextAllocOffset = 0;
    }

    uint32_t LinearAllocator::Allocate(uint32_t sizeInBytes, uint32_t alignment, size_t& outPageIndex, bool& outLarge)
    {
        if (sizeInBytes > m_PageSize)
        {
            bool isNew = false;
            outPageIndex = RequestPage(sizeInBytes, true, &isNew);
            outLarge = true;

            if (isNew)
            {
                LOG_TRACE("{} creates new LARGE page; Size={}", m_Name, sizeInBytes);
            }
            return 0;
        }

        uint32_t offset = m_NextAllocOffset;

        if (alignment != 0)
        {
            offset = MathUtils::AlignUp(offset, alignment);
        }

        if (m_CurrentPageIndex == std::numeric_limits<size_t>::max() || offset + sizeInBytes > m_PageSize)
        {
            bool isNew = false;
            m_CurrentPageIndex = RequestPage(m_PageSize, false, &isNew);
            offset = 0; // AlignUp already done

            if (isNew)
            {
                LOG_TRACE("{} creates new page; Size={}", m_Name, m_PageSize);
            }
        }

        m_NextAllocOffset = offset + sizeInBytes;
        outPageIndex = m_CurrentPageIndex;
        outLarge = false;
        return offset;
    }

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
        m_FreeBlocks.resize(static_cast<size_t>(m_MaxOrder) + 1);
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
        LOG_TRACE("{} creates new buddy allocator; MinBlockSize={}; MaxBlockSize={}", m_Name, m_MinBlockSize, maxBlockSize);
        m_Allocators.emplace_back(std::make_unique<BuddyAllocator>(m_MinBlockSize, maxBlockSize));
    }
}
