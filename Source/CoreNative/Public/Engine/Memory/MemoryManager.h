#pragma once

#include <stdint.h>
#include <stdalign.h>
#include <type_traits>
#include <vector>
#include <fmt/base.h>
#include <string_view>

namespace march
{
    enum class MemoryLabel
    {
        Default,
        Graphics,
        ImGui,
        _Count
    };

	struct MemoryAllocation
	{
		void* Pointer;
		size_t SizeInBytes;
		size_t Alignment;
		MemoryLabel Label;
		const char* File;
		int Line;
	};

    struct MemoryManager
    {
        static void Initialize();

        static void* Allocate(size_t sizeInBytes, MemoryLabel label, const char* file, int line);
        static void* Allocate(size_t sizeInBytes, size_t alignment, MemoryLabel label, const char* file, int line);
        static void Release(void* ptr, MemoryLabel label);

        static size_t GetAllocatedSizeInBytes(MemoryLabel label);
        static std::vector<MemoryAllocation> GetActiveAllocations();
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

// Ref: https://fmt.dev/11.1/api/#formatting-user-defined-types
template <>
struct fmt::formatter<march::MemoryLabel> : formatter<string_view>
{
    // parse is inherited from formatter<string_view>.

    format_context::iterator format(march::MemoryLabel label, format_context& ctx) const
    {
        string_view name;
        switch (label)
        {
        case march::MemoryLabel::Default:  name = "Default";  break;
        case march::MemoryLabel::Graphics: name = "Graphics"; break;
        case march::MemoryLabel::ImGui:    name = "ImGui";    break;
        default:                           name = "Unknown";  break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};
