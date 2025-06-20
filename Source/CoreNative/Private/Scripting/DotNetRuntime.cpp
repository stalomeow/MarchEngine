#include "pch.h"
#include "Engine/Scripting/DotNetRuntime.h"
#include "Engine/Misc/PlatformUtils.h"
#include "Engine/Misc/StringUtils.h"
#include "hostfxr.h"
#include "coreclr_delegates.h"
#include <string>
#include <unordered_map>
#include <stdint.h>
#include <stdexcept>
#include <memory>

// https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting

#ifdef PLATFORM_WINDOWS
#define DOTNET_C_STR(s) ::march::PlatformUtils::Windows::Utf8ToWide(s).c_str()
#else
#define DOTNET_C_STR(s) (s).c_str()
#endif

namespace march
{
    // 为了可读性，这里不用数组
    // pair.first 是 type name，pair.second 是 method name
    const std::unordered_map<ManagedMethod, std::pair<LPCSTR, LPCSTR>> g_ManagedMethodConfig =
    {
        { ManagedMethod::Application_Initialize                   , { "March.Core.Application,March.Core"           , "Initialize"             } },
        { ManagedMethod::Application_PostInitialize               , { "March.Core.Application,March.Core"           , "PostInitialize"         } },
        { ManagedMethod::Application_Tick                         , { "March.Core.Application,March.Core"           , "Tick"                   } },
        { ManagedMethod::Application_Quit                         , { "March.Core.Application,March.Core"           , "Quit"                   } },
        { ManagedMethod::Application_FullGC                       , { "March.Core.Application,March.Core"           , "FullGC"                 } },
        { ManagedMethod::EditorApplication_Initialize             , { "March.Editor.EditorApplication,March.Editor" , "Initialize"             } },
        { ManagedMethod::EditorApplication_PostInitialize         , { "March.Editor.EditorApplication,March.Editor" , "PostInitialize"         } },
        { ManagedMethod::EditorApplication_OpenConsoleWindowIfNot , { "March.Editor.EditorApplication,March.Editor" , "OpenConsoleWindowIfNot" } },
        { ManagedMethod::AssetManager_NativeLoadAsset             , { "March.Core.AssetManager,March.Core"          , "NativeLoadAsset"        } },
        { ManagedMethod::AssetManager_NativeUnloadAsset           , { "March.Core.AssetManager,March.Core"          , "NativeUnloadAsset"      } },
        { ManagedMethod::Mesh_NativeGetGeometry                   , { "March.Core.Rendering.Mesh,March.Core"        , "NativeGetGeometry"      } },
        { ManagedMethod::Texture_NativeGetDefault                 , { "March.Core.Rendering.Texture,March.Core"     , "NativeGetDefault"       } },
        { ManagedMethod::JobManager_NativeSchedule                , { "March.Core.JobManager,March.Core"            , "NativeSchedule"         } },
        { ManagedMethod::JobManager_NativeComplete                , { "March.Core.JobManager,March.Core"            , "NativeComplete"         } },
        { ManagedMethod::DragDrop_HandleExternalFiles             , { "March.Editor.DragDrop,March.Editor"          , "HandleExternalFiles"    } },
    };

    constexpr LPCSTR g_ManagedAssemblies[] =
    {
        "March.Core.dll",
        "March.Editor.dll",
    };

    static std::string GetDotNetFilePath(const LPCSTR fileName)
    {
        return PlatformUtils::GetExecutableDirectory() + "/" + fileName;
    }

    class DotNetRuntimeImpl : public IDotNetRuntime
    {
    public:
        DotNetRuntimeImpl();
        ~DotNetRuntimeImpl() override = default;

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
        void* hostfxr = PlatformUtils::GetDllHandle(GetDotNetFilePath(DOTNET_HOSTFXR_PATH));

        if (hostfxr == nullptr)
        {
            throw std::runtime_error("Failed to load hostfxr");
        }

        auto initFunc = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(PlatformUtils::GetDllExport(hostfxr, "hostfxr_initialize_for_runtime_config"));
        auto getDelegateFunc = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(PlatformUtils::GetDllExport(hostfxr, "hostfxr_get_runtime_delegate"));
        auto closeFunc = reinterpret_cast<hostfxr_close_fn>(PlatformUtils::GetDllExport(hostfxr, "hostfxr_close"));

        if (initFunc == nullptr || getDelegateFunc == nullptr || closeFunc == nullptr)
        {
            PlatformUtils::FreeDllHandle(hostfxr);
            throw std::runtime_error("Failed to get exports from hostfxr");
        }

        // Load .NET
        hostfxr_handle contextHandle = nullptr;
        int32_t rc = initFunc(DOTNET_C_STR(GetDotNetFilePath(DOTNET_RUNTIME_CONFIG_PATH)), nullptr, &contextHandle);
        if (rc != 0 || contextHandle == nullptr)
        {
            closeFunc(contextHandle);
            throw std::runtime_error("Failed to initialize .NET runtime");
        }

        // Get the load assembly function pointer
        rc = getDelegateFunc(contextHandle, hdt_load_assembly_and_get_function_pointer, reinterpret_cast<void**>(&m_LoadAssemblyAndGetFunctionPointer));
        if (rc != 0 || m_LoadAssemblyAndGetFunctionPointer == nullptr)
        {
            closeFunc(contextHandle);
            throw std::runtime_error("Failed to get hdt_load_assembly_and_get_function_pointer");
        }

        rc = getDelegateFunc(contextHandle, hdt_get_function_pointer, reinterpret_cast<void**>(&m_GetFunctionPointer));
        if (rc != 0 || m_GetFunctionPointer == nullptr)
        {
            closeFunc(contextHandle);
            throw std::runtime_error("Failed to get hdt_get_function_pointer");
        }

        rc = getDelegateFunc(contextHandle, hdt_load_assembly, reinterpret_cast<void**>(&m_LoadAssembly));
        if (rc != 0 || m_LoadAssembly == nullptr)
        {
            closeFunc(contextHandle);
            throw std::runtime_error("Failed to get hdt_load_assembly");
        }

        closeFunc(contextHandle);
    }

    void DotNetRuntimeImpl::LoadAssemblies() const
    {
        for (LPCSTR assembly : g_ManagedAssemblies)
        {
            std::string path = GetDotNetFilePath(assembly);

            if (m_LoadAssembly(DOTNET_C_STR(path), nullptr, nullptr) != 0)
            {
                throw std::runtime_error("Failed to load assembly: " + path);
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

            LPCSTR typeName = it->second.first;
            LPCSTR methodName = it->second.second;

            int rc = m_GetFunctionPointer(DOTNET_C_STR(typeName), DOTNET_C_STR(methodName), UNMANAGEDCALLERSONLY_METHOD,
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
