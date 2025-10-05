#include "pch.h"
#include "Engine/Memory/MemoryManager.h"
#include "Engine/Misc/MathUtils.h"
#include "Engine/Debug.h"
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

    class StackAllocator : public IMemoryAllocator
    {
        struct Header
        {
            size_t IsReleased: 1;
            size_t Size: 63;
            uintptr_t LastPtr;
        };

        uintptr_t m_BufferStart;
        uintptr_t m_BufferEnd;
        uintptr_t m_LastAlloc;

        uintptr_t GetCurrentFreePtr() const
        {
            if (m_LastAlloc == 0)
                return m_BufferStart;
            Header* header = reinterpret_cast<Header*>(m_LastAlloc) - 1;
            return m_LastAlloc + header->Size;
        }

    public:
        explicit StackAllocator(size_t size)
        {
            void* p = malloc(size);
            m_BufferStart = reinterpret_cast<uintptr_t>(p);
            m_BufferEnd = m_BufferStart + size;
            m_LastAlloc = 0;
        }

        ~StackAllocator() override
        {
            free(reinterpret_cast<void*>(m_BufferStart));
        }

        void* Allocate(size_t sizeInBytes, size_t alignment) override
        {
            assert(MathUtils::IsPowerOfTwo(alignment) && (alignment >= alignof(Header)));
            assert((sizeInBytes & 0x8000000000000000) == 0); // 最高位不能是 1

            uintptr_t currentPtr = GetCurrentFreePtr();
            uintptr_t alignedPtr = MathUtils::AlignUp(currentPtr, alignment);

            if (uintptr_t padding = alignedPtr - currentPtr; padding < sizeof(Header))
            {
                // 补加上 ceil((sizeof(Header) - padding) / alignment) * alignment
                alignedPtr += (sizeof(Header) - padding + alignment - 1) / alignment * alignment;
            }

            // Fallback to default allocator
            if (alignedPtr + sizeInBytes >= m_BufferEnd)
            {
                LOG_WARNING("StackAllocator out of memory, fallback to default allocator; Size={}; Alignment={}", sizeInBytes, alignment);
                return mi_malloc_aligned(sizeInBytes, alignment);
            }

            Header* header = reinterpret_cast<Header*>(alignedPtr) - 1;
            header->IsReleased = 0;
            header->Size = sizeInBytes;
            header->LastPtr = m_LastAlloc;

            m_LastAlloc = alignedPtr;
            return reinterpret_cast<void*>(alignedPtr);
        }

        void Release(void* ptr) override
        {
            assert(ptr != nullptr);

            if (uintptr_t p = reinterpret_cast<uintptr_t>(ptr); p == m_LastAlloc)
            {
                Header* header = reinterpret_cast<Header*>(p) - 1;

                do
                {
                    m_LastAlloc = header->LastPtr;
                    header = m_LastAlloc == 0 ? nullptr : reinterpret_cast<Header*>(m_LastAlloc) - 1;
                } while (header && header->IsReleased == 1);
            }
            else if (p < m_BufferStart || p >= m_BufferEnd)
            {
                mi_free(ptr);
            }
            else
            {
                Header* header = reinterpret_cast<Header*>(p) - 1;
                header->IsReleased = 1;
            }
        }
    };

    static thread_local std::unique_ptr<StackAllocator> g_TlsStackAllocator;

    class TlsStackAllocator : public IMemoryAllocator
    {
        static StackAllocator* GetAllocator()
        {
            if (g_TlsStackAllocator == nullptr)
                g_TlsStackAllocator = std::make_unique<StackAllocator>(8 * 1024 * 1024); // 每个线程 8MB
            return g_TlsStackAllocator.get();
        }

    public:
        void* Allocate(size_t sizeInBytes, size_t alignment) override
        {
            return GetAllocator()->Allocate(sizeInBytes, alignment);
        }

        void Release(void* ptr) override
        {
            GetAllocator()->Release(ptr);
        }
    };

    static constexpr size_t g_MemoryLabelCount = static_cast<size_t>(MemoryLabel::_Count);

    struct MemoryManagerConfig
    {
        std::unique_ptr<IMemoryAllocator> Allocators[g_MemoryLabelCount];

        MemoryManagerConfig()
        {
            Allocators[static_cast<size_t>(MemoryLabel::Default)] = std::make_unique<DefaultAllocator>();
            Allocators[static_cast<size_t>(MemoryLabel::Temp)] = std::make_unique<TlsStackAllocator>();
            Allocators[static_cast<size_t>(MemoryLabel::Debug)] = std::make_unique<DefaultAllocator>();
            Allocators[static_cast<size_t>(MemoryLabel::Graphics)] = std::make_unique<DefaultAllocator>();
            Allocators[static_cast<size_t>(MemoryLabel::ImGui)] = std::make_unique<DefaultAllocator>();
        }
    };

    static MemoryManagerConfig g_Config{};

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

        IMemoryAllocator* allocator = g_Config.Allocators[static_cast<size_t>(label)].get();
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

        IMemoryAllocator* allocator = g_Config.Allocators[static_cast<size_t>(label)].get();

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
