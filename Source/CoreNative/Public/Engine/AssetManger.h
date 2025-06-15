#pragma once

#include <string>
#include <utility>

namespace march
{
    struct AssetManager final
    {
        static void* LoadAsset(const std::string& path);
        static void UnloadAsset(void* asset);
    };

    template <typename T>
    class asset_ptr final
    {
    public:
        constexpr asset_ptr() noexcept : m_Asset(nullptr) {}

        constexpr asset_ptr(std::nullptr_t) noexcept : m_Asset(nullptr) {}

        asset_ptr& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        asset_ptr(asset_ptr&& other) noexcept : m_Asset(other.release()) {}

        asset_ptr& operator=(asset_ptr&& other) noexcept
        {
            reset(other.release()); // 这里不用检查 this != &other
            return *this;
        }

        ~asset_ptr() noexcept { reset(); }

        T* release() noexcept { return std::exchange(m_Asset, nullptr); }

        void reset(T* asset = nullptr) noexcept
        {
            if (T* old = std::exchange(m_Asset, asset); old != nullptr)
            {
                AssetManager::UnloadAsset(old);
            }
        }

        void reset(const std::string& path)
        {
            reset(static_cast<T*>(AssetManager::LoadAsset(path)));
        }

        T* get() const noexcept { return m_Asset; }

        T* operator->() const noexcept { return m_Asset; }

        T& operator*() const noexcept { return *m_Asset; }

        bool operator==(std::nullptr_t) const noexcept { return m_Asset == nullptr; }

        bool operator!=(std::nullptr_t) const noexcept { return m_Asset != nullptr; }

        operator bool() const noexcept { return m_Asset != nullptr; }

        asset_ptr(const asset_ptr& other) = delete;
        asset_ptr& operator=(const asset_ptr& other) = delete;

    private:
        T* m_Asset;
    };
}
