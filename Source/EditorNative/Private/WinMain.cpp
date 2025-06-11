#include "pch.h"
#include "EditorApplication.h"
#include "Engine/Misc/DeferFunc.h"
#include <memory>
#include <Windows.h>
#include <ole2.h>

using namespace march;

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;}
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    // Enable run-time memory check for debug builds.
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    auto app = std::make_unique<EditorApplication>();

    // 初始化 OLE 用于支持 DragDrop
    if (FAILED(OleInitialize(nullptr)))
    {
        app->CrashWithMessage("Error", "Failed to initialize OLE.", /* debugBreak */ false);
    }

    DEFER_FUNC() { OleUninitialize(); };

    return app->Run(hInstance, lpCmdLine, nCmdShow);
}
