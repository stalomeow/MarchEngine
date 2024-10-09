#pragma once

#include <string>
#include <imgui.h>

namespace march
{
    class EditorWindowInternalUtility;

    class EditorWindow
    {
        friend EditorWindowInternalUtility;

    public:
        EditorWindow();
        EditorWindow(const std::string& title);
        virtual ~EditorWindow() = default;

        EditorWindow(const EditorWindow&) = delete;
        EditorWindow& operator=(const EditorWindow&) = delete;
        EditorWindow(EditorWindow&&) = delete;
        EditorWindow& operator=(EditorWindow&&) = delete;

        const std::string& GetTitle() const;
        bool GetIsOpen() const;

    protected:
        void SetTitle(const std::string& title);

        virtual ImGuiWindowFlags GetWindowFlags() const;
        virtual void OnOpen() {}
        virtual void OnClose() {}
        virtual void OnDraw() {}

    private:
        bool Begin();
        void End();

        std::string m_Title;
        bool m_IsOpen;
    };

    class EditorWindowInternalUtility
    {
    public:
        static bool InvokeBegin(EditorWindow* window);
        static void InvokeEnd(EditorWindow* window);
        static void SetTitle(EditorWindow* window, const std::string& title);
        static void SetIsOpen(EditorWindow* window, bool value);
        static void InvokeOnOpen(EditorWindow* window);
        static void InvokeOnClose(EditorWindow* window);
        static void InvokeOnDraw(EditorWindow* window);
    };
}
