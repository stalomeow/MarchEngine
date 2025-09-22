#pragma once

#include "Engine/Memory/MemoryManager.h"
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
    template <typename T>
    struct allocator
    {
        MemoryLabel Label;

        allocator() : Label(MemoryLabel::Default) {}
        allocator(MemoryLabel label) : Label(label) {}

        using value_type = T;

        T* allocate(size_t n)
        {
            return MemoryManager::Allocate(n * sizeof(T), static_cast<size_t>(alignof(T)), Label, __FILE__, __LINE__);
        }

        void deallocate(T* p, size_t n)
        {
            MemoryManager::Release(p, Label);
        }
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
        unique_ptr_deleter(MemoryLabel label) : Label(label) {}
        void operator()(T* ptr) const { MARCH_DELETE(ptr, Label); }
    };

    template <typename T>
    struct unique_ptr_deleter<T[]>
    {
        MemoryLabel Label;
        size_t Size;
        unique_ptr_deleter(MemoryLabel label, size_t size) : Label(label), Size(size) {}
        void operator()(T* ptr) const { MARCH_DELETE_ARRAY(ptr, Label, Size); }
    };

    template <typename T>
    using unique_ptr = std::unique_ptr<T, unique_ptr_deleter<T>>;

    template <typename T, typename... Args, std::enable_if_t<!std::is_array_v<T>, int> = 0>
    inline unique_ptr<T> make_unique(MemoryLabel label, Args&&... args)
    {
        return unique_ptr<T>(MARCH_NEW(T, label)(std::forward<Args>(args)...), unique_ptr_deleter<T>(label));
    }

    template <typename T, std::enable_if_t<std::is_array_v<T>&& std::extent_v<T> == 0, int> = 0>
    inline unique_ptr<T> make_unique(MemoryLabel label, size_t size)
    {
        using Elem = std::remove_extent_t<T>;
        return unique_ptr<T>(MARCH_NEW_ARRAY(Elem, label, size)(), unique_ptr_deleter<T>(label, size));
    }
}
