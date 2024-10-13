#pragma once

#include "RenderPipeline.h"
#include <Windows.h>
#include <vector>
#include <string>

namespace march
{
    class IEngine
    {
    public:
        virtual ~IEngine() = default;

        virtual void OnStart(const std::vector<std::string>& args) {}
        virtual void OnTick(bool willQuit) {}
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

    public:
        virtual RenderPipeline* GetRenderPipeline() = 0;
    };
}
