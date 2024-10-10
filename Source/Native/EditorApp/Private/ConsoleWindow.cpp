#include "ConsoleWindow.h"
#include "Debug.h"

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
        return ImGuiWindowFlags_NoScrollbar;
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

                ImVec4 timeColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                timeColor.w = 0.6f;
                ImGui::PushStyleColor(ImGuiCol_Text, timeColor);
                ImGui::TextUnformatted(Debug::GetTimePrefix(item.Time).c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine();

                ImVec4 color;
                bool has_color = false;
                if (item.Type == LogType::Info) { color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); has_color = true; }
                else if (item.Type == LogType::Error) { color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); has_color = true; }
                else if (item.Type == LogType::Warn) { color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); has_color = true; }

                if (has_color) ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(Debug::GetTypePrefix(item.Type).c_str());
                if (has_color) ImGui::PopStyleColor();
                ImGui::SameLine();

                size_t newlinePos = item.Message.find_first_of("\r\n");
                if (newlinePos == std::string::npos)
                {
                    ImGui::TextUnformatted(item.Message.c_str());
                }
                else
                {
                    ImGui::TextUnformatted(item.Message.substr(0, newlinePos).c_str());
                }
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
