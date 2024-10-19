#pragma once

#include <string>

namespace march
{
    class AssetManager final
    {
    public:
        static void* LoadAsset(const std::string& path);
        static void UnloadAsset(void* asset);
    };

    template <typename T>
    class UniqueAssetPtr final
    {
    public:
        UniqueAssetPtr() : m_Asset(nullptr) {}
        UniqueAssetPtr(nullptr_t) : m_Asset(nullptr) {}

        ~UniqueAssetPtr()
        {
            if (m_Asset != nullptr)
            {
                AssetManager::UnloadAsset(m_Asset);
                m_Asset = nullptr;
            }
        }

        UniqueAssetPtr(const UniqueAssetPtr& other) = delete;
        UniqueAssetPtr& operator=(const UniqueAssetPtr& other) = delete;

        UniqueAssetPtr(UniqueAssetPtr&& other) noexcept
        {
            m_Asset = other.m_Asset;
            other.m_Asset = nullptr;
        }

        UniqueAssetPtr& operator=(UniqueAssetPtr&& other) noexcept
        {
            if (this != &other)
            {
                if (m_Asset != nullptr)
                {
                    AssetManager::UnloadAsset(m_Asset);
                }

                m_Asset = other.m_Asset;
                other.m_Asset = nullptr;
            }

            return *this;
        }

        T* operator->() const noexcept
        {
            return m_Asset;
        }

        T* Get() const noexcept
        {
            return m_Asset;
        }

        UniqueAssetPtr& operator=(nullptr_t)
        {
            if (m_Asset != nullptr)
            {
                AssetManager::UnloadAsset(m_Asset);
                m_Asset = nullptr;
            }

            return *this;
        }

        void Reset()
        {
            if (m_Asset != nullptr)
            {
                AssetManager::UnloadAsset(m_Asset);
                m_Asset = nullptr;
            }
        }

        bool operator==(nullptr_t) const noexcept
        {
            return m_Asset == nullptr;
        }

        operator bool() const noexcept
        {
            return m_Asset != nullptr;
        }

        static UniqueAssetPtr Make(const std::string& path)
        {
            return UniqueAssetPtr(static_cast<T*>(AssetManager::LoadAsset(path)));
        }

    private:
        UniqueAssetPtr(T* asset) : m_Asset(asset) {}

        T* m_Asset;
    };
}
