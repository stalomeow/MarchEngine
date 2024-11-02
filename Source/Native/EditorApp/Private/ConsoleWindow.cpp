#include "ConsoleWindow.h"
#include "Debug.h"
#include "EditorGUI.h"
#include "DotNetRuntime.h"
#include <time.h>

namespace march
{
    ConsoleWindow::ConsoleWindow()
        : m_LogLevelFilter(0)
        , m_LogMsgFilter{}
        , m_SelectedLog(-1)
        , m_AutoScroll(true)
    {
    }

    ImGuiWindowFlags ConsoleWindow::GetWindowFlags() const
    {
        return base::GetWindowFlags() | ImGuiWindowFlags_NoScrollbar;
    }

    static std::string GetLogLevelPrefix(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error:   return "ERROR";
        default:                return "UNKNOWN";
        }
    }

    static std::string GetLogTimePrefix(time_t t)
    {
        struct tm timeInfo;
        if (localtime_s(&timeInfo, &t) != 0)
        {
            throw std::runtime_error("Failed to get local time.");
        }

        char tmp[32] = { 0 };
        strftime(tmp, sizeof(tmp), "[%H:%M:%S]", &timeInfo);
        return std::string(tmp);
    }

    void ConsoleWindow::OnDraw()
    {
        base::OnDraw();

        if (ImGui::Button("Clear"))
        {
            Log::Clear();
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
        ImGui::Combo("##LogLevelFilter", &m_LogLevelFilter, "All\0Trace\0Debug\0Info\0Warning\0Error\0\0");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        m_LogMsgFilter.Draw("##LogMsgFilter", ImGui::GetContentRegionAvail().x);

        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto Scroll", &m_AutoScroll);
            ImGui::EndPopup();
        }

        ImGui::SeparatorText(StringUtility::Format("%d Trace | %d Debug | %d Info | %d Warning | %d Error",
            Log::GetCount(LogLevel::Trace),
            Log::GetCount(LogLevel::Debug),
            Log::GetCount(LogLevel::Info),
            Log::GetCount(LogLevel::Warning),
            Log::GetCount(LogLevel::Error)).c_str());

        ImVec2 totalContentSize = ImGui::GetContentRegionAvail();
        ImVec2 scrollRegionMinSize = ImVec2(totalContentSize.x, totalContentSize.y * 0.25f);
        ImVec2 scrollRegionMaxSize = ImVec2(totalContentSize.x, totalContentSize.y * 0.75f);
        ImGui::SetNextWindowSizeConstraints(scrollRegionMinSize, scrollRegionMaxSize);

        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_ResizeY | ImGuiChildFlags_Border, ImGuiWindowFlags_None))
        {
            Log::ForEach([this](int32_t i, const LogEntry& entry)
            {
                if ((m_LogLevelFilter != 0 && (m_LogLevelFilter - 1 != static_cast<int32_t>(entry.Level))) ||
                    (m_LogMsgFilter.IsActive() && !m_LogMsgFilter.PassFilter(entry.Message.c_str())))
                {
                    if (m_SelectedLog == i)
                        m_SelectedLog = -1;
                    return;
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
                        ImGui::SetClipboardText(entry.Message.c_str());
                    }

                    ImGui::EndPopup();
                }

                ImGui::SameLine();
                ImGui::SetCursorPos(cursorPos);

                DrawColorfulLogEntryText(entry);
            });

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
            bool success = Log::ReadAt(m_SelectedLog, [this](const LogEntry& entry)
            {
                ImGui::PushTextWrapPos();
                ImGui::TextUnformatted(entry.Message.c_str());
                ImGui::Spacing();

                for (const LogStackFrame& frame : entry.StackTrace)
                {
                    ImGui::Text("%s (at %s : %d)", frame.Function.c_str(), frame.Filename.c_str(), frame.Line);
                }

                ImGui::PopTextWrapPos();

                if (ImGui::BeginPopupContextWindow())
                {
                    if (ImGui::MenuItem("Copy"))
                    {
                        ImGui::SetClipboardText(entry.Message.c_str());
                    }

                    ImGui::EndPopup();
                }
            });

            if (!success)
            {
                m_SelectedLog = -1;
            }
        }
        ImGui::EndChild();
    }

    void ConsoleWindow::DrawColorfulLogEntryText(const LogEntry& entry)
    {
        ImVec4 timeColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        timeColor.w = 0.6f;
        ImGui::PushStyleColor(ImGuiCol_Text, timeColor);
        ImGui::TextUnformatted(GetLogTimePrefix(entry.Time).c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine();

        ImVec4 severityColor{};
        bool hasColor = false;

        switch (entry.Level)
        {
        case LogLevel::Trace:
            severityColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            hasColor = true;
            break;

        case LogLevel::Debug:
            severityColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
            hasColor = true;
            break;

        case LogLevel::Info:
            severityColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            hasColor = true;
            break;

        case LogLevel::Warning:
            severityColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            hasColor = true;
            break;

        case LogLevel::Error:
            severityColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            hasColor = true;
            break;
        }

        if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, severityColor);
        ImGui::TextUnformatted(GetLogLevelPrefix(entry.Level).c_str());
        if (hasColor) ImGui::PopStyleColor();

        ImGui::SameLine();

        size_t newlinePos = entry.Message.find_first_of("\r\n");
        if (newlinePos == std::string::npos)
        {
            ImGui::TextUnformatted(entry.Message.c_str());
        }
        else
        {
            ImGui::TextUnformatted(entry.Message.substr(0, newlinePos).c_str());
        }
    }

    void ConsoleWindow::DrawMainViewportSideBarConsole()
    {
        if (EditorGUI::BeginMainViewportSideBar("##SingleLineConsoleWindow", ImGuiDir_Down, ImGui::GetTextLineHeight()))
        {
            Log::ReadLast([](const LogEntry& entry) { DrawColorfulLogEntryText(entry); });

            if (EditorGUI::IsWindowClicked(ImGuiMouseButton_Left))
            {
                DotNet::RuntimeInvoke(ManagedMethod::EditorApplication_OpenConsoleWindowIfNot);
            }
        }

        EditorGUI::EndMainViewportSideBar();
    }

    int32_t ConsoleWindowInternalUtility::GetLogTypeFilter(ConsoleWindow* window)
    {
        return static_cast<int32_t>(window->m_LogLevelFilter);
    }

    void ConsoleWindowInternalUtility::SetLogTypeFilter(ConsoleWindow* window, int32_t value)
    {
        window->m_LogLevelFilter = static_cast<int>(value);
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
