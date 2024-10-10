#include "EditorWindow.h"

namespace march
{
    EditorWindow::EditorWindow() : m_Title(), m_Id(), m_FullName(), m_IsOpen(true)
    {
    }

    bool EditorWindow::Begin()
    {
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

    bool EditorWindow::GetIsOpen() const
    {
        return m_IsOpen;
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
