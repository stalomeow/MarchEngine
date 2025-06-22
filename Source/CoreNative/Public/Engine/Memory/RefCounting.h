#pragma once

#include "Engine/Ints.h"
#include <atomic>

namespace march
{
    class RefCountedObject
    {
        template <typename T>
        friend class RefCountPtr;

        std::atomic<uint32> m_RefCount;

    public:
        RefCountedObject(const RefCountedObject&) = delete;
        RefCountedObject& operator=(const RefCountedObject&) = delete;

        RefCountedObject(RefCountedObject&&) = delete;
        RefCountedObject& operator=(RefCountedObject&&) = delete;

        virtual ~RefCountedObject() = default;

    protected:
        RefCountedObject() noexcept : m_RefCount(1) {}
    };

    template <typename T>
    class RefCountPtr final
    {
        template <typename U>
        friend class RefCountPtr;

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
                m_Ptr->m_RefCount.fetch_add(1, std::memory_order_relaxed);
            }
        }

        void InternalRelease() noexcept
        {
            if (T* temp = m_Ptr; temp != nullptr)
            {
                m_Ptr = nullptr;

                if (temp->m_RefCount.fetch_sub(1, std::memory_order_release) == 1)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    delete temp;
                }
            }
        }
    };
}

#include "Engine/Object.h"

#define MARCH_MAKE_REF(T, ...) ::march::RefCountPtr<T>().Attach(MARCH_NEW T(__VA_ARGS__))

template <typename T>
struct std::hash<march::RefCountPtr<T>>
{
    auto operator()(const march::RefCountPtr<T>& ptr) const noexcept
    {
        return std::hash<T*>{}(ptr.Get());
    }
};
