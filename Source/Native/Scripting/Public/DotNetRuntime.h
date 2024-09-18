#pragma once

#include <memory>

namespace march
{
    enum class ManagedMethod : int
    {
        Initialize,
        Tick,
        DrawInspectorWindow,
        DrawProjectWindow,
        DrawHierarchyWindow,

        // 该成员仅用于记录方法的数量
        NumMethods,
    };

    class IDotNetRuntime
    {
    public:
        IDotNetRuntime() = default;
        virtual ~IDotNetRuntime() = default;

        IDotNetRuntime(const IDotNetRuntime&) = delete;
        IDotNetRuntime& operator=(const IDotNetRuntime&) = delete;

        IDotNetRuntime(IDotNetRuntime&&) = default;
        IDotNetRuntime& operator=(IDotNetRuntime&&) = default;

        template <typename Ret, typename ...Args>
        Ret Invoke(ManagedMethod method, Args ...args);

        void Invoke(ManagedMethod method) { Invoke<void>(method); }

    protected:
        virtual void* GetFunctionPointer(ManagedMethod method) = 0;
    };

    std::unique_ptr<IDotNetRuntime> CreateDotNetRuntime();

    template <typename Ret, typename ...Args>
    Ret IDotNetRuntime::Invoke(ManagedMethod method, Args ...args)
    {
        typedef Ret(*FunctionType)(Args...);
        auto func = reinterpret_cast<FunctionType>(GetFunctionPointer(method));
        return func(args...);
    }
}
