#include "Scripting/DotNet.h"
#include "Scripting/ScriptTypes.h"
#include "Core/Debug.h"
#include "App/WinApplication.h"
#include "Rendering/RenderObject.h"
#include "Rendering/Resource/Texture.h"
#include "Rendering/Mesh.hpp"
#include "Rendering/RenderPipeline.h"
#include "Rendering/Shader.h"
#include "Rendering/Material.h"
#include "Editor/EditorGUI.h"
#include "Core/IEngine.h"
#include <Windows.h>
#include <assert.h>
#include <unordered_map>
#include <string>

namespace dx12demo
{
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
        load_assembly_fn load_assembly = nullptr;
        hostfxr_handle contextHandle = nullptr;
        rc = init_fptr(L"DX12Demo.Core.runtimeconfig.json",
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

        rc = get_delegate_fptr(
            contextHandle,
            hdt_load_assembly,
            reinterpret_cast<void**>(&load_assembly));
        assert(rc == 0 && load_assembly != nullptr);

        close_fptr(contextHandle);

        // Function pointer to managed delegate
        char_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring basePath(path);
        size_t pos = basePath.find_last_of(L"\\");
        basePath = basePath.substr(0, pos + 1);

        rc = load_assembly((basePath + L"DX12Demo.Core.dll").c_str(), nullptr, nullptr);
        assert(rc == 0);

        rc = load_assembly((basePath + L"DX12Demo.Editor.dll").c_str(), nullptr, nullptr);
        assert(rc == 0);

        rc = get_function_pointer(
            L"DX12Demo.Core.EntryPoint,DX12Demo.Core",
            L"OnNativeTick",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            nullptr,
            reinterpret_cast<void**>(&m_TickFunc));
        assert(rc == 0);

        rc = get_function_pointer(
            L"DX12Demo.Core.EntryPoint,DX12Demo.Core",
            L"OnNativeInitialize",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            nullptr,
            reinterpret_cast<void**>(&m_InitFunc));
        assert(rc == 0);

        rc = get_function_pointer(
            L"DX12Demo.Editor.Windows.InspectorWindow,DX12Demo.Editor",
            L"Draw",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            nullptr,
            reinterpret_cast<void**>(&m_DrawInspectorFunc));
        assert(rc == 0);

        rc = get_function_pointer(
            L"DX12Demo.Editor.Windows.ProjectWindow,DX12Demo.Editor",
            L"Draw",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            nullptr,
            reinterpret_cast<void**>(&m_DrawProjectWindowFunc));
        assert(rc == 0);

        rc = get_function_pointer(
            L"DX12Demo.Editor.Windows.HierarchyWindow,DX12Demo.Editor",
            L"Draw",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            nullptr,
            reinterpret_cast<void**>(&m_DrawHierarchyWindowFunc));
        assert(rc == 0);
    }

    void DotNetEnv::InvokeTickFunc()
    {
        m_TickFunc();
    }

    void DotNetEnv::InvokeInitFunc()
    {
        m_InitFunc();
    }

    void DotNetEnv::InvokeDrawInspectorFunc()
    {
        m_DrawInspectorFunc();
    }

    void DotNetEnv::InvokeDrawProjectWindowFunc()
    {
        m_DrawProjectWindowFunc();
    }

    void DotNetEnv::InvokeDrawHierarchyWindowFunc()
    {
        m_DrawHierarchyWindowFunc();
    }
}
