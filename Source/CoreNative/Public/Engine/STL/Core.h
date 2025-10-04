#pragma once

#include "Engine/Memory/MemoryManager.h"
#include <stdint.h>
#include <stdexcept>
#include <array>
#include <vector>
#include <deque>
#include <forward_list>
#include <list>
#include <stack>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <string>
#include <memory>

namespace march::stl
{
    // Ref: https://en.cppreference.com/w/cpp/named_req/Allocator
    template <typename T>
    struct allocator
    {
        using value_type = T;
        using pointer = T*;

        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap = std::true_type;
        using is_always_equal = std::false_type;

        template <typename U>
        struct rebind { using other = allocator<U>; };

        MemoryLabel Label;

        constexpr allocator() noexcept : Label(MemoryLabel::Default) {}
        constexpr allocator(MemoryLabel label) noexcept : Label(label) {}

        template <typename U>
        constexpr allocator(const allocator<U>& other) noexcept : Label(other.Label) {}

        // 移动后不改变自己的 Label
        template <typename U>
        constexpr allocator(allocator<U>&& other) noexcept : Label(other.Label) {}

        template <typename U>
        constexpr allocator& operator=(const allocator<U>& other) noexcept
        {
            Label = other.Label;
            return *this;
        }

        template <typename U>
        constexpr allocator& operator=(allocator<U>&& other) noexcept
        {
            // 移动后不改变自己的 Label
            Label = other.Label;
            return *this;
        }

        pointer allocate(size_type n)
        {
            if (n > max_size())
                throw std::bad_array_new_length();
            size_t sizeInBytes = n * sizeof(T);
            size_t alignment = static_cast<size_t>(alignof(T));
            return static_cast<pointer>(MemoryManager::Allocate(sizeInBytes, alignment, Label, __FILE__, __LINE__));
        }

        void deallocate(pointer p, size_type)
        {
            MemoryManager::Release(p, Label);
        }

        constexpr size_type max_size() const noexcept { return static_cast<size_type>(-1) / sizeof(T); }

        template <typename U>
        friend constexpr bool operator==(const allocator& a, const allocator<U>& b) noexcept { return a.Label == b.Label; }

        template <typename U>
        friend constexpr bool operator!=(const allocator& a, const allocator<U>& b) noexcept { return a.Label != b.Label; }
    };

    template <typename T, size_t N>
    using array = std::array<T, N>;

    template <typename T>
    using vector = std::vector<T, allocator<T>>;

    template <typename T>
    using deque = std::deque<T, allocator<T>>;

    template <typename T>
    using forward_list = std::forward_list<T, allocator<T>>;

    template <typename T>
    using list = std::list<T, allocator<T>>;

    template <typename T, typename Container = deque<T>>
    using stack = std::stack<T, Container>;

    template <typename T, typename Container = deque<T>>
    using queue = std::queue<T, Container>;

    template <typename T, typename Container = vector<T>, typename Compare = std::less<typename Container::value_type>>
    using priority_queue = std::priority_queue<T, Container, Compare>;

    template <typename K, typename V, typename Hash = std::hash<K>, typename Equal = std::equal_to<K>>
    using unordered_map = std::unordered_map<K, V, Hash, Equal, allocator<std::pair<const K, V>>>;

    template <typename K, typename V, typename Hash = std::hash<K>, typename Equal = std::equal_to<K>>
    using unordered_multimap = std::unordered_multimap<K, V, Hash, Equal, allocator<std::pair<const K, V>>>;

    template <typename K, typename Hash = std::hash<K>, typename Equal = std::equal_to<K>>
    using unordered_set = std::unordered_set<K, Hash, Equal, allocator<K>>;

    template <typename K, typename Hash = std::hash<K>, typename Equal = std::equal_to<K>>
    using unordered_multiset = std::unordered_multiset<K, Hash, Equal, allocator<K>>;

    template <typename K, typename V, typename Compare = std::less<K>>
    using map = std::map<K, V, Compare, allocator<std::pair<const K, V>>>;

    template <typename K, typename V, typename Compare = std::less<K>>
    using multimap = std::multimap<K, V, Compare, allocator<std::pair<const K, V>>>;

    template <typename K, typename Compare = std::less<K>>
    using set = std::set<K, Compare, allocator<K>>;

    template <typename K, typename Compare = std::less<K>>
    using multiset = std::multiset<K, Compare, allocator<K>>;

    using string = std::basic_string<char, std::char_traits<char>, allocator<char>>;
    using wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t>>;

    template <typename T>
    struct unique_ptr_deleter
    {
        MemoryLabel Label;
        unique_ptr_deleter() : Label(MemoryLabel::Default) {}
        unique_ptr_deleter(MemoryLabel label) : Label(label) {}
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        unique_ptr_deleter(const unique_ptr_deleter<U>& other) : Label(other.Label) {}
        void operator()(T* ptr) const { MARCH_DELETE(ptr, Label); }
    };

    template <typename T>
    struct unique_ptr_deleter<T[]>
    {
        MemoryLabel Label;
        size_t Size;
        unique_ptr_deleter() : Label(MemoryLabel::Default), Size(0) {}
        unique_ptr_deleter(MemoryLabel label, size_t size) : Label(label), Size(size) {}
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U(*)[], T(*)[]>>>
        unique_ptr_deleter(const unique_ptr_deleter<U[]>& other) : Label(other.Label), Size(other.Size) {}
        void operator()(T* ptr) const { MARCH_DELETE_ARRAY(ptr, Label, Size); }
    };

    template <typename T>
    using unique_ptr = std::unique_ptr<T, unique_ptr_deleter<T>>;

    template <typename T, typename... Args, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    inline unique_ptr<T> make_unique(MemoryLabel label, Args&&... args)
    {
        return unique_ptr<T>(MARCH_NEW(T, label)(std::forward<Args>(args)...), unique_ptr_deleter<T>(label));
    }

    template <typename T, std::enable_if_t<std::is_array_v<T> && std::extent_v<T> == 0, int> = 0>
    inline unique_ptr<T> make_unique(MemoryLabel label, size_t size)
    {
        using Elem = std::remove_extent_t<T>;
        return unique_ptr<T>(MARCH_NEW_ARRAY(Elem, label, size)(), unique_ptr_deleter<T>(label, size));
    }
}
