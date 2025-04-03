#include "pch.h"
#include "Engine/ImGui/ImGuiStyleManager.h"
#include <imgui.h>

namespace march
{
    static constexpr ImVec4 ColorFromBytes(uint8_t r, uint8_t g, uint8_t b)
    {
        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    }

    static constexpr ImVec4 WithAlpha(const ImVec4& color, float alpha)
    {
        return ImVec4(color.x, color.y, color.z, alpha);
    }

    void ImGuiStyleManager::ApplyDefaultStyle()
    {
        // https://github.com/ocornut/imgui/issues/707

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        constexpr ImVec4 dockingEmptyBgColor = ColorFromBytes(18, 18, 18);
        constexpr ImVec4 bgColor = ColorFromBytes(25, 25, 26);
        constexpr ImVec4 menuColor = ColorFromBytes(35, 35, 36);
        constexpr ImVec4 lightBgColor = ColorFromBytes(90, 90, 92);
        constexpr ImVec4 veryLightBgColor = ColorFromBytes(110, 110, 115);

        constexpr ImVec4 panelColor = ColorFromBytes(55, 55, 59);
        constexpr ImVec4 panelHoverColor = ColorFromBytes(35, 80, 142);
        constexpr ImVec4 panelActiveColor = ColorFromBytes(0, 95, 170);

        constexpr ImVec4 textColor = ColorFromBytes(230, 230, 230);
        constexpr ImVec4 textHighlightColor = ColorFromBytes(255, 255, 255);
        constexpr ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
        constexpr ImVec4 borderColor = ColorFromBytes(58, 58, 58);

        colors[ImGuiCol_Text] = textColor;
        colors[ImGuiCol_TextDisabled] = textDisabledColor;
        colors[ImGuiCol_TextSelectedBg] = panelActiveColor;
        colors[ImGuiCol_WindowBg] = bgColor;
        colors[ImGuiCol_ChildBg] = bgColor;
        colors[ImGuiCol_PopupBg] = bgColor;
        colors[ImGuiCol_Border] = borderColor;
        colors[ImGuiCol_BorderShadow] = borderColor;
        colors[ImGuiCol_FrameBg] = panelColor;
        colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
        colors[ImGuiCol_FrameBgActive] = panelActiveColor;
        colors[ImGuiCol_TitleBg] = dockingEmptyBgColor;
        colors[ImGuiCol_TitleBgActive] = dockingEmptyBgColor;
        colors[ImGuiCol_TitleBgCollapsed] = dockingEmptyBgColor;
        colors[ImGuiCol_MenuBarBg] = menuColor;
        colors[ImGuiCol_ScrollbarBg] = panelColor;
        colors[ImGuiCol_ScrollbarGrab] = lightBgColor;
        colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
        colors[ImGuiCol_ScrollbarGrabActive] = veryLightBgColor;
        colors[ImGuiCol_CheckMark] = textColor;
        colors[ImGuiCol_SliderGrab] = WithAlpha(textColor, 0.4f);
        colors[ImGuiCol_SliderGrabActive] = WithAlpha(textHighlightColor, 0.4f);
        colors[ImGuiCol_Button] = panelColor;
        colors[ImGuiCol_ButtonHovered] = panelHoverColor;
        colors[ImGuiCol_ButtonActive] = panelActiveColor;
        colors[ImGuiCol_Header] = panelColor;
        colors[ImGuiCol_HeaderHovered] = panelHoverColor;
        colors[ImGuiCol_HeaderActive] = panelActiveColor;
        colors[ImGuiCol_Separator] = borderColor;
        colors[ImGuiCol_SeparatorHovered] = panelHoverColor;
        colors[ImGuiCol_SeparatorActive] = panelActiveColor;
        colors[ImGuiCol_ResizeGrip] = bgColor;
        colors[ImGuiCol_ResizeGripHovered] = panelHoverColor;
        colors[ImGuiCol_ResizeGripActive] = panelActiveColor;
        colors[ImGuiCol_PlotLines] = panelActiveColor;
        colors[ImGuiCol_PlotLinesHovered] = panelHoverColor;
        colors[ImGuiCol_PlotHistogram] = panelActiveColor;
        colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
        colors[ImGuiCol_ModalWindowDimBg] = bgColor;
        colors[ImGuiCol_DragDropTarget] = panelActiveColor;
        colors[ImGuiCol_NavHighlight] = bgColor;
        colors[ImGuiCol_DockingPreview] = panelActiveColor;
        colors[ImGuiCol_DockingEmptyBg] = dockingEmptyBgColor;
        colors[ImGuiCol_Tab] = bgColor;
        colors[ImGuiCol_TabActive] = panelColor;
        colors[ImGuiCol_TabUnfocused] = bgColor;
        colors[ImGuiCol_TabUnfocusedActive] = panelColor;
        colors[ImGuiCol_TabHovered] = panelColor;
        colors[ImGuiCol_TabDimmedSelected] = panelColor;
        colors[ImGuiCol_TabDimmedSelectedOverline] = panelColor;
        colors[ImGuiCol_TabSelectedOverline] = panelActiveColor;

        //style.WindowMenuButtonPosition = ImGuiDir_None;
        style.WindowRounding = 3.0f;
        style.ChildRounding = 3.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 3.0f;
        style.TabBarBorderSize = 1.0f;
        style.TabBarOverlineSize = 2.0f;
        style.DockingSeparatorSize = 1.0f;
    }
}
