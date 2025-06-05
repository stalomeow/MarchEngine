#include "pch.h"
#include "ImGuiDemoWindow.h"
#include "imgui.h"

namespace march
{
    bool ImGuiDemoWindow::Begin()
    {
        return true;
    }

    void ImGuiDemoWindow::End()
    {

    }

    void ImGuiDemoWindow::OnDraw()
    {
        ImGui::ShowDemoWindow(&m_IsOpen);
    }
}
