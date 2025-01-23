#pragma once

#include "Engine/Ints.h"
#include <atomic>

namespace march
{
    class MarchObject
    {
    public:
        virtual ~MarchObject() = default;
    };

    class ThreadSafeRefCountedObject
    {
    public:
        uint32 AddRef() noexcept
        {
            return ++m_RefCount;
        }

        uint32 Release() noexcept
        {
            uint32 refCount = --m_RefCount;

            if (refCount == 0)
            {
                delete this;
            }

            return refCount;
        }

        ThreadSafeRefCountedObject(const ThreadSafeRefCountedObject&) = delete;
        ThreadSafeRefCountedObject& operator=(const ThreadSafeRefCountedObject&) = delete;

        ThreadSafeRefCountedObject(ThreadSafeRefCountedObject&&) = delete;
        ThreadSafeRefCountedObject& operator=(ThreadSafeRefCountedObject&&) = delete;

        virtual ~ThreadSafeRefCountedObject() = default;

    protected:
        ThreadSafeRefCountedObject() noexcept : m_RefCount(1) {}

    private:
        std::atomic<uint32> m_RefCount;
    };

    template <typename T, typename = std::enable_if_t<std::is_base_of_v<ThreadSafeRefCountedObject, T>>>
    class RefCountPtr final
    {
    public:
        RefCountPtr() noexcept : m_Ptr(nullptr)
        {
        }

        RefCountPtr(std::nullptr_t) noexcept : m_Ptr(nullptr)
        {
        }

        RefCountPtr(T* ptr) noexcept : m_Ptr(ptr)
        {
            InternalAddRef();
        }

        RefCountPtr(const RefCountPtr& other) noexcept : m_Ptr(other.m_Ptr)
        {
            InternalAddRef();
        }

        template <typename U>
        RefCountPtr(const RefCountPtr<U>& other, typename std::enable_if_t<std::is_convertible_v<U*, T*>>* = nullptr) noexcept : m_Ptr(other.m_Ptr)
        {
            InternalAddRef();
        }

        RefCountPtr(RefCountPtr&& other) noexcept : m_Ptr(nullptr)
        {
            // 下面定义了 operator&，所以这里取 other 的地址要先转成其他类型
            if (this != reinterpret_cast<RefCountPtr*>(&reinterpret_cast<uint8&>(other)))
            {
                Swap(other);
            }
        }

        ~RefCountPtr() noexcept
        {
            InternalRelease();
        }

        RefCountPtr& operator=(std::nullptr_t) noexcept
        {
            InternalRelease();
            return *this;
        }

        RefCountPtr& operator=(T* ptr) noexcept
        {
            if (m_Ptr != ptr)
            {
                RefCountPtr(ptr).Swap(*this);
            }
            return *this;
        }

        RefCountPtr& operator=(const RefCountPtr& other) noexcept
        {
            if (m_Ptr != other.m_Ptr)
            {
                RefCountPtr(other).Swap(*this);
            }
            return *this;
        }

        RefCountPtr& operator=(RefCountPtr&& other) noexcept
        {
            RefCountPtr(std::move(other)).Swap(*this);
            return *this;
        }

        T* operator->() const noexcept
        {
            return m_Ptr;
        }

        T* Get() const noexcept
        {
            return m_Ptr;
        }

        T* const* GetAddressOf() const noexcept
        {
            return &m_Ptr;
        }

        T** GetAddressOf() noexcept
        {
            return &m_Ptr;
        }

        T** ReleaseAndGetAddressOf() noexcept
        {
            InternalRelease();
            return &m_Ptr;
        }

        T** operator&() noexcept
        {
            return ReleaseAndGetAddressOf();
        }

        void Reset() noexcept
        {
            InternalRelease();
        }

        void Swap(RefCountPtr& other) noexcept
        {
            T* temp = m_Ptr;
            m_Ptr = other.m_Ptr;
            other.m_Ptr = temp;
        }

        operator bool() const noexcept
        {
            return m_Ptr != nullptr;
        }

        bool operator==(std::nullptr_t) const noexcept
        {
            return m_Ptr == nullptr;
        }

        bool operator==(const RefCountPtr& other) const noexcept
        {
            return m_Ptr == other.m_Ptr;
        }

        bool operator==(T* ptr) const noexcept
        {
            return m_Ptr == ptr;
        }

        bool operator!=(std::nullptr_t) const noexcept
        {
            return m_Ptr != nullptr;
        }

        bool operator!=(const RefCountPtr& other) const noexcept
        {
            return m_Ptr != other.m_Ptr;
        }

        bool operator!=(T* ptr) const noexcept
        {
            return m_Ptr != ptr;
        }

        template <typename U>
        RefCountPtr<U> As() const noexcept
        {
            return RefCountPtr<U>(static_cast<U*>(m_Ptr));
        }

        RefCountPtr& Attach(T* ptr) noexcept
        {
            if (m_Ptr != ptr)
            {
                InternalRelease();
                m_Ptr = ptr;
            }
            return *this;
        }

        T* Detach() noexcept
        {
            T* temp = m_Ptr;
            m_Ptr = nullptr;
            return temp;
        }

    private:
        T* m_Ptr;

        void InternalAddRef() const noexcept
        {
            if (m_Ptr != nullptr)
            {
                m_Ptr->AddRef();
            }
        }

        void InternalRelease() noexcept
        {
            if (T* temp = m_Ptr; temp != nullptr)
            {
                m_Ptr = nullptr;
                temp->Release();
            }
        }
    };
}

// https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library?view=msvc-170
#ifdef _DEBUG
    #include <crtdbg.h>
    #define MARCH_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
    // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the allocations to be of _CLIENT_BLOCK type
#else
    #define MARCH_NEW new
#endif

#define MARCH_MAKE_REF(T, ...) ::march::RefCountPtr<T>().Attach(MARCH_NEW T(__VA_ARGS__))

template <typename T>
struct std::hash<march::RefCountPtr<T>>
{
    auto operator()(const march::RefCountPtr<T>& ptr) const noexcept
    {
        return std::hash<T*>{}(ptr.Get());
    }
};
