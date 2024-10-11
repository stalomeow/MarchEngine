#include "ConsoleWindow.h"
#include "Debug.h"
#include "EditorGUI.h"
#include "DotNetRuntime.h"

namespace march
{
    ConsoleWindow::ConsoleWindow()
        : m_LogTypeFilter(0)
        , m_LogMsgFilter{}
        , m_SelectedLog(-1)
        , m_AutoScroll(true)
    {
    }

    ImGuiWindowFlags ConsoleWindow::GetWindowFlags() const
    {
        return base::GetWindowFlags() | ImGuiWindowFlags_NoScrollbar;
    }

    void ConsoleWindow::OnDraw()
    {
        base::OnDraw();

        if (ImGui::Button("Clear"))
        {
            Debug::ClearLogs();
        }

        ImGui::SameLine();

        if (ImGui::Button("Options"))
        {
            ImGui::OpenPopup("Options");
        }

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::TextUnformatted("Filter (inc,-exc)");
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        ImGui::Combo("##LogTypeFilter", &m_LogTypeFilter, "All\0Info\0Warn\0Error\0\0");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        m_LogMsgFilter.Draw("##LogMsgFilter", ImGui::GetContentRegionAvail().x);

        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto Scroll", &m_AutoScroll);
            ImGui::EndPopup();
        }

        ImGui::SeparatorText(StringUtility::Format("%d Info | %d Warn | %d Error",
            Debug::GetLogCount(LogType::Info),
            Debug::GetLogCount(LogType::Warn),
            Debug::GetLogCount(LogType::Error)).c_str());

        ImVec2 totalContentSize = ImGui::GetContentRegionAvail();
        ImVec2 scrollRegionMinSize = ImVec2(totalContentSize.x, totalContentSize.y * 0.25f);
        ImVec2 scrollRegionMaxSize = ImVec2(totalContentSize.x, totalContentSize.y * 0.75f);
        ImGui::SetNextWindowSizeConstraints(scrollRegionMinSize, scrollRegionMaxSize);

        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_ResizeY | ImGuiChildFlags_Border, ImGuiWindowFlags_None))
        {
            for (int i = 0; i < Debug::s_Logs.size(); i++)
            {
                const auto& item = Debug::s_Logs[i];

                if ((m_LogTypeFilter == 1 && item.Type != LogType::Info) ||
                    (m_LogTypeFilter == 2 && item.Type != LogType::Warn) ||
                    (m_LogTypeFilter == 3 && item.Type != LogType::Error) ||
                    (m_LogMsgFilter.IsActive() && !m_LogMsgFilter.PassFilter(item.Message.c_str())))
                {
                    if (m_SelectedLog == i)
                    {
                        m_SelectedLog = -1;
                    }
                    continue;
                }

                float width = ImGui::GetContentRegionMax().x;
                float height = ImGui::GetTextLineHeight();
                ImVec2 cursorPos = ImGui::GetCursorPos();
                std::string label = "##LogItem" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), i == m_SelectedLog, ImGuiSelectableFlags_None, ImVec2(width, height)))
                {
                    m_SelectedLog = i;
                }

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Copy"))
                    {
                        ImGui::SetClipboardText(item.Message.c_str());
                    }

                    ImGui::EndPopup();
                }

                ImGui::SameLine();
                ImGui::SetCursorPos(cursorPos);

                DrawColorfulLogEntryText(&item);
            }

            // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
            // Using a scrollbar or mouse-wheel will take away from the bottom edge.
            if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("DetailedRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None))
        {
            if (m_SelectedLog >= 0 && m_SelectedLog < Debug::s_Logs.size())
            {
                const LogEntry& item = Debug::s_Logs[m_SelectedLog];

                ImGui::PushTextWrapPos();
                ImGui::TextUnformatted(item.Message.c_str());
                ImGui::Spacing();

                for (const LogStackFrame& frame : item.StackTrace)
                {
                    ImGui::Text("%s (at %s : %d)", frame.Function.c_str(), frame.Filename.c_str(), frame.Line);
                }

                ImGui::PopTextWrapPos();

                if (ImGui::BeginPopupContextWindow())
                {
                    if (ImGui::MenuItem("Copy"))
                    {
                        ImGui::SetClipboardText(item.Message.c_str());
                    }

                    ImGui::EndPopup();
                }
            }
            else
            {
                m_SelectedLog = -1;
            }
        }
        ImGui::EndChild();
    }

    void ConsoleWindow::DrawColorfulLogEntryText(const LogEntry* entry)
    {
        ImVec4 timeColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        timeColor.w = 0.6f;
        ImGui::PushStyleColor(ImGuiCol_Text, timeColor);
        ImGui::TextUnformatted(Debug::GetTimePrefix(entry->Time).c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine();

        ImVec4 severityColor;
        bool hasColor = false;
        if (entry->Type == LogType::Info) { severityColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); hasColor = true; }
        else if (entry->Type == LogType::Error) { severityColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); hasColor = true; }
        else if (entry->Type == LogType::Warn) { severityColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); hasColor = true; }

        if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, severityColor);
        ImGui::TextUnformatted(Debug::GetTypePrefix(entry->Type).c_str());
        if (hasColor) ImGui::PopStyleColor();

        ImGui::SameLine();

        size_t newlinePos = entry->Message.find_first_of("\r\n");
        if (newlinePos == std::string::npos)
        {
            ImGui::TextUnformatted(entry->Message.c_str());
        }
        else
        {
            ImGui::TextUnformatted(entry->Message.substr(0, newlinePos).c_str());
        }
    }

    void ConsoleWindow::DrawMainViewportSideBarConsole(IDotNetRuntime* dotnet)
    {
        if (EditorGUI::BeginMainViewportSideBar("##SingleLineConsoleWindow", ImGuiDir_Down, ImGui::GetTextLineHeight()))
        {
            if (!Debug::s_Logs.empty())
            {
                DrawColorfulLogEntryText(&Debug::s_Logs.back());
            }

            if (EditorGUI::IsWindowClicked(ImGuiMouseButton_Left))
            {
                dotnet->Invoke(ManagedMethod::EditorApplication_OpenConsoleWindowIfNot);
            }
        }

        EditorGUI::EndMainViewportSideBar();
    }

    int32_t ConsoleWindowInternalUtility::GetLogTypeFilter(ConsoleWindow* window)
    {
        return static_cast<int32_t>(window->m_LogTypeFilter);
    }

    void ConsoleWindowInternalUtility::SetLogTypeFilter(ConsoleWindow* window, int32_t value)
    {
        window->m_LogTypeFilter = static_cast<int>(value);
    }

    bool ConsoleWindowInternalUtility::GetAutoScroll(ConsoleWindow* window)
    {
        return window->m_AutoScroll;
    }

    void ConsoleWindowInternalUtility::SetAutoScroll(ConsoleWindow* window, bool value)
    {
        window->m_AutoScroll = value;
    }
}
