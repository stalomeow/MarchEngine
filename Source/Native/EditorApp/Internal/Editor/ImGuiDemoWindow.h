#pragma once

#include "Editor/EditorWindow.h"

namespace march
{
    class ImGuiDemoWindow : public EditorWindow
    {
    public:
        ImGuiDemoWindow() = default;
        virtual ~ImGuiDemoWindow() = default;

    protected:
        bool Begin() override;
        void End() override;
        void OnDraw() override;
    };
}
