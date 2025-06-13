#pragma once

#include <imgui.h>

namespace march
{
    struct ImGuiStyleManager
    {
        static void ApplyDefaultStyle();
        static ImVec4 GetSystemWindowBackgroundColor();
    };
}
