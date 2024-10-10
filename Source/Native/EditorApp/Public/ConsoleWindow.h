#pragma once

#include "EditorWindow.h"
#include <imgui.h>
#include <stdint.h>

namespace march
{
    struct LogEntry;
    class IDotNetRuntime;
    class ConsoleWindowInternalUtility;

    class ConsoleWindow : public EditorWindow
    {
        using base = typename EditorWindow;
        friend ConsoleWindowInternalUtility;

    public:
        ConsoleWindow();
        virtual ~ConsoleWindow() = default;

        static void DrawMainViewportSideBarConsole(IDotNetRuntime* dotnet);

    protected:
        ImGuiWindowFlags GetWindowFlags() const override;
        void OnDraw() override;

    private:
        static void DrawColorfulLogEntryText(const LogEntry* entry);

        int m_LogTypeFilter;
        ImGuiTextFilter m_LogMsgFilter;
        int m_SelectedLog;
        bool m_AutoScroll;
    };

    class ConsoleWindowInternalUtility
    {
    public:
        static int32_t GetLogTypeFilter(ConsoleWindow* window);
        static void SetLogTypeFilter(ConsoleWindow* window, int32_t value);
        static bool GetAutoScroll(ConsoleWindow* window);
        static void SetAutoScroll(ConsoleWindow* window, bool value);
    };
}
