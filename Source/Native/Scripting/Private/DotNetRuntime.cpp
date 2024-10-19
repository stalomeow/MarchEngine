#include "DotNetRuntime.h"
#include "PathHelper.h"
#include "StringUtility.h"
#include "hostfxr.h"
#include "coreclr_delegates.h"
#include <Windows.h>
#include <string>
#include <unordered_map>
#include <stdint.h>
#include <stdexcept>
#include <memory>

namespace march
{
    // 为了可读性，这里不用数组
    // pair.first 是 type name，pair.second 是 method name
    const std::unordered_map<ManagedMethod, std::pair<LPCWSTR, LPCWSTR>> g_ManagedMethodConfig =
    {
        { ManagedMethod::Application_OnStart                      , { L"March.Core.Application,March.Core"          , L"OnStart"                 } },
        { ManagedMethod::Application_OnTick                       , { L"March.Core.Application,March.Core"          , L"OnTick"                  } },
        { ManagedMethod::Application_OnQuit                       , { L"March.Core.Application,March.Core"          , L"OnQuit"                  } },
        { ManagedMethod::EditorApplication_OnStart                , { L"March.Editor.EditorApplication,March.Editor", L"OnStart"                 } },
        { ManagedMethod::EditorApplication_OnTick                 , { L"March.Editor.EditorApplication,March.Editor", L"OnTick"                  } },
        { ManagedMethod::EditorApplication_OnQuit                 , { L"March.Editor.EditorApplication,March.Editor", L"OnQuit"                  } },
        { ManagedMethod::EditorApplication_OpenConsoleWindowIfNot , { L"March.Editor.EditorApplication,March.Editor", L"OpenConsoleWindowIfNot"  } },
        { ManagedMethod::AssetManager_NativeLoadAsset             , { L"March.Core.AssetManager,March.Core"         , L"NativeLoadAsset"         } },
        { ManagedMethod::AssetManager_NativeUnloadAsset           , { L"March.Core.AssetManager,March.Core"         , L"NativeUnloadAsset"       } },
    };

    const LPCWSTR g_ManagedRuntimeConfigFile = L"March.Core.runtimeconfig.json";

    const LPCWSTR g_ManagedAssemblies[] =
    {
        L"March.Core.dll",
        L"March.Editor.dll",
    };

    static std::wstring GetHostfxrPath()
    {
        std::wstring dir = PathHelper::GetWorkingDirectoryUtf16();
        return dir + L"\\Runtime\\host\\fxr\\8.0.8\\hostfxr.dll";
    }

    static std::wstring GetManagedFilePath(const LPCWSTR fileName)
    {
        std::wstring dir = PathHelper::GetWorkingDirectoryUtf16();
        return dir + L"\\Managed\\" + fileName;
    }

    class DotNetRuntimeImpl : public IDotNetRuntime
    {
    public:
        DotNetRuntimeImpl();
        ~DotNetRuntimeImpl() = default;

        DotNetRuntimeImpl(const DotNetRuntimeImpl&) = delete;
        DotNetRuntimeImpl& operator=(const DotNetRuntimeImpl&) = delete;

        DotNetRuntimeImpl(DotNetRuntimeImpl&&) = default;
        DotNetRuntimeImpl& operator=(DotNetRuntimeImpl&&) = default;

        void LoadAssemblies() const;

    protected:
        void* GetFunctionPointer(ManagedMethod method) override;

    private:
        load_assembly_and_get_function_pointer_fn m_LoadAssemblyAndGetFunctionPointer;
        get_function_pointer_fn m_GetFunctionPointer;
        load_assembly_fn m_LoadAssembly;

        void* m_Methods[static_cast<int>(ManagedMethod::NumMethods)];
    };

    DotNetRuntimeImpl::DotNetRuntimeImpl() : m_Methods{}
    {
        // Load hostfxr and get desired exports
        HMODULE hostfxr = LoadLibraryW(GetHostfxrPath().c_str());
        if (hostfxr == NULL)
        {
            throw std::runtime_error("Failed to load hostfxr.dll");
        }

        auto initFunc = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(GetProcAddress(hostfxr, "hostfxr_initialize_for_runtime_config"));
        auto getDelegateFunc = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(GetProcAddress(hostfxr, "hostfxr_get_runtime_delegate"));
        auto closeFunc = reinterpret_cast<hostfxr_close_fn>(GetProcAddress(hostfxr, "hostfxr_close"));
        if (initFunc == nullptr || getDelegateFunc == nullptr || closeFunc == nullptr)
        {
            FreeLibrary(hostfxr);
            throw std::runtime_error("Failed to get exports from hostfxr.dll");
        }

        // Load .NET
        hostfxr_handle contextHandle = nullptr;
        int32_t rc = initFunc(GetManagedFilePath(g_ManagedRuntimeConfigFile).c_str(), nullptr, &contextHandle);
        if (rc != 0 || contextHandle == nullptr)
        {
            closeFunc(contextHandle);
            throw std::runtime_error("Failed to initialize .NET runtime");
        }

        // Get the load assembly function pointer
        rc = getDelegateFunc(contextHandle, hdt_load_assembly_and_get_function_pointer,
            reinterpret_cast<void**>(&m_LoadAssemblyAndGetFunctionPointer));
        if (rc != 0 || m_LoadAssemblyAndGetFunctionPointer == nullptr)
        {
            closeFunc(contextHandle);
            throw std::runtime_error("Failed to get hdt_load_assembly_and_get_function_pointer");
        }

        rc = getDelegateFunc(contextHandle, hdt_get_function_pointer,
            reinterpret_cast<void**>(&m_GetFunctionPointer));
        if (rc != 0 || m_GetFunctionPointer == nullptr)
        {
            closeFunc(contextHandle);
            throw std::runtime_error("Failed to get hdt_get_function_pointer");
        }

        rc = getDelegateFunc(contextHandle, hdt_load_assembly,
            reinterpret_cast<void**>(&m_LoadAssembly));
        if (rc != 0 || m_LoadAssembly == nullptr)
        {
            closeFunc(contextHandle);
            throw std::runtime_error("Failed to get hdt_load_assembly");
        }

        closeFunc(contextHandle);
    }

    void DotNetRuntimeImpl::LoadAssemblies() const
    {
        for (LPCWSTR assembly : g_ManagedAssemblies)
        {
            std::wstring path = GetManagedFilePath(assembly);

            if (m_LoadAssembly(path.c_str(), nullptr, nullptr) != 0)
            {
                throw std::runtime_error("Failed to load assembly: " + StringUtility::Utf16ToUtf8(path));
            }
        }
    }

    void* DotNetRuntimeImpl::GetFunctionPointer(ManagedMethod method)
    {
        int index = static_cast<int>(method);

        if (m_Methods[index] == nullptr)
        {
            auto it = g_ManagedMethodConfig.find(method);
            if (it == g_ManagedMethodConfig.cend())
            {
                throw std::runtime_error("Invalid managed method");
            }

            LPCWSTR typeName = it->second.first;
            LPCWSTR methodName = it->second.second;

            int rc = m_GetFunctionPointer(typeName, methodName, UNMANAGEDCALLERSONLY_METHOD,
                nullptr, nullptr, &m_Methods[index]);
            if (rc != 0 || m_Methods[index] == nullptr)
            {
                throw std::runtime_error("Failed to get function pointer");
            }
        }

        return m_Methods[index];
    }

    static std::unique_ptr<DotNetRuntimeImpl> g_Runtime;

    void DotNet::InitRuntime()
    {
        g_Runtime = std::make_unique<DotNetRuntimeImpl>();
        g_Runtime->LoadAssemblies();
    }

    void DotNet::DestroyRuntime()
    {
        g_Runtime.reset();
    }

    IDotNetRuntime* DotNet::GetRuntime()
    {
        return g_Runtime.get();
    }
}
