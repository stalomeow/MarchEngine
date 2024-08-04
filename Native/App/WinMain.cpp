#include "App/WinApplication.h"
#include "Rendering/GfxManager.h"
#include "Editor/GameEditor.h"

using namespace dx12demo;

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    WinApplication& app = GetApp();

    if (!app.Initialize(hInstance, nCmdShow))
    {
        return 0;
    }

    GameEditor editor{};
    return app.RunEngine(&editor);
}
