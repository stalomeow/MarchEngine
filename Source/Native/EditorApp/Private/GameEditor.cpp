#include "GameEditor.h"
#include "WinApplication.h"
#include "GfxDevice.h"
#include "EditorGUI.h"
#include "GfxCommandList.h"
#include "GfxBuffer.h"
#include "Debug.h"
#include "StringUtility.h"
#include "PathHelper.h"
#include "EditorGUI.h"
#include "Camera.h"
#include "Display.h"
#include "GfxTexture.h"
#include <DirectXMath.h>
#include <stdint.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <tuple>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace march
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

    void GameEditor::OnStart(const std::vector<std::string>& args)
    {
        if (std::count(args.begin(), args.end(), "-load-renderdoc") > 0)
        {
            m_RenderDoc.Load(); // 越早越好
        }

        m_DotNet = CreateDotNetRuntime(); // 越早越好，mixed debugger 需要 runtime 加载完后才能工作

        auto [width, height] = GetApp().GetClientWidthAndHeight();

        GfxDeviceDesc desc{};

        if (std::count(args.begin(), args.end(), "-enable-d3d12-debug-layer") > 0)
        {
            desc.EnableDebugLayer = true;
        }

        desc.WindowHandle = GetApp().GetHWND();
        desc.WindowWidth = width;
        desc.WindowHeight = height;
        desc.ViewTableStaticDescriptorCount = 2;
        desc.ViewTableDynamicDescriptorCapacity = 4096;
        desc.SamplerTableStaticDescriptorCount = 0;
        desc.SamplerTableDynamicDescriptorCapacity = 1024;
        InitGfxDevice(desc);

        Display::CreateMainDisplay(GetGfxDevice(), 10, 10); // temp
        Display::CreateEditorSceneDisplay(GetGfxDevice(), static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        m_RenderPipeline = std::make_unique<RenderPipeline>();
        m_StaticDescriptorViewTable = GetGfxDevice()->GetStaticDescriptorTable(GfxDescriptorTableType::CbvSrvUav);
        //m_StaticDescriptorViewTable.Copy(1, m_RenderPipeline->GetColorShaderResourceView());

        InitImGui();

        m_DotNet->Invoke(ManagedMethod::Initialize);
        m_DotNet->Invoke(ManagedMethod::EditorInitialize);
    }

    static void SetStyles()
    {
        // https://github.com/ocornut/imgui/issues/707

        constexpr auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b)
            {
                return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
            };

        auto& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        const ImVec4 bgColor = ColorFromBytes(20, 20, 21);
        const ImVec4 menuColor = ColorFromBytes(34, 34, 35);
        const ImVec4 lightBgColor = ColorFromBytes(90, 90, 92);
        const ImVec4 veryLightBgColor = ColorFromBytes(110, 110, 115);

        const ImVec4 panelColor = ColorFromBytes(50, 50, 54);
        const ImVec4 panelHoverColor = ColorFromBytes(35, 80, 142);
        const ImVec4 panelActiveColor = ColorFromBytes(0, 95, 170);

        const ImVec4 textColor = ColorFromBytes(230, 230, 230);
        const ImVec4 textHighlightColor = ColorFromBytes(255, 255, 255);
        const ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
        const ImVec4 borderColor = ColorFromBytes(58, 58, 58);

        colors[ImGuiCol_Text] = textColor;
        colors[ImGuiCol_TextDisabled] = textDisabledColor;
        colors[ImGuiCol_TextSelectedBg] = panelActiveColor;
        colors[ImGuiCol_WindowBg] = bgColor;
        colors[ImGuiCol_ChildBg] = bgColor;
        colors[ImGuiCol_PopupBg] = bgColor;
        colors[ImGuiCol_Border] = borderColor;
        colors[ImGuiCol_BorderShadow] = borderColor;
        colors[ImGuiCol_FrameBg] = panelColor;
        colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
        colors[ImGuiCol_FrameBgActive] = panelActiveColor;
        colors[ImGuiCol_TitleBg] = bgColor;
        colors[ImGuiCol_TitleBgActive] = bgColor;
        colors[ImGuiCol_TitleBgCollapsed] = bgColor;
        colors[ImGuiCol_MenuBarBg] = menuColor;
        colors[ImGuiCol_ScrollbarBg] = panelColor;
        colors[ImGuiCol_ScrollbarGrab] = lightBgColor;
        colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
        colors[ImGuiCol_ScrollbarGrabActive] = veryLightBgColor;
        colors[ImGuiCol_CheckMark] = textColor;
        colors[ImGuiCol_SliderGrab] = textColor;
        colors[ImGuiCol_SliderGrabActive] = textHighlightColor;
        colors[ImGuiCol_Button] = panelColor;
        colors[ImGuiCol_ButtonHovered] = panelHoverColor;
        colors[ImGuiCol_ButtonActive] = panelActiveColor;
        colors[ImGuiCol_Header] = panelColor;
        colors[ImGuiCol_HeaderHovered] = panelHoverColor;
        colors[ImGuiCol_HeaderActive] = panelActiveColor;
        colors[ImGuiCol_Separator] = borderColor;
        colors[ImGuiCol_SeparatorHovered] = borderColor;
        colors[ImGuiCol_SeparatorActive] = borderColor;
        colors[ImGuiCol_ResizeGrip] = bgColor;
        colors[ImGuiCol_ResizeGripHovered] = panelHoverColor;
        colors[ImGuiCol_ResizeGripActive] = panelActiveColor;
        colors[ImGuiCol_PlotLines] = panelActiveColor;
        colors[ImGuiCol_PlotLinesHovered] = panelHoverColor;
        colors[ImGuiCol_PlotHistogram] = panelActiveColor;
        colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
        colors[ImGuiCol_ModalWindowDimBg] = bgColor;
        colors[ImGuiCol_DragDropTarget] = panelActiveColor;
        colors[ImGuiCol_NavHighlight] = bgColor;
        colors[ImGuiCol_DockingPreview] = panelActiveColor;
        colors[ImGuiCol_Tab] = bgColor;
        colors[ImGuiCol_TabActive] = panelActiveColor;
        colors[ImGuiCol_TabUnfocused] = bgColor;
        colors[ImGuiCol_TabUnfocusedActive] = panelActiveColor;
        colors[ImGuiCol_TabHovered] = panelHoverColor;

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.TabRounding = 0.0f;
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
        io.Fonts->AddFontFromFileTTF(GetFontPath().c_str(), m_FontSize * GetApp().GetDisplayScale(),
            nullptr, io.Fonts->GetGlyphRangesChineseFull());
        io.Fonts->AddFontDefault();
        io.Fonts->Build();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        SetStyles();

        //ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
        //ImGui::GetStyle().FrameBorderSize = 1.0f;
        //ImGui::GetStyle().FrameRounding = 2.0f;
        //ImGui::GetStyle().TabRounding = 2.0f;
        ImGui::GetStyle().TabBarOverlineSize = 0.0f;

        auto device = GetGfxDevice()->GetD3D12Device();
        ImGui_ImplDX12_Init(device, GetGfxDevice()->GetMaxFrameLatency(),
            GetGfxDevice()->GetBackBufferFormat(), m_StaticDescriptorViewTable.GetD3D12DescriptorHeap(),
            m_StaticDescriptorViewTable.GetCpuHandle(0), m_StaticDescriptorViewTable.GetGpuHandle(0));
    }

    void GameEditor::OnQuit()
    {
        GetGfxDevice()->WaitForIdle();
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void GameEditor::DrawImGui(Camera* sceneCamera)
    {
        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        static bool showStyleEditor = false;
        static bool showMetrics = false;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
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
        ImGui::PopStyleVar();

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
            m_DotNet->Invoke(ManagedMethod::DrawInspectorWindow);
            ImGui::End();

            ImGui::Begin("Project");
            m_DotNet->Invoke(ManagedMethod::DrawProjectWindow);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (m_ShowAnotherWindow)
        {
            ImGui::Begin("Scene", &m_ShowAnotherWindow, ImGuiWindowFlags_MenuBar);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::RadioButton("Play", false))
                {
                }

                if (ImGui::RadioButton("MSAA", sceneCamera->GetEnableMSAA()))
                {
                    // 不等 gpu idle 也行，但是切换 msaa 会有一帧的延迟，画面会闪一下
                    GetGfxDevice()->WaitForIdle();
                    sceneCamera->SetEnableMSAA(!sceneCamera->GetEnableMSAA());
                    SetSceneViewSrv(sceneCamera);
                }

                ImGui::Spacing();

                if (ImGui::RadioButton("Wireframe", sceneCamera->GetEnableWireframe()))
                {
                    CameraInternalUtility::SetEnableWireframe(sceneCamera, !sceneCamera->GetEnableWireframe());
                }

                ImGui::EndMenuBar();
            }

            auto contextSize = ImGui::GetContentRegionAvail();

            if (m_LastSceneViewWidth != contextSize.x || m_LastSceneViewHeight != contextSize.y)
            {
                m_LastSceneViewWidth = contextSize.x;
                m_LastSceneViewHeight = contextSize.y;
                ResizeRenderPipeline(sceneCamera, m_LastSceneViewWidth, m_LastSceneViewHeight);
            }

            auto srvHandle = m_StaticDescriptorViewTable.GetGpuHandle(1);
            ImGui::Image((ImTextureID)srvHandle.ptr, contextSize);
            ImGui::End();
        }

        if (m_ShowHierarchyWindow)
        {
            ImGui::Begin("Hierarchy", &m_ShowHierarchyWindow);
            m_DotNet->Invoke(ManagedMethod::DrawHierarchyWindow);
            ImGui::End();
        }

        if (m_ShowDescriptorHeapWindow)
        {
            ImGui::Begin("DescriptorTable Profiler", &m_ShowDescriptorHeapWindow);
            DrawDebugDescriptorTableAllocator("CBV-SRV-UAV Allocator", GetGfxDevice()->GetViewDescriptorTableAllocator());
            ImGui::Spacing();
            DrawDebugDescriptorTableAllocator("Sampler Allocator", GetGfxDevice()->GetSamplerDescriptorTableAllocator());
            ImGui::End();
        }

        DrawConsoleWindow();

        // Rendering
        ImGui::Render();
    }

    void GameEditor::DrawDebugDescriptorTableAllocator(const std::string& name, GfxDescriptorTableAllocator* allocator)
    {
        if (!ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            return;
        }

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const float height = 50.0f; // 固定高度

        const uint64_t currentFrame = GetApp().GetFrameCount();
        const UINT dynamicCapacity = allocator->GetDynamicDescriptorCapacity();
        const UINT staticCount = allocator->GetStaticDescriptorCount();
        const UINT capacity = dynamicCapacity + staticCount;
        const float columnWidth = width / static_cast<float>(capacity);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 动态区域用绿色表示，静态区域用灰色表示
        drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + dynamicCapacity * columnWidth, p.y + height), IM_COL32(0, 255, 0, 80));
        drawList->AddRectFilled(ImVec2(p.x + dynamicCapacity * columnWidth, p.y), ImVec2(p.x + width, p.y + height), IM_COL32(192, 192, 192, 80));

        uint32_t front = allocator->GetDynamicFront();
        uint32_t rear = allocator->GetDynamicRear();

        if (front < rear)
        {
            float x0 = p.x + front * columnWidth;
            float x1 = p.x + rear * columnWidth;

            ImU32 color = IM_COL32(255, 0, 0, 255);
            drawList->AddRectFilled(ImVec2(x0, p.y), ImVec2(x1, p.y + height), color);
        }
        else if (front > rear)
        {
            float x0 = p.x + rear * columnWidth;
            float x1 = p.x + front * columnWidth;

            ImU32 color = IM_COL32(255, 0, 0, 255);
            drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(x0, p.y + height), color);
            drawList->AddRectFilled(ImVec2(x1, p.y), ImVec2(p.x + width, p.y + height), color);
        }

        int dynamicDescriptorCount = static_cast<int>((rear + dynamicCapacity - front) % dynamicCapacity);

        // 让 ImGui 知道这个区域是有内容的
        ImGui::Dummy(ImVec2(width, height));

        float dynamicDescriptorUsage = dynamicDescriptorCount / static_cast<float>(dynamicCapacity) * 100;
        std::string label1 = StringUtility::Format("Dynamic Capacity: %d / %d (%.2f%% Used)", dynamicDescriptorCount, static_cast<int>(dynamicCapacity), dynamicDescriptorUsage);
        std::string label2 = StringUtility::Format("Static Count: %d", staticCount);

        float startX = ImGui::GetCursorPosX();
        ImGui::TextUnformatted(label1.c_str());
        ImGui::SameLine();
        ImVec2 label2Size = ImGui::CalcTextSize(label2.c_str());
        ImGui::SetCursorPosX(startX + width - label2Size.x);
        ImGui::TextUnformatted(label2.c_str());

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
        GetGfxDevice()->BeginFrame();
        CalculateFrameStats();

        m_DotNet->Invoke(ManagedMethod::Tick);

        Camera* sceneCamera = nullptr;
        for (const auto c : Camera::GetAllCameras())
        {
            if (c->GetIsEditorSceneCamera())
            {
                sceneCamera = c;
                break;
            }
        }

        if (sceneCamera != nullptr)
        {
            DrawImGui(sceneCamera);
            m_RenderPipeline->Render(sceneCamera);

            // Render Dear ImGui graphics
            GetGfxDevice()->SetBackBufferAsRenderTarget();
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GetGfxDevice()->GetGraphicsCommandList()->GetD3D12CommandList());
        }

        GetGfxDevice()->EndFrame();
    }

    void GameEditor::OnResized()
    {
        auto [width, height] = GetApp().GetClientWidthAndHeight();
        GetGfxDevice()->ResizeBackBuffer(width, height);
    }

    void GameEditor::ResizeRenderPipeline(Camera* sceneCamera, int width, int height)
    {
        GetGfxDevice()->WaitForIdle();
        sceneCamera->GetTargetDisplay()->Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        SetSceneViewSrv(sceneCamera);
    }

    void GameEditor::SetSceneViewSrv(Camera* sceneCamera)
    {
        Display* display = sceneCamera->GetTargetDisplay();

        if (display->GetEnableMSAA())
        {
            m_StaticDescriptorViewTable.Copy(1, display->GetResolvedColorBuffer()->GetSrvCpuDescriptorHandle());
        }
        else
        {
            m_StaticDescriptorViewTable.Copy(1, display->GetColorBuffer()->GetSrvCpuDescriptorHandle());
        }
    }

    std::string GameEditor::GetFontPath()
    {
        std::string basePath = PathHelper::GetWorkingDirectoryUtf8();
        return basePath + "\\Resources\\Fonts\\Inter-Regular.otf";
    }

    void GameEditor::OnDisplayScaleChanged()
    {
        DEBUG_LOG_INFO("DPI Changed: %f", GetApp().GetDisplayScale());

        auto& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF(GetFontPath().c_str(), m_FontSize * GetApp().GetDisplayScale(),
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

            GetApp().SetTitle(std::wstring(L"March Engine <DX12>") +
                L"    fps: " + fpsStr +
                L"   mspf: " + mspfStr);

            // Reset for next average.
            frameCnt = 0;
            timeElapsed += 1.0f;
        }
    }
}
