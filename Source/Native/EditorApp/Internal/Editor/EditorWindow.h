#pragma once

#include "Engine/Object.h"
#include <string>
#include "imgui.h"

namespace march
{
    class EditorWindowInternalUtility;

    class EditorWindow : public MarchObject
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
        const std::string& GetFullName() const;
        const ImVec2& GetDefaultSize() const;
        bool GetIsOpen() const;

        ImGuiID GetImGuiID() const;

        // Docking API
        // Usage Ref: https://github.com/ocornut/imgui/issues/4430

        static void DockSpaceOverMainViewport();
        static ImGuiID GetMainViewportDockSpaceNode();
        static void SplitDockNodeHorizontal(ImGuiID node, float sizeRatioForLeftNode, ImGuiID* pOutLeftNode, ImGuiID* pOutRightNode);
        static void SplitDockNodeVertical(ImGuiID node, float sizeRatioForTopNode, ImGuiID* pOutTopNode, ImGuiID* pOutBottomNode);
        static void ApplyModificationsInChildDockNodes(ImGuiID rootNode);
        void DockIntoNode(ImGuiID node);

    protected:
        void SetTitle(const std::string& title);
        void SetId(const std::string& id);
        void SetDefaultSize(const ImVec2& size);

        virtual bool Begin();
        virtual void End();
        virtual ImGuiWindowFlags GetWindowFlags() const;

        virtual void OnOpen() {}
        virtual void OnClose() {}
        virtual void OnDraw() {}

        bool m_IsOpen;

    private:
        std::string m_Title;
        std::string m_Id;
        std::string m_FullName;
        ImVec2 m_DefaultSize;
    };

    class EditorWindowInternalUtility
    {
    public:
        static bool InvokeBegin(EditorWindow* window);
        static void InvokeEnd(EditorWindow* window);
        static void SetTitle(EditorWindow* window, const std::string& title);
        static void SetId(EditorWindow* window, const std::string& id);
        static void SetDefaultSize(EditorWindow* window, const ImVec2& size);
        static void SetIsOpen(EditorWindow* window, bool value);
        static void InvokeOnOpen(EditorWindow* window);
        static void InvokeOnClose(EditorWindow* window);
        static void InvokeOnDraw(EditorWindow* window);
    };
}
