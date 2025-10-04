#pragma once

#include "Engine/STL/Core.h"

namespace march::stl
{
    template <typename>
    class function;

    // TODO 小对象优化

    template <typename Ret, typename... Args>
    class function<Ret(Args...)>
    {
        class Callable
        {
        public:
            virtual ~Callable() = default;
            virtual Ret Invoke(Args... args) = 0;
        };

        template <typename Func>
        class CallableImpl : public Callable
        {
        public:
            CallableImpl(const Func&& func) : m_Func(func) {}
            CallableImpl(Func&& func) : m_Func(std::move(func)) {}
            Ret Invoke(Args... args) override { return m_Func(std::forward<Args>(args)...); }

        private:
            Func m_Func;
        };

        stl::unique_ptr<Callable> m_Callable;

    public:
        function() : m_Callable(nullptr) {}
        function(nullptr_t) : m_Callable(nullptr) {}

        template <typename Func, typename = std::enable_if_t<std::is_invocable_r_v<Ret, Func, Args...>>>
        function(MemoryLabel label, Func&& func)
        {
            m_Callable = stl::make_unique<CallableImpl<std::decay_t<Func>>>(label, std::forward<Func>(func));
        }

        operator bool() const { return m_Callable != nullptr; }

        Ret operator()(Args... args)
        {
            if (m_Callable == nullptr)
                throw std::bad_function_call();
            return m_Callable->Invoke(std::forward<Args>(args)...);
        }
    };
}
