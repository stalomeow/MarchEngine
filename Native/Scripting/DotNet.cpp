#include "Scripting/DotNet.h"
#include "Scripting/ScriptTypes.h"
#include "Core/Debug.h"
#include "App/WinApplication.h"
#include "Rendering/RenderObject.h"
#include "Rendering/Mesh.hpp"
#include "Rendering/RenderPipeline.h"
#include "Editor/EditorGUI.h"
#include "Core/IEngine.h"
#include <Windows.h>
#include <assert.h>
#include <unordered_map>
#include <string>

using namespace dx12demo::binding;

#define CSHARP_BINDING_ENTRY(name) { u8#name, ::dx12demo::binding::##name }

namespace dx12demo
{
    namespace
    {
        std::unordered_map<std::string, void*> g_ExportFuncs =
        {
            CSHARP_BINDING_ENTRY(MarshalString),
            CSHARP_BINDING_ENTRY(UnmarshalString),
            CSHARP_BINDING_ENTRY(FreeString),

            CSHARP_BINDING_ENTRY(Debug_Info),
            CSHARP_BINDING_ENTRY(Debug_Warn),
            CSHARP_BINDING_ENTRY(Debug_Error),

            CSHARP_BINDING_ENTRY(IEngine_GetRenderPipeline),

            CSHARP_BINDING_ENTRY(Application_GetDeltaTime),
            CSHARP_BINDING_ENTRY(Application_GetElapsedTime),
            CSHARP_BINDING_ENTRY(Application_GetEngine),
            CSHARP_BINDING_ENTRY(Application_GetDataPath),

            CSHARP_BINDING_ENTRY(RenderObject_New),
            CSHARP_BINDING_ENTRY(RenderObject_Delete),
            CSHARP_BINDING_ENTRY(RenderObject_SetPosition),
            CSHARP_BINDING_ENTRY(RenderObject_SetRotation),
            CSHARP_BINDING_ENTRY(RenderObject_SetScale),
            CSHARP_BINDING_ENTRY(RenderObject_GetMesh),
            CSHARP_BINDING_ENTRY(RenderObject_SetMesh),
            CSHARP_BINDING_ENTRY(RenderObject_GetIsActive),
            CSHARP_BINDING_ENTRY(RenderObject_SetIsActive),

            CSHARP_BINDING_ENTRY(SimpleMesh_New),
            CSHARP_BINDING_ENTRY(SimpleMesh_Delete),
            CSHARP_BINDING_ENTRY(SimpleMesh_ClearSubMeshes),
            CSHARP_BINDING_ENTRY(SimpleMesh_AddSubMeshCube),
            CSHARP_BINDING_ENTRY(SimpleMesh_AddSubMeshSphere),

            CSHARP_BINDING_ENTRY(RenderPipeline_AddRenderObject),
            CSHARP_BINDING_ENTRY(RenderPipeline_RemoveRenderObject),
            CSHARP_BINDING_ENTRY(RenderPipeline_AddLight),
            CSHARP_BINDING_ENTRY(RenderPipeline_RemoveLight),

            CSHARP_BINDING_ENTRY(Light_New),
            CSHARP_BINDING_ENTRY(Light_Delete),
            CSHARP_BINDING_ENTRY(Light_SetPosition),
            CSHARP_BINDING_ENTRY(Light_SetRotation),
            CSHARP_BINDING_ENTRY(Light_SetIsActive),
            CSHARP_BINDING_ENTRY(Light_GetType),
            CSHARP_BINDING_ENTRY(Light_SetType),
            CSHARP_BINDING_ENTRY(Light_GetColor),
            CSHARP_BINDING_ENTRY(Light_SetColor),
            CSHARP_BINDING_ENTRY(Light_GetFalloffRange),
            CSHARP_BINDING_ENTRY(Light_SetFalloffRange),
            CSHARP_BINDING_ENTRY(Light_GetSpotPower),
            CSHARP_BINDING_ENTRY(Light_SetSpotPower),

            CSHARP_BINDING_ENTRY(EditorGUI_PrefixLabel),
            CSHARP_BINDING_ENTRY(EditorGUI_FloatField),
            CSHARP_BINDING_ENTRY(EditorGUI_Vector2Field),
            CSHARP_BINDING_ENTRY(EditorGUI_Vector3Field),
            CSHARP_BINDING_ENTRY(EditorGUI_Vector4Field),
            CSHARP_BINDING_ENTRY(EditorGUI_ColorField),
            CSHARP_BINDING_ENTRY(EditorGUI_FloatSliderField),
            CSHARP_BINDING_ENTRY(EditorGUI_CollapsingHeader),
            CSHARP_BINDING_ENTRY(EditorGUI_Combo),
            CSHARP_BINDING_ENTRY(EditorGUI_CenterButton),
            CSHARP_BINDING_ENTRY(EditorGUI_Space),
            CSHARP_BINDING_ENTRY(EditorGUI_SeparatorText),
            CSHARP_BINDING_ENTRY(EditorGUI_TextField),
            CSHARP_BINDING_ENTRY(EditorGUI_Checkbox),
            CSHARP_BINDING_ENTRY(EditorGUI_BeginDisabled),
            CSHARP_BINDING_ENTRY(EditorGUI_EndDisabled),
            CSHARP_BINDING_ENTRY(EditorGUI_LabelField),
            CSHARP_BINDING_ENTRY(EditorGUI_PushIDString),
            CSHARP_BINDING_ENTRY(EditorGUI_PushIDInt),
            CSHARP_BINDING_ENTRY(EditorGUI_PopID),
            CSHARP_BINDING_ENTRY(EditorGUI_Foldout),
            CSHARP_BINDING_ENTRY(EditorGUI_Indent),
            CSHARP_BINDING_ENTRY(EditorGUI_Unindent),
            CSHARP_BINDING_ENTRY(EditorGUI_SameLine),
            CSHARP_BINDING_ENTRY(EditorGUI_GetContentRegionAvail),
            CSHARP_BINDING_ENTRY(EditorGUI_SetNextItemWidth),
            CSHARP_BINDING_ENTRY(EditorGUI_Separator),
            CSHARP_BINDING_ENTRY(EditorGUI_BeginPopup),
            CSHARP_BINDING_ENTRY(EditorGUI_EndPopup),
            CSHARP_BINDING_ENTRY(EditorGUI_MenuItem),
            CSHARP_BINDING_ENTRY(EditorGUI_BeginMenu),
            CSHARP_BINDING_ENTRY(EditorGUI_EndMenu),
            CSHARP_BINDING_ENTRY(EditorGUI_OpenPopup),
            CSHARP_BINDING_ENTRY(EditorGUI_FloatRangeField),
            CSHARP_BINDING_ENTRY(EditorGUI_BeginTreeNode),
            CSHARP_BINDING_ENTRY(EditorGUI_EndTreeNode),
            CSHARP_BINDING_ENTRY(EditorGUI_IsItemClicked),
            CSHARP_BINDING_ENTRY(EditorGUI_BeginPopupContextWindow),
            CSHARP_BINDING_ENTRY(EditorGUI_BeginPopupContextItem),
        };

        CSHARP_API(void*) LookUpExportFunc(CSharpChar* pKey, CSharpInt keyLength)
        {
            std::string key = CSharpString_ToUtf8(pKey, keyLength);
            auto func = g_ExportFuncs.find(key);
            return func == g_ExportFuncs.end() ? nullptr : func->second;
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
            L"DX12Demo.Core.Binding.NativeFunctionAttribute,DX12Demo.Core",
            L"SetLookUpFn",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            nullptr,
            reinterpret_cast<void**>(&m_SetLookUpFn));
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

    void DotNetEnv::InvokeMainFunc()
    {
        m_SetLookUpFn(LookUpExportFunc);
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
