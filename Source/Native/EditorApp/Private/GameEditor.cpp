#include "GameEditor.h"
#include "WinApplication.h"
#include "GfxDevice.h"
#include "EditorGUI.h"
#include "GfxCommandList.h"
#include "GfxBuffer.h"
#include "Debug.h"
#include "StringUtility.h"
#include "RenderDoc.h"
#include "DotNetRuntime.h"
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
#include "IconsFontAwesome6.h"
#include "IconsFontAwesome6Brands.h"
#include <imgui_freetype.h>
#include "Shader.h"

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
            RenderDoc::Load(); // 越早越好
        }

        DotNet::InitRuntime(); // 越早越好，mixed debugger 需要 runtime 加载完后才能工作

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
        m_ImGuiRenderGraph = std::make_unique<RenderGraph>(false);
        m_StaticDescriptorViewTable = GetGfxDevice()->GetStaticDescriptorTable(GfxDescriptorTableType::CbvSrvUav);

        InitImGui();
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

        const ImVec4 panelColor = ColorFromBytes(55, 55, 59);
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
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.ConfigDockingAlwaysTabBar = true;

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(GetApp().GetHWND());

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        SetStyles();

        //ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
        //ImGui::GetStyle().FrameBorderSize = 1.0f;
        //ImGui::GetStyle().FrameRounding = 2.0f;
        //ImGui::GetStyle().TabRounding = 2.0f;

        ReloadFonts();

        auto device = GetGfxDevice()->GetD3D12Device();
        ImGui_ImplDX12_Init(device, GetGfxDevice()->GetMaxFrameLatency(),
            m_ImGuiRtvFormat, m_StaticDescriptorViewTable.GetD3D12DescriptorHeap(),
            m_StaticDescriptorViewTable.GetCpuHandle(0), m_StaticDescriptorViewTable.GetGpuHandle(0));

        ImGuizmo::GetStyle().RotationLineThickness = 3.0f;
        ImGuizmo::GetStyle().RotationOuterLineThickness = 2.0f;
    }

    void GameEditor::OnQuit()
    {
        DotNet::DestroyRuntime();

        GetGfxDevice()->WaitForIdle();
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        m_ImGuiRenderGraph.reset();
        DestroyAllPipelineStates();
        Display::DestroyMainDisplay();
        DestroyGfxDevice();
        GfxUtility::ReportLiveObjects();
    }

    void GameEditor::DrawBaseImGui()
    {
        if (EditorGUI::BeginMainMenuBar())
        {
            if (ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_C, ImGuiInputFlags_RouteAlways))
            {
                RenderDoc::CaptureSingleFrame();
            }

            if (ImGui::BeginMenu("RenderDoc"))
            {
                if (ImGui::MenuItem("Capture", "Alt+C", nullptr, RenderDoc::IsLoaded()))
                {
                    RenderDoc::CaptureSingleFrame();
                }

                ImGui::SeparatorText("Information");

                if (ImGui::BeginMenu("Library"))
                {
                    ImGui::TextUnformatted(RenderDoc::GetLibraryPath().c_str());
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("API Version"))
                {
                    auto [major, minor, patch] = RenderDoc::GetVersion();
                    ImGui::Text("%d.%d.%d", major, minor, patch);
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Num Captures"))
                {
                    ImGui::Text("%d", RenderDoc::GetNumCaptures());
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            EditorGUI::EndMainMenuBar();
        }

        if (EditorGUI::BeginMainViewportSideBar("##SingleLineToolbar", ImGuiDir_Up, ImGui::GetFrameHeight()))
        {
            float width1 = EditorGUI::CalcButtonWidth(ICON_FA_PLAY) * 1.8f;
            float width2 = EditorGUI::CalcButtonWidth(ICON_FA_PAUSE) * 1.8f;
            float width3 = EditorGUI::CalcButtonWidth(ICON_FA_FORWARD_STEP) * 1.8f;
            float width4 = EditorGUI::CalcButtonWidth(ICON_FA_CAMERA) * 1.8f;
            float buttonWidth = width1 + width2 + width3 + width4;
            float contentTotalWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (contentTotalWidth - buttonWidth) * 0.5f);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

            ImGui::Button(ICON_FA_PLAY, ImVec2(width1, ImGui::GetFrameHeight()));
            ImGui::SameLine();
            ImGui::Button(ICON_FA_PAUSE, ImVec2(width2, ImGui::GetFrameHeight()));
            ImGui::SameLine();
            ImGui::Button(ICON_FA_FORWARD_STEP, ImVec2(width3, ImGui::GetFrameHeight()));
            ImGui::SameLine();

            ImGui::BeginDisabled(!RenderDoc::IsLoaded());
            if (ImGui::Button(ICON_FA_CAMERA, ImVec2(width4, ImGui::GetFrameHeight())))
            {
                RenderDoc::CaptureSingleFrame();
            }
            ImGui::EndDisabled();

            ImGui::PopStyleVar();
        }
        EditorGUI::EndMainViewportSideBar();

        ConsoleWindow::DrawMainViewportSideBarConsole();

        ImGui::DockSpaceOverViewport();
    }

    void GameEditor::OnTick(bool willQuit)
    {
        GfxDevice* device = GetGfxDevice();

        device->BeginFrame();
        CalculateFrameStats();

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (!m_IsScriptInitialized)
        {
            // 初始化代码可能也会用到 GfxDevice，所以放在 tick 里
            DotNet::RuntimeInvoke(ManagedMethod::Application_OnStart);
            DotNet::RuntimeInvoke(ManagedMethod::EditorApplication_OnStart);

            m_RenderPipeline = std::make_unique<RenderPipeline>();
            m_IsScriptInitialized = true;
        }

        // Update
        DrawBaseImGui();
        DotNet::RuntimeInvoke(ManagedMethod::Application_OnTick);
        DotNet::RuntimeInvoke(ManagedMethod::EditorApplication_OnTick);

        if (willQuit && m_IsScriptInitialized)
        {
            m_IsScriptInitialized = false;

            m_BlitImGuiShader.reset();
            m_BlitImGuiMaterial.reset();

            m_RenderPipeline->ReleaseAssets();

            // 退出代码可能也会用到 GfxDevice，所以放在 tick 里
            DotNet::RuntimeInvoke(ManagedMethod::EditorApplication_OnQuit);
            DotNet::RuntimeInvoke(ManagedMethod::Application_OnQuit);

            m_RenderPipeline.reset();
        }

        if (!willQuit)
        {
            // Render Dear ImGui graphics
            int32_t tempRenderTargetId = Shader::GetNameId("_TempImGuiRenderTarget");
            int32_t backBufferId = Shader::GetNameId("_BackBuffer");

            if (!m_BlitImGuiShader)
            {
                m_BlitImGuiShader.reset("Engine/Shaders/BlitImGui.shader");
                m_BlitImGuiMaterial = std::make_unique<Material>();
                m_BlitImGuiMaterial->SetShader(m_BlitImGuiShader.get());
            }

            DrawImGuiRenderGraph(device, tempRenderTargetId);
            BlitImGuiToBackBuffer(device, tempRenderTargetId, backBufferId);
            m_ImGuiRenderGraph->CompileAndExecute();
        }

        device->EndFrame();
    }

    void GameEditor::DrawImGuiRenderGraph(GfxDevice* device, int32_t renderTargetId)
    {
        auto builder = m_ImGuiRenderGraph->AddPass("DrawImGui");

        GfxRenderTextureDesc desc = device->GetBackBuffer()->GetDesc();
        desc.Format = m_ImGuiRtvFormat;

        builder.CreateTransientTexture(renderTargetId, desc);
        builder.SetColorTarget(renderTargetId, false);
        builder.ClearRenderTargets(ClearFlags::Color);

        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            ImGui::Render();
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context.GetD3D12GraphicsCommandList());
        });
    }

    void GameEditor::BlitImGuiToBackBuffer(GfxDevice* device, int32_t srcTextureId, int32_t backBufferId)
    {
        auto builder = m_ImGuiRenderGraph->AddPass("BlitImGuiToBackBuffer");

        builder.ImportTexture(backBufferId, device->GetBackBuffer());
        builder.SetColorTarget(backBufferId, false);

        TextureHandle srcTexture = builder.ReadTexture(srcTextureId, ReadFlags::PixelShader);

        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.SetTexture("_SrcTex", srcTexture.Get());
            context.DrawMesh(GetFullScreenTriangleMesh(), m_BlitImGuiMaterial.get());
        });
    }

    GfxMesh* GameEditor::GetFullScreenTriangleMesh()
    {
        return m_RenderPipeline->m_FullScreenTriangleMesh.get();
    }

    void GameEditor::OnResized()
    {
        auto [width, height] = GetApp().GetClientWidthAndHeight();
        GetGfxDevice()->ResizeBackBuffer(width, height);
    }

    std::string GameEditor::GetFontPath(std::string fontName) const
    {
        std::string basePath = PathHelper::GetWorkingDirectoryUtf8();
        return basePath + "\\Resources\\Fonts\\" + fontName;
    }

    std::string GameEditor::GetFontAwesomePath(std::string fontName) const
    {
        std::string basePath = PathHelper::GetWorkingDirectoryUtf8();
        return basePath + "\\Resources\\FontAwesome\\" + fontName;
    }

    void GameEditor::ReloadFonts()
    {
        const float dpiScale = GetApp().GetDisplayScale();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        // 英文字体
        ImFontConfig latinConfig{};
        latinConfig.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF(GetFontPath("Inter-Regular.otf").c_str(), m_FontSizeLatin * dpiScale,
            &latinConfig, io.Fonts->GetGlyphRangesDefault());

        // 中文字体
        ImFontConfig cjkConfig{};
        cjkConfig.MergeMode = true;
        cjkConfig.PixelSnapH = true;
        cjkConfig.RasterizerDensity = 1.5f; // 稍微放大一点，更清晰
        io.Fonts->AddFontFromFileTTF(GetFontPath("NotoSansSC-Regular.ttf").c_str(), m_FontSizeCJK * dpiScale,
            &cjkConfig, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

        // Font Awesome 图标字体
        float iconFontSizePixels = m_FontSizeIcon * dpiScale;
        static const ImWchar faIconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
        static const ImWchar fabIconsRanges[] = { ICON_MIN_FAB, ICON_MAX_16_FAB, 0 };

        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphMinAdvanceX = iconFontSizePixels; // 让所有图标等宽
        iconConfig.GlyphMaxAdvanceX = iconFontSizePixels; // 让所有图标等宽

        // use FONT_ICON_FILE_NAME_FAR if you want regular instead of solid
        io.Fonts->AddFontFromFileTTF(GetFontAwesomePath(FONT_ICON_FILE_NAME_FAS).c_str(), iconFontSizePixels,
            &iconConfig, faIconsRanges);
        io.Fonts->AddFontFromFileTTF(GetFontAwesomePath(FONT_ICON_FILE_NAME_FAB).c_str(), iconFontSizePixels,
            &iconConfig, fabIconsRanges);

        io.Fonts->Build();
    }

    void GameEditor::OnDisplayScaleChanged()
    {
        DEBUG_LOG_INFO("DPI Changed: %f", GetApp().GetDisplayScale());

        ReloadFonts();
        ImGui_ImplDX12_InvalidateDeviceObjects();
    }

    void GameEditor::OnPaint()
    {
        OnTick(false);
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
