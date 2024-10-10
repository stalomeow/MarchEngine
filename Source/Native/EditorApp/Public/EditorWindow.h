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
        virtual ~EditorWindow() = default;

        EditorWindow(const EditorWindow&) = delete;
        EditorWindow& operator=(const EditorWindow&) = delete;
        EditorWindow(EditorWindow&&) = delete;
        EditorWindow& operator=(EditorWindow&&) = delete;

        const std::string& GetTitle() const;
        const std::string& GetId() const;
        bool GetIsOpen() const;

    protected:
        void SetTitle(const std::string& title);
        void SetId(const std::string& id);

        virtual ImGuiWindowFlags GetWindowFlags() const;
        virtual void OnOpen() {}
        virtual void OnClose() {}
        virtual void OnDraw() {}

    private:
        bool Begin();
        void End();

        std::string m_Title;
        std::string m_Id;
        std::string m_FullName;
        bool m_IsOpen;
    };

    class EditorWindowInternalUtility
    {
    public:
        static bool InvokeBegin(EditorWindow* window);
        static void InvokeEnd(EditorWindow* window);
        static void SetTitle(EditorWindow* window, const std::string& title);
        static void SetId(EditorWindow* window, const std::string& id);
        static void SetIsOpen(EditorWindow* window, bool value);
        static void InvokeOnOpen(EditorWindow* window);
        static void InvokeOnClose(EditorWindow* window);
        static void InvokeOnDraw(EditorWindow* window);
    };
}
