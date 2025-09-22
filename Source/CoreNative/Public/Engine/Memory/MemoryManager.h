#pragma once

#include <stdint.h>
#include <stdalign.h>
#include <type_traits>

namespace march
{
    enum class MemoryLabel
    {
        Default,
        StackAlloc,
        ImGui,
        _LabelCount
    };

    class IMemoryAllocator
    {
    public:
        virtual ~IMemoryAllocator() = default;

        virtual void* Allocate(size_t sizeInBytes, size_t alignment) = 0;
        virtual void Release(void* ptr) = 0;
    };

    struct MemoryManager
    {
        static void Initialize();

        static void* Allocate(size_t sizeInBytes, size_t alignment, MemoryLabel label, const char* file, int line);
        static void Release(void* ptr, MemoryLabel label);

        static size_t GetAllocatedSizeInBytes(MemoryLabel label);
        static void LogActiveAllocations(bool reportAsLeak = false);
    };

    namespace internal
    {
        template <typename T>
        inline void DeleteInternal(T* ptr, MemoryLabel label)
        {
            if (ptr)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                    ptr->~T();
                MemoryManager::Release(ptr, label);
            }
        }

        template <typename T>
        inline void DeleteInternal(T* ptr, MemoryLabel label, size_t size)
        {
            if (ptr)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                    for (size_t i = 0; i < size; i++)
                        ptr[i].~T();
                MemoryManager::Release(ptr, label);
            }
        }
    }
}

#define MARCH_NEW(type, label) new (::march::MemoryManager::Allocate(sizeof(type), static_cast<size_t>(alignof(type)), label, __FILE__, __LINE__)) type
#define MARCH_DELETE(ptr, label) ::march::internal::DeleteInternal(ptr, label)

#define MARCH_NEW_ARRAY(type, label, n) new (::march::MemoryManager::Allocate(sizeof(type) * n, static_cast<size_t>(alignof(type)), label, __FILE__, __LINE__)) type[n]
#define MARCH_DELETE_ARRAY(ptr, label, n) ::march::internal::DeleteInternal(ptr, label, n)
