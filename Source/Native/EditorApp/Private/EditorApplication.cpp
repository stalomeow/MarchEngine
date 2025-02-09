#include "pch.h"
#include "Editor/EditorApplication.h"
#include "Editor/EditorGUI.h"
#include "Editor/ConsoleWindow.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/Rendering/RenderPipeline.h"
#include "Engine/Rendering/Gizmos.h"
#include "Engine/Rendering/Display.h"
#include "Engine/ImGui/IconsFontAwesome6.h"
#include "Engine/ImGui/IconsFontAwesome6Brands.h"
#include "Engine/ImGui/ImGuiBackend.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Misc/PathUtils.h"
#include "Engine/Scripting/DotNetRuntime.h"
#include "Engine/Profiling/FrameDebugger.h"
#include "Engine/Debug.h"
#include <directx/d3dx12.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <ImGuizmo.h>
#include <algorithm>

using namespace DirectX;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace march
{
    EditorApplication::EditorApplication()
        : m_SwapChain(nullptr)
        , m_RenderPipeline(nullptr)
        , m_DataPath{}
        , m_ImGuiIniFilename{}
    {
    }

    EditorApplication::~EditorApplication() {}

    // Win32 message handler
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    bool EditorApplication::OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& outResult)
    {
        if (ImGui_ImplWin32_WndProcHandler(GetWindowHandle(), msg, wParam, lParam))
        {
            outResult = true;
            return true;
        }
        return false;
    }

    void EditorApplication::OnStart(const std::vector<std::string>& args)
    {
        auto it = std::find(args.begin(), args.end(), "-project-path");

        if (it != args.end() && (it = std::next(it)) != args.end())
        {
            m_DataPath = *it;
        }
        else
        {
            throw std::runtime_error("Project path not found in command line arguments.");
        }

        if (std::count(args.begin(), args.end(), "-load-renderdoc") > 0)
        {
            FrameDebugger::LoadPlugin(FrameDebuggerPlugin::RenderDoc); // 越早越好
        }

        if (std::count(args.begin(), args.end(), "-load-pix") > 0)
        {
            FrameDebugger::LoadPlugin(FrameDebuggerPlugin::PIX); // 越早越好
        }

        DotNet::InitRuntime(); // 越早越好，mixed debugger 需要 runtime 加载完后才能工作

        GfxDeviceDesc desc{};

        if (std::count(args.begin(), args.end(), "-enable-d3d12-debug-layer") > 0)
        {
            desc.EnableDebugLayer = true;
        }

        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = 1024;
        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = 64;
        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = 64;
        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = 64;
        desc.OnlineViewDescriptorHeapSize = 10000;
        desc.OnlineSamplerDescriptorHeapSize = 2048;
        GfxDevice* device = InitGfxDevice(desc);

        m_SwapChain = std::make_unique<GfxSwapChain>(device, GetWindowHandle(), GetClientWidth(), GetClientHeight());
        Display::CreateMainDisplay(10, 10); // dummy

        InitImGui();

        // 初始化代码可能也会用到 GfxDevice
        DotNet::RuntimeInvoke(ManagedMethod::Application_Initialize);
        DotNet::RuntimeInvoke(ManagedMethod::EditorApplication_Initialize);

        // 需要用到 managed method
        m_RenderPipeline = std::make_unique<RenderPipeline>();
        Gizmos::InitResources();
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

    void EditorApplication::InitImGui()
    {
        m_ImGuiIniFilename = GetDataPath() + "/ProjectSettings/imgui.ini";

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
        ImGui_ImplWin32_Init(GetWindowHandle());

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        SetStyles();

        //ImGui::GetStyle().WindowMenuButtonPosition = ImGuiDir_None;
        //ImGui::GetStyle().FrameBorderSize = 1.0f;
        //ImGui::GetStyle().FrameRounding = 2.0f;
        //ImGui::GetStyle().TabRounding = 2.0f;

        ReloadFonts();

        ImGui_ImplDX12_Init(GetGfxDevice(), "Engine/Shaders/DearImGui.shader");

        ImGuizmo::GetStyle().RotationLineThickness = 3.0f;
        ImGuizmo::GetStyle().RotationOuterLineThickness = 2.0f;
    }

    void EditorApplication::OnQuit()
    {
        // 退出代码可能也会用到 GfxDevice
        DotNet::RuntimeInvoke(ManagedMethod::Application_Quit);

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        m_RenderPipeline.reset();
        m_SwapChain.reset();

        Gizmos::ReleaseResources();
        Display::DestroyMainDisplay();
        GfxTexture::ClearSamplerCache();
        ShaderUtils::ClearRootSignatureCache();

        DotNet::RuntimeInvoke(ManagedMethod::Application_FullGC);
        DotNet::DestroyRuntime();

        DestroyGfxDevice();
        GfxUtils::ReportLiveObjects();
    }

    void EditorApplication::DrawBaseImGui()
    {
        // Main Menu Bar 占位
        if (EditorGUI::BeginMainMenuBar())
        {
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

            if (FrameDebugger::GetLoadedPlugin() && ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_C, ImGuiInputFlags_RouteAlways))
            {
                FrameDebugger::Capture();
            }

            ImGui::BeginDisabled(!FrameDebugger::GetLoadedPlugin());
            bool capture = ImGui::Button(ICON_FA_CAMERA, ImVec2(width4, ImGui::GetFrameHeight()));
            ImGui::SetItemTooltip("Capture Frame (Alt+C)");
            if (capture) FrameDebugger::Capture();
            ImGui::EndDisabled();

            ImGui::PopStyleVar();
        }
        EditorGUI::EndMainViewportSideBar();

        ConsoleWindow::DrawMainViewportSideBarConsole();

        ImGui::DockSpaceOverViewport();
    }

    void EditorApplication::OnTick()
    {
        m_SwapChain->WaitForFrameLatency();
        CalculateFrameStats();

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {
            DrawBaseImGui();
            DotNet::RuntimeInvoke(ManagedMethod::Application_Tick);

            ImGui::Render();
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_SwapChain->GetBackBuffer());
        }

        ImGui::EndFrame();
        GetGfxDevice()->EndFrame();
        m_SwapChain->Present();
    }

    void EditorApplication::OnResize()
    {
        m_SwapChain->Resize(GetClientWidth(), GetClientHeight());
    }

    static std::string GetFontPath(std::string fontName)
    {
        std::string basePath = PathUtils::GetWorkingDirectoryUtf8();
        return basePath + "\\Resources\\Fonts\\" + fontName;
    }

    static std::string GetFontAwesomePath(std::string fontName)
    {
        std::string basePath = PathUtils::GetWorkingDirectoryUtf8();
        return basePath + "\\Resources\\FontAwesome\\" + fontName;
    }

    void EditorApplication::ReloadFonts()
    {
        const float dpiScale = GetDisplayScale();

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

    void EditorApplication::OnDisplayScaleChange()
    {
        LOG_TRACE("DPI Changed: {}", GetDisplayScale());

        ReloadFonts();
        ImGui_ImplDX12_ReloadFontTexture();
    }

    void EditorApplication::OnPaint()
    {
        OnTick();
    }

    void EditorApplication::CalculateFrameStats()
    {
        // Code computes the average frames per second, and also the 
        // average time it takes to render one frame.  These stats 
        // are appended to the window caption bar.

        static int frameCnt = 0;
        static float timeElapsed = 0.0f;

        frameCnt++;

        // Compute averages over one second period.
        if ((GetElapsedTime() - timeElapsed) >= 1.0f)
        {
            float fps = (float)frameCnt; // fps = frameCnt / 1
            float mspf = 1000.0f / fps;

            std::string fpsStr = std::to_string(fps);
            std::string mspfStr = std::to_string(mspf);
            SetWindowTitle("March Engine <DX12>    fps: " + fpsStr + "   mspf: " + mspfStr);

            // Reset for next average.
            frameCnt = 0;
            timeElapsed += 1.0f;
        }
    }

    std::string EditorApplication::SaveFilePanelInProject(const std::string& title, const std::string& defaultName, const std::string& extension, const std::string& path) const
    {
        std::wstring wBasePathWinStyle = StringUtils::Utf8ToUtf16(GetDataPath());
        if (!path.empty())
        {
            wBasePathWinStyle += L'\\';
            wBasePathWinStyle += StringUtils::Utf8ToUtf16(path);

            if (wBasePathWinStyle.back() == L'\\' || wBasePathWinStyle.back() == L'/')
            {
                wBasePathWinStyle.pop_back();
            }
        }
        std::replace(wBasePathWinStyle.begin(), wBasePathWinStyle.end(), L'/', L'\\');

        std::wstring wExtension = StringUtils::Utf8ToUtf16(extension);

        std::vector<wchar_t> filter{};
        filter.insert(filter.end(), wExtension.begin(), wExtension.end());
        filter.push_back(L' ');
        filter.push_back(L'F');
        filter.push_back(L'i');
        filter.push_back(L'l');
        filter.push_back(L'e');
        filter.push_back(L'\0');
        filter.push_back(L'*');
        filter.push_back(L'.');
        filter.insert(filter.end(), wExtension.begin(), wExtension.end());
        filter.push_back(L'\0');
        filter.push_back(L'\0');

        std::wstring fileNameBuffer = StringUtils::Utf8ToUtf16(defaultName);
        fileNameBuffer.resize(MAX_PATH);

        std::wstring wTitle = StringUtils::Utf8ToUtf16(title);

        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GetWindowHandle();
        ofn.lpstrFilter = filter.data();
        ofn.lpstrFile = fileNameBuffer.data();
        ofn.nMaxFile = static_cast<DWORD>(fileNameBuffer.size());
        ofn.lpstrTitle = wTitle.c_str();
        ofn.lpstrInitialDir = wBasePathWinStyle.c_str();
        ofn.lpstrDefExt = wExtension.c_str();
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if (GetSaveFileNameW(&ofn) && fileNameBuffer.find(wBasePathWinStyle) != std::wstring::npos)
        {
            std::string result = StringUtils::Utf16ToUtf8(fileNameBuffer.c_str()); // 使用 c_str 忽略 buffer 后面大量 '\0'
            std::replace(result.begin(), result.end(), '\\', '/');
            return result.substr(GetDataPath().size() + 1); // 返回相对 Data 目录的路径
        }

        return "";
    }
}
