#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include <optional>
#include <memory>

namespace march
{
    class LinearAllocator
    {
    public:
        LinearAllocator(const std::string& name, uint32_t pageSize);
        virtual ~LinearAllocator() = default;

        void Reset();
        uint32_t Allocate(uint32_t sizeInBytes, uint32_t alignment, size_t& outPageIndex, bool& outLarge);

        const std::string& GetName() const { return m_Name; }

    protected:
        // 返回 page index
        virtual size_t RequestNewPage(uint32_t sizeInBytes, bool large) = 0;

    private:
        const std::string m_Name;
        const uint32_t m_PageSize;
        size_t m_CurrentPageIndex;
        uint32_t m_NextAllocOffset;
    };

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
        const std::string m_Name;
        const uint32_t m_MinBlockSize;
        const uint32_t m_DefaultMaxBlockSize;
        std::vector<std::unique_ptr<BuddyAllocator>> m_Allocators;
    };
}
