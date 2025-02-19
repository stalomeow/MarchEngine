#include "pch.h"
#include "Editor/EditorWindow.h"
#include <imgui_internal.h>

namespace march
{
    EditorWindow::EditorWindow()
        : m_IsOpen(true)
        , m_Title()
        , m_Id()
        , m_FullName()
        , m_DefaultSize(600.0f, 350.0f)
    {
    }

    bool EditorWindow::Begin()
    {
        ImGui::SetNextWindowSize(m_DefaultSize, ImGuiCond_FirstUseEver);
        return ImGui::Begin(m_FullName.c_str(), &m_IsOpen, GetWindowFlags());
    }

    void EditorWindow::End()
    {
        ImGui::End();
    }

    ImGuiWindowFlags EditorWindow::GetWindowFlags() const
    {
        return ImGuiWindowFlags_None;
    }

    const std::string& EditorWindow::GetTitle() const
    {
        return m_Title;
    }

    const std::string& EditorWindow::GetId() const
    {
        return m_Id;
    }

    const std::string& EditorWindow::GetFullName() const
    {
        return m_FullName;
    }

    const ImVec2& EditorWindow::GetDefaultSize() const
    {
        return m_DefaultSize;
    }

    bool EditorWindow::GetIsOpen() const
    {
        return m_IsOpen;
    }

    ImGuiID EditorWindow::GetImGuiID() const
    {
        return ImGui::GetID(m_FullName.c_str());
    }

    void EditorWindow::SetTitle(const std::string& title)
    {
        m_Title = title;

        // https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-about-the-id-stack-system
        m_FullName = m_Title + "###" + m_Id;
    }

    void EditorWindow::SetId(const std::string& id)
    {
        m_Id = id;

        // https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-about-the-id-stack-system
        m_FullName = m_Title + "###" + m_Id;
    }

    void EditorWindow::SetDefaultSize(const ImVec2& size)
    {
        m_DefaultSize = size;
    }

    static ImGuiID g_MainViewportDockSpaceId = 0;

    void EditorWindow::DockSpaceOverMainViewport()
    {
        g_MainViewportDockSpaceId = ImGui::DockSpaceOverViewport();
    }

    ImGuiID EditorWindow::GetMainViewportDockSpaceNode()
    {
        return g_MainViewportDockSpaceId;
    }

    void EditorWindow::SplitDockNode(ImGuiID node, ImGuiDir splitDir, float sizeRatioForNodeAtDir, ImGuiID* pOutNodeAtDir, ImGuiID* pOutNodeAtOppositeDir)
    {
        ImGui::DockBuilderSplitNode(node, splitDir, sizeRatioForNodeAtDir, pOutNodeAtDir, pOutNodeAtOppositeDir);
    }

    void EditorWindow::ApplyModificationsInChildDockNodes(ImGuiID rootNode)
    {
        ImGui::DockBuilderFinish(rootNode);
    }

    void EditorWindow::DockIntoNode(ImGuiID node)
    {
        ImGui::DockBuilderDockWindow(GetFullName().c_str(), node);
    }

    bool EditorWindowInternalUtility::InvokeBegin(EditorWindow* window)
    {
        return window->Begin();
    }

    void EditorWindowInternalUtility::InvokeEnd(EditorWindow* window)
    {
        window->End();
    }

    void EditorWindowInternalUtility::SetTitle(EditorWindow* window, const std::string& title)
    {
        window->SetTitle(title);
    }

    void EditorWindowInternalUtility::SetId(EditorWindow* window, const std::string& id)
    {
        window->SetId(id);
    }

    void EditorWindowInternalUtility::SetDefaultSize(EditorWindow* window, const ImVec2& size)
    {
        window->SetDefaultSize(size);
    }

    void EditorWindowInternalUtility::SetIsOpen(EditorWindow* window, bool value)
    {
        window->m_IsOpen = value;
    }

    void EditorWindowInternalUtility::InvokeOnOpen(EditorWindow* window)
    {
        window->OnOpen();
    }

    void EditorWindowInternalUtility::InvokeOnClose(EditorWindow* window)
    {
        window->OnClose();
    }

    void EditorWindowInternalUtility::InvokeOnDraw(EditorWindow* window)
    {
        window->OnDraw();
    }
}
