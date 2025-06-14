#include "pch.h"
#include "Engine/Scripting/DotNetRuntime.h"
#include "Engine/Misc/PathUtils.h"
#include "Engine/Misc/StringUtils.h"
#include "hostfxr.h"
#include "coreclr_delegates.h"
#include <Windows.h>
#include <string>
#include <unordered_map>
#include <stdint.h>
#include <stdexcept>
#include <memory>

// https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting

namespace march
{
    // 为了可读性，这里不用数组
    // pair.first 是 type name，pair.second 是 method name
    const std::unordered_map<ManagedMethod, std::pair<LPCWSTR, LPCWSTR>> g_ManagedMethodConfig =
    {
        { ManagedMethod::Application_Initialize                   , { L"March.Core.Application,March.Core"           , L"Initialize"             } },
        { ManagedMethod::Application_PostInitialize               , { L"March.Core.Application,March.Core"           , L"PostInitialize"         } },
        { ManagedMethod::Application_Tick                         , { L"March.Core.Application,March.Core"           , L"Tick"                   } },
        { ManagedMethod::Application_Quit                         , { L"March.Core.Application,March.Core"           , L"Quit"                   } },
        { ManagedMethod::Application_FullGC                       , { L"March.Core.Application,March.Core"           , L"FullGC"                 } },
        { ManagedMethod::EditorApplication_Initialize             , { L"March.Editor.EditorApplication,March.Editor" , L"Initialize"             } },
        { ManagedMethod::EditorApplication_PostInitialize         , { L"March.Editor.EditorApplication,March.Editor" , L"PostInitialize"         } },
        { ManagedMethod::EditorApplication_OpenConsoleWindowIfNot , { L"March.Editor.EditorApplication,March.Editor" , L"OpenConsoleWindowIfNot" } },
        { ManagedMethod::AssetManager_NativeLoadAsset             , { L"March.Core.AssetManager,March.Core"          , L"NativeLoadAsset"        } },
        { ManagedMethod::AssetManager_NativeUnloadAsset           , { L"March.Core.AssetManager,March.Core"          , L"NativeUnloadAsset"      } },
        { ManagedMethod::Mesh_NativeGetGeometry                   , { L"March.Core.Rendering.Mesh,March.Core"        , L"NativeGetGeometry"      } },
        { ManagedMethod::Texture_NativeGetDefault                 , { L"March.Core.Rendering.Texture,March.Core"     , L"NativeGetDefault"       } },
        { ManagedMethod::JobManager_NativeSchedule                , { L"March.Core.JobManager,March.Core"            , L"NativeSchedule"         } },
        { ManagedMethod::JobManager_NativeComplete                , { L"March.Core.JobManager,March.Core"            , L"NativeComplete"         } },
        { ManagedMethod::DragDrop_HandleExternalFiles             , { L"March.Editor.DragDrop,March.Editor"          , L"HandleExternalFiles"    } },
    };

    constexpr LPCWSTR g_ManagedAssemblies[] =
    {
        L"March.Core.dll",
        L"March.Editor.dll",
    };

    static std::wstring GetManagedFilePath(const LPCWSTR fileName)
    {
        std::wstring dir = PathUtils::GetWorkingDirectoryUtf16();
        return dir + L"\\" + fileName;
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
        HMODULE hostfxr = LoadLibraryW(GetManagedFilePath(DOTNET_HOSTFXR_PATH).c_str());
        if (hostfxr == NULL)
        {
            throw std::runtime_error("Failed to load hostfxr.dll");
        }

        auto initFunc = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(reinterpret_cast<void*>(GetProcAddress(hostfxr, "hostfxr_initialize_for_runtime_config")));
        auto getDelegateFunc = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(reinterpret_cast<void*>(GetProcAddress(hostfxr, "hostfxr_get_runtime_delegate")));
        auto closeFunc = reinterpret_cast<hostfxr_close_fn>(reinterpret_cast<void*>(GetProcAddress(hostfxr, "hostfxr_close")));
        if (initFunc == nullptr || getDelegateFunc == nullptr || closeFunc == nullptr)
        {
            FreeLibrary(hostfxr);
            throw std::runtime_error("Failed to get exports from hostfxr.dll");
        }

        // Load .NET
        hostfxr_handle contextHandle = nullptr;
        int32_t rc = initFunc(GetManagedFilePath(DOTNET_RUNTIME_CONFIG_PATH).c_str(), nullptr, &contextHandle);
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
                throw std::runtime_error("Failed to load assembly: " + StringUtils::Utf16ToUtf8(path));
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
