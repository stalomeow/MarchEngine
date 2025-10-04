#include "pch.h"
#include "Engine/Memory/MemoryManager.h"
#include <memory>
#include <vector>
#include <shared_mutex>
#include <unordered_map>
#include <atomic>
#include "mimalloc.h"

#ifdef _DEBUG
#define DEBUG_MEMORY 1
#endif

#define DEFAULT_ALIGNMENT __STDCPP_DEFAULT_NEW_ALIGNMENT__

namespace march
{
    class IMemoryAllocator
    {
    public:
        virtual ~IMemoryAllocator() = default;

        virtual void* Allocate(size_t sizeInBytes, size_t alignment) = 0;
        virtual void Release(void* ptr) = 0;
    };

    class DefaultAllocator : public IMemoryAllocator
    {
    public:
        void* Allocate(size_t sizeInBytes, size_t alignment) override
        {
            return mi_malloc_aligned(sizeInBytes, alignment);
        }

        void Release(void* ptr) override
        {
            mi_free(ptr);
        }
    };

    static constexpr size_t g_MemoryLabelCount = static_cast<size_t>(MemoryLabel::_Count);
    static std::unique_ptr<IMemoryAllocator> g_Allocators[g_MemoryLabelCount]{};

    void MemoryManager::Initialize()
    {
        g_Allocators[static_cast<size_t>(MemoryLabel::Default)] = std::make_unique<DefaultAllocator>();
        g_Allocators[static_cast<size_t>(MemoryLabel::Graphics)] = std::make_unique<DefaultAllocator>();
        g_Allocators[static_cast<size_t>(MemoryLabel::ImGui)] = std::make_unique<DefaultAllocator>();
    }

#ifdef DEBUG_MEMORY
    struct AllocData
    {
        size_t SizeInBytes;
        size_t Alignment;
        MemoryLabel Label;
        const char* File;
        int Line;
    };

    struct AllocMap
    {
        bool Initialized;
        std::unordered_map<void*, AllocData> Data;
        std::shared_mutex Mutex;
    };

    static thread_local AllocMap g_ThreadLocalAllocMap{};
    static std::vector<AllocMap*> g_ThreadAllocMaps{};
    static std::shared_mutex g_ThreadAllocMapsMutex{};

    static std::atomic<size_t> g_AllocatedSizeInBytes[g_MemoryLabelCount]{};

    static void RegisterAlloc(void* ptr, size_t sizeInBytes, size_t alignment, MemoryLabel label, const char* file, int line)
    {
        if (!g_ThreadLocalAllocMap.Initialized)
        {
            std::unique_lock lock(g_ThreadAllocMapsMutex);
            g_ThreadAllocMaps.push_back(&g_ThreadLocalAllocMap);
            g_ThreadLocalAllocMap.Initialized = true;
        }

        {
            std::unique_lock lock(g_ThreadLocalAllocMap.Mutex);
            g_ThreadLocalAllocMap.Data[ptr] = AllocData{ sizeInBytes, alignment, label, file, line };
        }

        g_AllocatedSizeInBytes[static_cast<size_t>(label)].fetch_add(sizeInBytes, std::memory_order_relaxed);
    }

    static void UnregisterAlloc(void* ptr, MemoryLabel label)
    {
        bool found = false;
        size_t sizeInBytes = 0;

        if (g_ThreadLocalAllocMap.Initialized)
        {
            std::unique_lock lock(g_ThreadLocalAllocMap.Mutex);

            if (auto it = g_ThreadLocalAllocMap.Data.find(ptr); it != g_ThreadLocalAllocMap.Data.end())
            {
                found = true;
                sizeInBytes = it->second.SizeInBytes;
                g_ThreadLocalAllocMap.Data.erase(it);
            }
        }

        // 有可能是在其他线程分配的内存
        if (!found)
        {
            std::shared_lock lock1(g_ThreadAllocMapsMutex);

            for (AllocMap* allocMap : g_ThreadAllocMaps)
            {
                if (allocMap == &g_ThreadLocalAllocMap)
                    continue;

                std::unique_lock lock2(allocMap->Mutex);

                if (auto it = allocMap->Data.find(ptr); it != allocMap->Data.end())
                {
                    found = true;
                    sizeInBytes = it->second.SizeInBytes;
                    allocMap->Data.erase(it);
                    break;
                }
            }
        }

        if (found && sizeInBytes > 0)
        {
            g_AllocatedSizeInBytes[static_cast<size_t>(label)].fetch_sub(sizeInBytes, std::memory_order_relaxed);
        }
    }
#endif

    void* MemoryManager::Allocate(size_t sizeInBytes, MemoryLabel label, const char* file, int line)
    {
        return Allocate(sizeInBytes, DEFAULT_ALIGNMENT, label, file, line);
    }

    void* MemoryManager::Allocate(size_t sizeInBytes, size_t alignment, MemoryLabel label, const char* file, int line)
    {
        if (sizeInBytes == 0)
            return nullptr;

        // 不要小于默认的对齐要求
        alignment = std::max<size_t>(alignment, DEFAULT_ALIGNMENT);

        IMemoryAllocator* allocator = g_Allocators[static_cast<size_t>(label)].get();
        void* ptr = allocator->Allocate(sizeInBytes, alignment);

#ifdef DEBUG_MEMORY
        RegisterAlloc(ptr, sizeInBytes, alignment, label, file, line);
#endif

        return ptr;
    }

    void MemoryManager::Release(void* ptr, MemoryLabel label)
    {
        if (ptr == nullptr)
            return;

        IMemoryAllocator* allocator = g_Allocators[static_cast<size_t>(label)].get();

#ifdef DEBUG_MEMORY
        UnregisterAlloc(ptr, label);
#endif

        allocator->Release(ptr);
    }

    size_t MemoryManager::GetAllocatedSizeInBytes(MemoryLabel label)
    {
        return g_AllocatedSizeInBytes[static_cast<size_t>(label)].load(std::memory_order_relaxed);
    }

    std::vector<MemoryAllocation> MemoryManager::GetActiveAllocations()
    {
        std::vector<MemoryAllocation> res{};

#ifdef DEBUG_MEMORY
        std::shared_lock lock1(g_ThreadAllocMapsMutex);

        for (AllocMap* allocMap : g_ThreadAllocMaps)
        {
            std::shared_lock lock2(allocMap->Mutex);

            for (const auto& [ptr, data] : allocMap->Data)
            {
                MemoryAllocation& alloc = res.emplace_back();
                alloc.Pointer = ptr;
                alloc.SizeInBytes = data.SizeInBytes;
                alloc.Alignment = data.Alignment;
                alloc.Label = data.Label;
                alloc.File = data.File;
                alloc.Line = data.Line;
            }
        }
#endif

        return res;
    }
}
