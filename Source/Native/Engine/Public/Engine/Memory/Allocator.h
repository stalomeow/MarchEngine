#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include <optional>
#include <memory>
#include <functional>

namespace march
{
    class LinearAllocator final
    {
    public:
        // 返回 page index
        using RequestPageFunc = std::function<size_t(uint32_t sizeInBytes, bool large, bool* pOutIsNew)>;

        LinearAllocator(const std::string& name, uint32_t pageSize, const RequestPageFunc& requestPageFunc);

        void Reset();
        uint32_t Allocate(uint32_t sizeInBytes, uint32_t alignment, size_t* pOutPageIndex, bool* pOutLarge);

        const std::string& GetName() const { return m_Name; }

    private:
        std::string m_Name;
        uint32_t m_PageSize;
        RequestPageFunc m_RequestPageFunc;

        size_t m_CurrentPageIndex;
        uint32_t m_NextAllocOffset;
    };

    struct BuddyAllocation
    {
        class BuddyAllocator* Owner;
        uint32_t Offset;
        uint32_t Order;
    };

    class BuddyAllocator final
    {
    public:
        BuddyAllocator(uint32_t minBlockSize, uint32_t maxBlockSize);

        void Reset();
        std::optional<uint32_t> Allocate(uint32_t sizeInBytes, uint32_t alignment, BuddyAllocation* pOutAllocation);
        void Release(const BuddyAllocation& allocation);

        uint32_t GetMaxSize() const { return m_MaxBlockSize; }
        uint32_t GetTotalAllocatedSize() const { return m_TotalAllocatedSize; }

    private:
        uint32_t m_MinBlockSize;
        uint32_t m_MaxBlockSize;
        uint32_t m_MaxOrder;
        std::vector<std::set<uint32_t>> m_FreeBlocks;
        uint32_t m_TotalAllocatedSize;

        uint32_t SizeToUnitSize(uint32_t size) const;
        uint32_t UnitSizeToOrder(uint32_t size) const;
        uint32_t OrderToUnitSize(uint32_t order) const;
        uint32_t GetBuddyOffset(uint32_t offset, uint32_t size) const;

        std::optional<uint32_t> AllocateBlock(uint32_t order);
        void ReleaseBlock(uint32_t offset, uint32_t order);
    };

    class MultiBuddyAllocator final
    {
    public:
        using AppendPageFunc = std::function<void(uint32_t sizeInBytes)>;

        MultiBuddyAllocator(const std::string& name, uint32_t minBlockSize, uint32_t defaultMaxBlockSize, const AppendPageFunc& appendPageFunc);

        void Reset();
        std::optional<uint32_t> Allocate(uint32_t sizeInBytes, uint32_t alignment, size_t* pOutPageIndex, BuddyAllocation* pOutAllocation);
        void Release(const BuddyAllocation& allocation);

        const std::string& GetName() const { return m_Name; }

    private:
        std::string m_Name;
        uint32_t m_MinBlockSize;
        uint32_t m_DefaultMaxBlockSize;
        AppendPageFunc m_AppendPageFunc;
        std::vector<std::unique_ptr<BuddyAllocator>> m_PageAllocators;

        void AppendNewPage(uint32_t maxBlockSize);
    };
}
