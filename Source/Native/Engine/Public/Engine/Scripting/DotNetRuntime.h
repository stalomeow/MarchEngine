#pragma once

namespace march
{
    enum class ManagedMethod
    {
        Application_Initialize,
        Application_PostInitialize,
        Application_Tick,
        Application_Quit,
        Application_FullGC,
        EditorApplication_Initialize,
        EditorApplication_PostInitialize,
        EditorApplication_OpenConsoleWindowIfNot,
        AssetManager_NativeLoadAsset,
        AssetManager_NativeUnloadAsset,
        Mesh_NativeGetGeometry,
        Texture_NativeGetDefault,
        JobManager_NativeSchedule,
        JobManager_NativeComplete,

        // 该成员仅用于记录方法的数量
        NumMethods,
    };

    class IDotNetRuntime
    {
    public:
        virtual ~IDotNetRuntime() = default;

        IDotNetRuntime(const IDotNetRuntime&) = delete;
        IDotNetRuntime& operator=(const IDotNetRuntime&) = delete;

        IDotNetRuntime(IDotNetRuntime&&) = default;
        IDotNetRuntime& operator=(IDotNetRuntime&&) = default;

        template <typename Ret, typename... Args>
        Ret Invoke(ManagedMethod method, Args... args);
        void Invoke(ManagedMethod method) { Invoke<void>(method); }

    protected:
        IDotNetRuntime() = default;
        virtual void* GetFunctionPointer(ManagedMethod method) = 0;
    };

    template <typename Ret, typename... Args>
    Ret IDotNetRuntime::Invoke(ManagedMethod method, Args... args)
    {
        typedef Ret(*FunctionType)(Args...);
        auto func = reinterpret_cast<FunctionType>(GetFunctionPointer(method));
        return func(args...);
    }

    class DotNet
    {
    public:
        static void InitRuntime();
        static void DestroyRuntime();
        static IDotNetRuntime* GetRuntime();

        template <typename Ret, typename... Args>
        static Ret RuntimeInvoke(ManagedMethod method, Args... args)
        {
            return GetRuntime()->Invoke<Ret, Args...>(method, args...);
        }

        static void RuntimeInvoke(ManagedMethod method)
        {
            return GetRuntime()->Invoke(method);
        }
    };
}
