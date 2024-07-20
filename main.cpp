#include "core/app.h"

using namespace std;

class DemoApp : public dx12demo::BaseWinApp
{
public:
    DemoApp(HINSTANCE hInstance) : BaseWinApp(hInstance) {}
};

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    DemoApp app(hInstance);

    if (!app.Initialize(nCmdShow))
    {
        return 1;
    }

    return app.Run();
}
