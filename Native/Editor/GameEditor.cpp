#include "Editor/GameEditor.h"
#include "App/WinApplication.h"
#include "Rendering/DxException.h"
#include "Rendering/GfxManager.h"
#include "Editor/EditorGUI.h"
#include "Rendering/Command/CommandBuffer.h"
#include "Rendering/Resource/GpuBuffer.h"
#include "Core/Debug.h"
#include "Core/StringUtility.h"
#include <DirectXMath.h>
#include <stdint.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <tuple>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace dx12demo
{
    // Win32 message handler
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    bool GameEditor::OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult)
    {
        if (ImGui_ImplWin32_WndProcHandler(GetApp().GetHWND(), msg, wParam, lParam))
        {
            outResult = true;
            return true;
        }
        return false;
    }

    void GameEditor::OnStart()
    {
        m_RenderDoc.Load(); // 越早越好
        m_DotNet.Load(); // 越早越好，mixed debugger 需要 runtime 加载完后才能工作

        auto [width, height] = GetApp().GetClientWidthAndHeight();
        GetGfxManager().Initialize(GetApp().GetHWND(), width, height, 2, 0);
        m_RenderPipeline = std::make_unique<RenderPipeline>(width, height);
        m_StaticDescriptorViewTable = GetGfxManager().GetViewDescriptorTableAllocator()->GetStaticTable();

        auto device = GetGfxManager().GetDevice();
        auto srvHandle = m_StaticDescriptorViewTable.GetCpuHandle(1);
        device->CreateShaderResourceView(m_RenderPipeline->GetResolvedColorTarget(), nullptr, srvHandle);

        InitImGui();

        m_DotNet.InvokeInitFunc();
    }

    void GameEditor::InitImGui()
    {
        // Setup Dear ImGui context
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(GetApp().GetHWND());
        io.Fonts->AddFontFromFileTTF(m_FontPath, m_FontSize * GetApp().GetDisplayScale(),
            nullptr, io.Fonts->GetGlyphRangesChineseFull());
        io.Fonts->AddFontDefault();
        io.Fonts->Build();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
        ImGui::GetStyle().FrameBorderSize = 1.0f;
        ImGui::GetStyle().FrameRounding = 2.0f;

        auto device = GetGfxManager().GetDevice();
        ImGui_ImplDX12_Init(device, GetGfxManager().GetMaxFrameLatency(),
            GetGfxManager().GetBackBufferFormat(), m_StaticDescriptorViewTable.GetHeapPointer(),
            m_StaticDescriptorViewTable.GetCpuHandle(0), m_StaticDescriptorViewTable.GetGpuHandle(0));
    }

    void GameEditor::OnQuit()
    {
        GetGfxManager().WaitForGpuIdle();
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void GameEditor::DrawImGui()
    {
        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static bool showStyleEditor = false;
        static bool showMetrics = false;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window"))
            {
                if (ImGui::BeginMenu("ImGui Tools"))
                {
                    if (ImGui::MenuItem("Style Editor"))
                    {
                        showStyleEditor = true;
                    }

                    if (ImGui::MenuItem("Metrics"))
                    {
                        showMetrics = true;
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Console"))
                {
                    m_ShowConsoleWindow = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_C, ImGuiInputFlags_RouteAlways))
            {
                m_RenderDoc.CaptureSingleFrame();
            }

            if (ImGui::BeginMenu("RenderDoc"))
            {
                if (ImGui::MenuItem("Capture", "Alt+C", nullptr, m_RenderDoc.IsLoaded()))
                {
                    m_RenderDoc.CaptureSingleFrame();
                }

                ImGui::SeparatorText("Information");

                if (ImGui::BeginMenu("Library"))
                {
                    ImGui::TextUnformatted(m_RenderDoc.GetLibraryPath().c_str());
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("API Version"))
                {
                    auto [major, minor, patch] = m_RenderDoc.GetVersion();
                    ImGui::Text("%d.%d.%d", major, minor, patch);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Num Captures"))
                {
                    ImGui::Text("%d", m_RenderDoc.GetNumCaptures());
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        ImGui::DockSpaceOverViewport();

        if (showStyleEditor)
        {
            ImGui::Begin("Style Editor", &showStyleEditor);
            ImGui::ShowStyleEditor();
            ImGui::End();
        }
        if (showMetrics) ImGui::ShowMetricsWindow(&showMetrics);

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (m_ShowDemoWindow)
            ImGui::ShowDemoWindow(&m_ShowDemoWindow);

        {
            ImGui::Begin("Inspector");
            m_DotNet.InvokeDrawInspectorFunc();
            ImGui::End();

            ImGui::Begin("Project");
            m_DotNet.InvokeDrawProjectWindowFunc();
            ImGui::End();
        }

        // 3. Show another simple window.
        if (m_ShowAnotherWindow)
        {
            ImGui::Begin("Scene", &m_ShowAnotherWindow, ImGuiWindowFlags_MenuBar);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::Button("Play"))
                {

                }

                ImGui::Spacing();

                if (ImGui::RadioButton("MSAA", m_RenderPipeline->GetEnableMSAA()))
                {
                    m_RenderPipeline->SetEnableMSAA(!m_RenderPipeline->GetEnableMSAA());
                }

                ImGui::Spacing();

                if (ImGui::RadioButton("Wireframe", m_RenderPipeline->GetIsWireframe()))
                {
                    m_RenderPipeline->SetIsWireframe(!m_RenderPipeline->GetIsWireframe());
                }

                ImGui::EndMenuBar();
            }

            auto contextSize = ImGui::GetContentRegionAvail();

            if (m_LastSceneViewWidth != contextSize.x || m_LastSceneViewHeight != contextSize.y)
            {
                m_LastSceneViewWidth = contextSize.x;
                m_LastSceneViewHeight = contextSize.y;
                ResizeRenderPipeline(m_LastSceneViewWidth, m_LastSceneViewHeight);
            }

            auto srvHandle = m_StaticDescriptorViewTable.GetGpuHandle(1);
            ImGui::Image((ImTextureID)srvHandle.ptr, contextSize);
            ImGui::End();
        }

        if (m_ShowHierarchyWindow)
        {
            ImGui::Begin("Hierarchy", &m_ShowHierarchyWindow);
            m_DotNet.InvokeDrawHierarchyWindowFunc();
            ImGui::End();
        }

        if (m_ShowDescriptorHeapWindow)
        {
            ImGui::Begin("DescriptorTable Profiler", &m_ShowDescriptorHeapWindow);
            DrawDebugDescriptorTableAllocator("CBV-SRV-UAV Allocator", GetGfxManager().GetViewDescriptorTableAllocator());
            ImGui::Spacing();
            DrawDebugDescriptorTableAllocator("Sampler Allocator", GetGfxManager().GetSamplerDescriptorTableAllocator());
            ImGui::End();
        }

        DrawConsoleWindow();

        // Rendering
        ImGui::Render();
    }

    void GameEditor::DrawDebugDescriptorTableAllocator(const std::string& name, DescriptorTableAllocator* allocator)
    {
        if (!ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            return;
        }

        int minDescriptorCount = INT_MAX;
        int maxDescriptorCount = INT_MIN;
        int maxLifetime = INT_MIN;
        int dynamicDescriptorCount = 0;

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const float height = 50.0f; // 固定高度

        const uint64_t currentFrame = GetApp().GetFrameCount();
        const UINT dynamicCapacity = allocator->GetDynamicDescriptorCapacity();
        const UINT staticCapacity = allocator->GetStaticDescriptorCount();
        const UINT capacity = dynamicCapacity + staticCapacity;
        const float columnWidth = width / static_cast<float>(capacity);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 动态区域用绿色表示，静态区域用灰色表示
        drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + dynamicCapacity * columnWidth, p.y + height), IM_COL32(0, 255, 0, 80));
        drawList->AddRectFilled(ImVec2(p.x + dynamicCapacity * columnWidth, p.y), ImVec2(p.x + width, p.y + height), IM_COL32(192, 192, 192, 80));

        for (const auto& kv : allocator->GetDynamicSegments())
        {
            minDescriptorCount = min(minDescriptorCount, static_cast<int>(kv.second.Count));
            maxDescriptorCount = max(maxDescriptorCount, static_cast<int>(kv.second.Count));
            maxLifetime = max(maxLifetime, static_cast<int>(currentFrame - kv.second.CreatedFrame));
            dynamicDescriptorCount += kv.second.Count;

            float x0 = p.x + kv.first * columnWidth; // kv.first 是 offset
            float x1 = x0 + kv.second.Count * columnWidth;

            ImU32 color = kv.second.CanRelease ? IM_COL32(0, 0, 255, 255) : IM_COL32(255, 0, 0, 255);
            drawList->AddRectFilled(ImVec2(x0, p.y), ImVec2(x1, p.y + height), color);
        }

        // 让 ImGui 知道这个区域是有内容的
        ImGui::Dummy(ImVec2(width, height));

        if (ImGui::BeginTable("DescriptorTableAllocatorInfo", 2, ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("Segment");
            ImGui::TableSetupColumn("Capacity");
            ImGui::TableHeadersRow();

            ImGui::TableNextColumn();
            const UINT segmentCount = allocator->GetDynamicSegments().size();
            ImGui::Text("Count: %d", static_cast<int>(segmentCount));
            if (segmentCount > 0)
            {
                ImGui::Text("Min Size: %d Descriptors", minDescriptorCount);
                ImGui::Text("Max Size: %d Descriptors", maxDescriptorCount);
                ImGui::Text("Max Lifetime: %d Frames", maxLifetime);
            }

            ImGui::TableNextColumn();
            float dynamicDescriptorUsage = dynamicDescriptorCount / static_cast<float>(dynamicCapacity) * 100;
            ImGui::Text("Dynamic: %d (%.2f%% Used)", static_cast<int>(dynamicCapacity), dynamicDescriptorUsage);
            ImGui::Text("Static: %d", static_cast<int>(staticCapacity));

            ImGui::EndTable();
        }

        ImGui::TreePop();
    }

    void GameEditor::DrawConsoleWindow()
    {
        if (!m_ShowConsoleWindow)
        {
            return;
        }

        if (!ImGui::Begin("Console", &m_ShowConsoleWindow, ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Clear"))
        {
            Debug::ClearLogs();
        }

        static int logTypeFilter = 0;
        static ImGuiTextFilter logMsgFilter{};
        static int selectedLog = -1;

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
        ImGui::Combo("##LogTypeFilter", &logTypeFilter, "All\0Info\0Warn\0Error\0\0");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        logMsgFilter.Draw("##LogMsgFilter", ImGui::GetContentRegionAvail().x);

        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto Scroll", &m_ConsoleWindowAutoScroll);
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

                if ((logTypeFilter == 1 && item.Type != LogType::Info)  ||
                    (logTypeFilter == 2 && item.Type != LogType::Warn)  ||
                    (logTypeFilter == 3 && item.Type != LogType::Error) ||
                    (logMsgFilter.IsActive() && !logMsgFilter.PassFilter(item.Message.c_str())))
                {
                    if (selectedLog == i)
                    {
                        selectedLog = -1;
                    }
                    continue;
                }

                float width = ImGui::GetContentRegionMax().x;
                float height = ImGui::GetTextLineHeight();
                ImVec2 cursorPos = ImGui::GetCursorPos();
                std::string label = "##LogItem" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), i == selectedLog, ImGuiSelectableFlags_None, ImVec2(width, height)))
                {
                    selectedLog = i;
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
            if (m_ConsoleWindowScrollToBottom || (m_ConsoleWindowAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
                ImGui::SetScrollHereY(1.0f);
            m_ConsoleWindowScrollToBottom = false;
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("DetailedRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None))
        {
            if (selectedLog >= 0 && selectedLog < Debug::s_Logs.size())
            {
                const LogEntry& item = Debug::s_Logs[selectedLog];

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
                selectedLog = -1;
            }
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void GameEditor::OnTick()
    {
        GetGfxManager().WaitForFameLatency();
        CalculateFrameStats();

        CommandBuffer* cmd = CommandBuffer::Get();
        EditorGUI::SetCommandBuffer(cmd);

        m_DotNet.InvokeTickFunc();
        DrawImGui();
        m_RenderPipeline->Render(cmd);

        // Render Dear ImGui graphics
        cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetGfxManager().GetBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
        cmd->GetList()->OMSetRenderTargets(1, &GetGfxManager().GetBackBufferView(), false, nullptr);

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd->GetList());

        cmd->GetList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetGfxManager().GetBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

        cmd->ExecuteAndRelease();
        EditorGUI::SetCommandBuffer(nullptr);
        GetGfxManager().Present();
    }

    void GameEditor::OnResized()
    {
        auto [width, height] = GetApp().GetClientWidthAndHeight();
        GetGfxManager().ResizeBackBuffer(width, height);
    }

    void GameEditor::ResizeRenderPipeline(int width, int height)
    {
        m_RenderPipeline->Resize(width, height);

        auto device = GetGfxManager().GetDevice();
        auto srvHandle = m_StaticDescriptorViewTable.GetCpuHandle(1);
        device->CreateShaderResourceView(m_RenderPipeline->GetResolvedColorTarget(), nullptr, srvHandle);
    }

    void GameEditor::OnDisplayScaleChanged()
    {
        DEBUG_LOG_INFO("DPI Changed: %f", GetApp().GetDisplayScale());

        auto& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF(m_FontPath, m_FontSize * GetApp().GetDisplayScale(),
            nullptr, io.Fonts->GetGlyphRangesChineseFull());
        io.Fonts->AddFontDefault();
        io.Fonts->Build();

        ImGui_ImplDX12_InvalidateDeviceObjects();
    }

    void GameEditor::OnPaint()
    {
        OnTick();
    }

    void GameEditor::CalculateFrameStats()
    {
        // Code computes the average frames per second, and also the 
        // average time it takes to render one frame.  These stats 
        // are appended to the window caption bar.

        static int frameCnt = 0;
        static float timeElapsed = 0.0f;

        frameCnt++;

        // Compute averages over one second period.
        if ((GetApp().GetElapsedTime() - timeElapsed) >= 1.0f)
        {
            float fps = (float)frameCnt; // fps = frameCnt / 1
            float mspf = 1000.0f / fps;

            std::wstring fpsStr = std::to_wstring(fps);
            std::wstring mspfStr = std::to_wstring(mspf);

            GetApp().SetTitle(std::wstring(L"DX12 Demo") +
                L"    fps: " + fpsStr +
                L"   mspf: " + mspfStr);

            // Reset for next average.
            frameCnt = 0;
            timeElapsed += 1.0f;
        }
    }
}
