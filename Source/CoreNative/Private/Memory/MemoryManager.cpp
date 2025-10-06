#include "pch.h"
#include "Engine/Memory/MemoryManager.h"
#include "Engine/Misc/PlatformUtils.h"
#include "Engine/Misc/StringUtils.h"
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

    class TlsStackAllocator : public IMemoryAllocator
    {
        static StackAllocator* GetAllocator()
        {
            static thread_local std::unique_ptr<StackAllocator> allocator = nullptr;

            if (allocator == nullptr)
                allocator = std::make_unique<StackAllocator>(8 * 1024 * 1024); // 每个线程 8MB
            return allocator.get();
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

    struct MemoryManagerData
    {
        static constexpr size_t LabelCount = static_cast<size_t>(MemoryLabel::_Count);

        DefaultAllocator DefaultAllocator{};
        TlsStackAllocator TlsStackAllocator{};

#ifdef DEBUG_MEMORY
        // 所有线程的 AllocMap
        std::atomic<class ThreadAllocRecorder*> Recorders{ nullptr };

        // 每个 Label 分配的内存总量
        std::atomic<size_t> SizeInBytes[LabelCount]{};
#endif
    };

    // 永远不释放
    static MemoryManagerData* g_MemoryData = new MemoryManagerData();

#ifdef DEBUG_MEMORY
    class ThreadAllocRecorder
    {
        struct AllocData
        {
            size_t SizeInBytes;
            size_t Alignment;
            MemoryLabel Label;
            const char* File;
            int Line;
        };

        std::shared_mutex m_Mutex;
        std::unordered_map<void*, AllocData> m_Data;
        std::atomic<ThreadAllocRecorder*> m_Next;

    public:
        ThreadAllocRecorder(std::atomic<ThreadAllocRecorder*>& head) : m_Mutex{}, m_Data{}, m_Next(nullptr)
        {
            ThreadAllocRecorder* oldHead = head.load(std::memory_order_relaxed);
            do
            {
                m_Next.store(oldHead, std::memory_order_relaxed);
            } while (!head.compare_exchange_weak(oldHead, this, std::memory_order_release, std::memory_order_relaxed));
        }

        void Register(void* ptr, size_t sizeInBytes, size_t alignment, MemoryLabel label, const char* file, int line)
        {
            std::unique_lock lock(m_Mutex);
            m_Data[ptr] = AllocData{ sizeInBytes, alignment, label, file, line };
        }

        bool Unregister(void* ptr, size_t& outSizeInBytes)
        {
            std::unique_lock lock(m_Mutex);

            if (auto it = m_Data.find(ptr); it != m_Data.end())
            {
                outSizeInBytes = it->second.SizeInBytes;
                m_Data.erase(it);
                return true;
            }

            return false;
        }

        void GetAllocs(std::vector<MemoryAllocation>& result)
        {
            std::shared_lock lock(m_Mutex);

            for (const auto& [ptr, data] : m_Data)
            {
                MemoryAllocation& alloc = result.emplace_back();
                alloc.Pointer = ptr;
                alloc.SizeInBytes = data.SizeInBytes;
                alloc.Alignment = data.Alignment;
                alloc.Label = data.Label;
                alloc.File = data.File;
                alloc.Line = data.Line;
            }
        }

        ThreadAllocRecorder* GetNext() const
        {
            return m_Next.load(std::memory_order_relaxed);
        }
    };

    static ThreadAllocRecorder* GetThreadLocalAllocRecorder()
    {
        // 永远不释放
        static thread_local ThreadAllocRecorder* recorder = nullptr;

        if (recorder == nullptr)
        {
            recorder = new ThreadAllocRecorder(g_MemoryData->Recorders);
        }

        return recorder;
    }

    static void RegisterAlloc(void* ptr, size_t sizeInBytes, size_t alignment, MemoryLabel label, const char* file, int line)
    {
        GetThreadLocalAllocRecorder()->Register(ptr, sizeInBytes, alignment, label, file, line);
        g_MemoryData->SizeInBytes[static_cast<size_t>(label)].fetch_add(sizeInBytes, std::memory_order_relaxed);
    }

    static void UnregisterAlloc(void* ptr, MemoryLabel label)
    {
        bool found = false;
        size_t sizeInBytes = 0;
        ThreadAllocRecorder* localRecorder = GetThreadLocalAllocRecorder();

        if (localRecorder->Unregister(ptr, sizeInBytes))
        {
            found = true;
        }
        else
        {
            ThreadAllocRecorder* recorder = g_MemoryData->Recorders.load(std::memory_order_acquire);

            // 有可能是在其他线程分配的内存
            while (recorder)
            {
                if (recorder != localRecorder && recorder->Unregister(ptr, sizeInBytes))
                {
                    found = true;
                    break;
                }

                recorder = recorder->GetNext();
            }
        }

        if (found && sizeInBytes > 0)
        {
            g_MemoryData->SizeInBytes[static_cast<size_t>(label)].fetch_sub(sizeInBytes, std::memory_order_relaxed);
        }
    }
#endif

    static IMemoryAllocator* GetAllocator(MemoryLabel label)
    {
        switch (label)
        {
        case MemoryLabel::Default:
        case MemoryLabel::Debug:
        case MemoryLabel::Graphics:
        case MemoryLabel::ImGui:
            return &g_MemoryData->DefaultAllocator;

        case MemoryLabel::Temp:
            return &g_MemoryData->TlsStackAllocator;

        default:
            return nullptr;
        }
    }

    void* MemoryManager::Allocate(size_t sizeInBytes, MemoryLabel label, const char* file, int line)
    {
        return Allocate(sizeInBytes, DEFAULT_ALIGNMENT, label, file, line);
    }

    void* MemoryManager::Allocate(size_t sizeInBytes, size_t alignment, MemoryLabel label, const char* file, int line)
    {
        if (sizeInBytes == 0)
            return nullptr;

        alignment = std::max<size_t>(alignment, DEFAULT_ALIGNMENT); // 不要小于默认的对齐要求
        void* ptr = GetAllocator(label)->Allocate(sizeInBytes, alignment);

#ifdef DEBUG_MEMORY
        RegisterAlloc(ptr, sizeInBytes, alignment, label, file, line);
#endif

        return ptr;
    }

    void MemoryManager::Release(void* ptr, MemoryLabel label)
    {
        if (ptr == nullptr)
            return;

#ifdef DEBUG_MEMORY
        UnregisterAlloc(ptr, label);
#endif

        GetAllocator(label)->Release(ptr);
    }

    size_t MemoryManager::GetAllocatedSizeInBytes(MemoryLabel label)
    {
#ifdef DEBUG_MEMORY
        return g_MemoryData->SizeInBytes[static_cast<size_t>(label)].load(std::memory_order_relaxed);
#else
        return 0;
#endif
    }

    std::vector<MemoryAllocation> MemoryManager::GetActiveAllocations()
    {
        std::vector<MemoryAllocation> res{};

#ifdef DEBUG_MEMORY
        ThreadAllocRecorder* recorder = g_MemoryData->Recorders.load(std::memory_order_acquire);

        while (recorder)
        {
            recorder->GetAllocs(res);
            recorder = recorder->GetNext();
        }
#endif

        return res;
    }
}
