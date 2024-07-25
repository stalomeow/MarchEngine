#pragma once

#include <Windows.h>

namespace dx12demo
{
    class IApplicationEventListener
    {
    public:
        virtual void OnAppStart() {}
        virtual void OnAppTick() {}
        virtual void OnAppQuit() {}

        virtual void OnAppResized() {}
        virtual void OnAppDisplayScaleChanged() {}
        virtual void OnAppPaint() {}

        virtual void OnAppPaused() {}
        virtual void OnAppResumed() {}

        virtual void OnAppMouseDown(WPARAM btnState, int x, int y) { }
        virtual void OnAppMouseUp(WPARAM btnState, int x, int y) { }
        virtual void OnAppMouseMove(WPARAM btnState, int x, int y) { }

        virtual void OnAppKeyDown(WPARAM btnState) { }
        virtual void OnAppKeyUp(WPARAM btnState) { }

        virtual bool OnAppMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult) { return false; }
    };
}
