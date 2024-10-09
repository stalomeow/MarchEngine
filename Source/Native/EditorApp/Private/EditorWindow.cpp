#include "EditorWindow.h"

namespace march
{
    EditorWindow::EditorWindow() : EditorWindow("Untitled")
    {
    }

    EditorWindow::EditorWindow(const std::string& title) : m_Title(title), m_IsOpen(true)
    {
    }

    bool EditorWindow::Begin()
    {
        return ImGui::Begin(m_Title.c_str(), &m_IsOpen, GetWindowFlags());
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

    bool EditorWindow::GetIsOpen() const
    {
        return m_IsOpen;
    }

    void EditorWindow::SetTitle(const std::string& title)
    {
        m_Title = title;
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
