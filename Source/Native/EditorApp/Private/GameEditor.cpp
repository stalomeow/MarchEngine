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
#include "Transform.h"
#include "GfxMesh.h"
#include <DirectXMath.h>
#include <stdint.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <tuple>
#include <ImGuizmo.h>
#include "ConsoleWindow.h"

using namespace DirectX;

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
        desc.ViewTableStaticDescriptorCount = 1;
        desc.ViewTableDynamicDescriptorCapacity = 4096;
        desc.SamplerTableStaticDescriptorCount = 0;
        desc.SamplerTableDynamicDescriptorCapacity = 1024;
        InitGfxDevice(desc);

        Display::CreateMainDisplay(GetGfxDevice(), 10, 10); // temp
        m_RenderPipeline = std::make_unique<RenderPipeline>();
        m_StaticDescriptorViewTable = GetGfxDevice()->GetStaticDescriptorTable(GfxDescriptorTableType::CbvSrvUav);

        InitImGui();

        m_DotNet->Invoke(ManagedMethod::Application_OnStart);
        m_DotNet->Invoke(ManagedMethod::EditorApplication_OnStart);
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

        const ImVec4 dockingEmptyBgColor = ColorFromBytes(18, 18, 18);
        const ImVec4 bgColor = ColorFromBytes(25, 25, 26);
        const ImVec4 menuColor = ColorFromBytes(35, 35, 36);
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
        colors[ImGuiCol_TitleBg] = dockingEmptyBgColor;
        colors[ImGuiCol_TitleBgActive] = dockingEmptyBgColor;
        colors[ImGuiCol_TitleBgCollapsed] = dockingEmptyBgColor;
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
        colors[ImGuiCol_SeparatorHovered] = panelHoverColor;
        colors[ImGuiCol_SeparatorActive] = panelActiveColor;
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
        colors[ImGuiCol_DockingEmptyBg] = dockingEmptyBgColor;
        colors[ImGuiCol_Tab] = bgColor;
        colors[ImGuiCol_TabActive] = panelColor;
        colors[ImGuiCol_TabUnfocused] = bgColor;
        colors[ImGuiCol_TabUnfocusedActive] = panelColor;
        colors[ImGuiCol_TabHovered] = panelColor;
        colors[ImGuiCol_TabDimmedSelected] = panelColor;
        colors[ImGuiCol_TabDimmedSelectedOverline] = panelColor;
        colors[ImGuiCol_TabSelectedOverline] = panelActiveColor;

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.TabRounding = 0.0f;
        style.TabBarBorderSize = 2.0f;
        style.TabBarOverlineSize = 2.0f;
    }

    void GameEditor::InitImGui()
    {
        m_ImGuiIniFilename = GetApp().GetDataPath() + "/ProjectSettings/imgui.ini";

        // Setup Dear ImGui context
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
        io.IniFilename = m_ImGuiIniFilename.c_str();

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

        auto device = GetGfxDevice()->GetD3D12Device();
        ImGui_ImplDX12_Init(device, GetGfxDevice()->GetMaxFrameLatency(),
            GetGfxDevice()->GetBackBufferFormat(), m_StaticDescriptorViewTable.GetD3D12DescriptorHeap(),
            m_StaticDescriptorViewTable.GetCpuHandle(0), m_StaticDescriptorViewTable.GetGpuHandle(0));
    }

    void GameEditor::OnQuit()
    {
        m_DotNet->Invoke(ManagedMethod::EditorApplication_OnQuit);
        m_DotNet->Invoke(ManagedMethod::Application_OnQuit);

        GetGfxDevice()->WaitForIdle();
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        //Display::DestroyMainDisplay();
        //DestroyGfxDevice();
        //GfxUtility::ReportLiveObjects();
    }

    void GameEditor::DrawBaseImGui()
    {
        if (EditorGUI::BeginMainMenuBar())
        {
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

            EditorGUI::EndMainMenuBar();
        }

        if (EditorGUI::BeginMainViewportSideBar("##SingleLineToolbar", ImGuiDir_Up, ImGui::GetFrameHeight()))
        {
            EditorGUI::Button("Play");
            ImGui::SameLine();
            EditorGUI::Button("Pause");
            ImGui::SameLine();
            EditorGUI::Button("Capture");
        }
        EditorGUI::EndMainViewportSideBar();

        ConsoleWindow::DrawMainViewportSideBarConsole(m_DotNet.get());

        ImGui::DockSpaceOverViewport();
    }

    void GameEditor::OnTick()
    {
        GfxDevice* device = GetGfxDevice();

        device->BeginFrame();
        CalculateFrameStats();

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        DrawBaseImGui();
        m_DotNet->Invoke(ManagedMethod::Application_OnTick);
        m_DotNet->Invoke(ManagedMethod::EditorApplication_OnTick);

        // Render Dear ImGui graphics
        ImGui::Render();
        device->SetBackBufferAsRenderTarget();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), device->GetGraphicsCommandList()->GetD3D12CommandList());

        device->EndFrame();
    }

    void GameEditor::OnResized()
    {
        auto [width, height] = GetApp().GetClientWidthAndHeight();
        GetGfxDevice()->ResizeBackBuffer(width, height);
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
