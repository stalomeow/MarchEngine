#include "Scripting/DotNet.h"
#include "Scripting/ScriptTypes.h"
#include "Core/Debug.h"
#include <Windows.h>
#include <assert.h>
#include <unordered_map>

namespace dx12demo
{
    namespace
    {
        std::unordered_map<std::string, void*> g_ExportFuncs =
        {
            { "Debug_LogInfo", ::dx12demo::binding::Debug_Info },
            { "Debug_LogWarn", ::dx12demo::binding::Debug_Warn },
            { "Debug_LogError", ::dx12demo::binding::Debug_Error },
        };

        void* LookUpExportFunc(CSharpString key)
        {
            std::string k = CSharpString_ToUtf8(key);
            auto v = g_ExportFuncs.find(k);
            return v == g_ExportFuncs.end() ? nullptr : v->second;
        }
    }

    void DotNetEnv::Load()
    {
        // Pre-allocate a large buffer for the path to hostfxr
        char_t buffer[MAX_PATH];
        size_t buffer_size = sizeof(buffer) / sizeof(char_t);
        int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
        assert(rc == 0);

        // Load hostfxr and get desired exports
        HMODULE lib = LoadLibraryW(buffer);
        assert(lib != NULL);

        auto init_fptr = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(GetProcAddress(lib, "hostfxr_initialize_for_runtime_config"));
        auto get_delegate_fptr = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(GetProcAddress(lib, "hostfxr_get_runtime_delegate"));
        auto close_fptr = reinterpret_cast<hostfxr_close_fn>(GetProcAddress(lib, "hostfxr_close"));

        // Load .NET
        load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;
        get_function_pointer_fn get_function_pointer = nullptr;
        hostfxr_handle contextHandle = nullptr;
        rc = init_fptr(L"DX12Demo.Binding.runtimeconfig.json",
            nullptr, &contextHandle);

        if (rc != 0)
        {
            close_fptr(contextHandle);
        }

        assert(rc == 0);

        // Get the load assembly function pointer
        rc = get_delegate_fptr(
            contextHandle,
            hdt_load_assembly_and_get_function_pointer,
            reinterpret_cast<void**>(&load_assembly_and_get_function_pointer));
        assert(rc == 0 && load_assembly_and_get_function_pointer != nullptr);

        rc = get_delegate_fptr(
            contextHandle,
            hdt_get_function_pointer,
            reinterpret_cast<void**>(&get_function_pointer));
        assert(rc == 0 && get_function_pointer != nullptr);

        close_fptr(contextHandle);

        // Function pointer to managed delegate

        rc = load_assembly_and_get_function_pointer(
            L"DX12Demo.Binding.dll",
            L"DX12Demo.Binding.NativeFunctionAttribute,DX12Demo.Binding",
            L"SetLookUpFn",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            reinterpret_cast<void**>(&m_SetLookUpFn));
        assert(rc == 0);

        rc = load_assembly_and_get_function_pointer(
            L"DX12Demo.Binding.dll",
            L"DX12Demo.Binding.EntryPoint,DX12Demo.Binding",
            L"OnNativeTick",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            reinterpret_cast<void**>(&m_TickFunc));
        assert(rc == 0);
    }

    void DotNetEnv::InvokeMainFunc()
    {
        m_SetLookUpFn(LookUpExportFunc);
    }

    void DotNetEnv::InvokeTickFunc()
    {
        m_TickFunc();
    }
}
