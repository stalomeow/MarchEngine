#pragma once

#include <utility>

namespace march::DeferFuncPrivate
{
    template <typename FuncType>
    class DeferFunc
    {
    public:
        explicit DeferFunc(FuncType&& func)
            : m_Func(std::forward<FuncType>(func))
        {
        }

        ~DeferFunc()
        {
            m_Func();
        }

        DeferFunc(const DeferFunc&) = delete;
        DeferFunc& operator=(const DeferFunc&) = delete;

        DeferFunc(DeferFunc&&) = default;
        DeferFunc& operator=(DeferFunc&&) = default;

    private:
        FuncType m_Func;
    };

    struct DeferFuncSyntaxSupport
    {
        template <typename FuncType>
        DeferFunc<FuncType> operator+(FuncType&& func)
        {
            return DeferFunc<FuncType>(std::forward<FuncType>(func));
        }
    };
}

#define __DEFER_FUNC_NAME_JOIN(x, y) x##y
#define __DEFER_FUNC_NAME(x, y) __DEFER_FUNC_NAME_JOIN(x, y)

// 类似 golang 的 defer 语法糖
#define DEFER_FUNC() const auto __DEFER_FUNC_NAME(__DeferFunc, __LINE__) = ::march::DeferFuncPrivate::DeferFuncSyntaxSupport() + [&]()->void
