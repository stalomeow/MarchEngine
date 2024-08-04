#pragma once

#include <Windows.h>

namespace dx12demo
{
    class IEngine
    {
    public:
        virtual void OnStart() {}
        virtual void OnTick() {}
        virtual void OnQuit() {}

        virtual void OnResized() {}
        virtual void OnDisplayScaleChanged() {}
        virtual void OnPaint() {}

        virtual void OnPaused() {}
        virtual void OnResumed() {}

        virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
        virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
        virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

        virtual void OnKeyDown(WPARAM btnState) { }
        virtual void OnKeyUp(WPARAM btnState) { }

        virtual bool OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult) { return false; }
    };
}
