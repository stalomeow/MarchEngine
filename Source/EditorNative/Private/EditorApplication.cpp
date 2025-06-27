#include "pch.h"
#include "resource.h"
#include "EditorApplication.h"
#include "BusyProgressBar.h"
#include "EditorGUI.h"
#include "ConsoleWindow.h"
#include "DragDrop.h"
#include "Gizmos.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/Rendering/RenderPipeline.h"
#include "Engine/Rendering/Display.h"
#include "IconsFontAwesome6.h"
#include "IconsFontAwesome6Brands.h"
#include "ImGuiBackend.h"
#include "ImGuiStyleManager.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Misc/PlatformUtils.h"
#include "Engine/Misc/DeferFunc.h"
#include "Engine/Scripting/DotNetRuntime.h"
#include "Engine/Profiling/FrameDebugger.h"
#include "Engine/Profiling/NsightAftermath.h"
#include "Engine/Debug.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "ImGuizmo.h"
#include <d3dx12.h>
#include <algorithm>
#include <stdexcept>
#include <filesystem>
#include <sstream>
#include <argparse/argparse.hpp>

using namespace DirectX;
namespace fs = std::filesystem;

// Forward declare message handler from imgui_impl_win32.cpp
// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace march
{
    EditorApplication::EditorApplication()
        : m_SwapChain(nullptr)
        , m_ProgressBar(nullptr)
        , m_ProjectName{}
        , m_DataPath{}
        , m_EngineResourcePath{}
        , m_EngineShaderPath{}
        , m_ShaderCachePath{}
        , m_ImGuiIniFilename{}
        , m_IsInitialized(false)
    {
    }

    EditorApplication::~EditorApplication() = default;

    void EditorApplication::OnStart(const std::vector<std::string>& args)
    {
        argparse::ArgumentParser program(EDITOR_APP_NAME, EDITOR_APP_VERSION, argparse::default_arguments::none);

        program.add_argument("--project").metavar("PATH").help("Specify the project path").required();

        auto& gfx = program.add_mutually_exclusive_group();
        gfx.add_argument("--renderdoc").help("Load RenderDoc plugin").flag();
        gfx.add_argument("--pix").help("Load PIX plugin").flag();
        gfx.add_argument("--d3d12-debug-layer").help("Enable D3D12 debug layer").flag();
        gfx.add_argument("--nvaftermath").help("Enable Minimum Nsight Aftermath").flag();
        gfx.add_argument("--nvaftermath-full").help("Enable Full Nsight Aftermath").flag();

        try
        {
            std::vector<std::string> fullArgs{};
            fullArgs.push_back(EDITOR_APP_NAME); // argparse 要求第一个参数是程序名称，但是 Windows 传给我们的参数列表没有程序名称
            fullArgs.insert(fullArgs.end(), args.begin(), args.end());
            program.parse_args(fullArgs);
        }
        catch (const std::exception& err)
        {
            CrashWithMessage(err.what(), program.help().str());
        }

        InitProject(program.get("--project"));

        GfxDeviceDesc desc{};
        bool useNsightAftermath = false;

        if (program["--renderdoc"] == true)
        {
            FrameDebugger::LoadPlugin(FrameDebuggerPlugin::RenderDoc); // 越早越好
        }
        else if (program["--pix"] == true)
        {
            FrameDebugger::LoadPlugin(FrameDebuggerPlugin::PIX); // 越早越好
        }
        else if (program["--d3d12-debug-layer"] == true)
        {
            desc.EnableDebugLayer = true;
        }
        else if (program["--nvaftermath"] == true)
        {
            useNsightAftermath = true;
            NsightAftermath::InitializeBeforeDeviceCreation(/* fullFeatures */ false);
        }
        else if (program["--nvaftermath-full"] == true)
        {
            useNsightAftermath = true;
            NsightAftermath::InitializeBeforeDeviceCreation(/* fullFeatures */ true);
        }

        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = 1024;
        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = 64;
        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = 64;
        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = 64;
        desc.OnlineViewDescriptorHeapSize = 10000;
        desc.OnlineSamplerDescriptorHeapSize = 2048;

        DotNet::InitRuntime(); // 越早越好，mixed debugger 需要 runtime 加载完后才能工作
        GfxDevice* device = InitGfxDevice(desc);

        if (useNsightAftermath)
        {
            NsightAftermath::InitializeDevice(device->GetD3DDevice4());
        }

        m_SwapChain = std::make_unique<GfxSwapChain>(device, GetWindowHandle(), GetClientWidth(), GetClientHeight());
        m_ProgressBar = std::make_unique<BusyProgressBar>("March 7th is working", 300 /* ms */);

        Display::CreateMainDisplay(10, 10); // dummy display

        if (!DropManager::Initialize(GetWindowHandle()))
        {
            CrashWithMessage("Error", "Failed to initialize drag and drop manager.");
        }

        InitImGui();
    }

    static std::string GetNormalizedProjectPath(const std::string& path)
    {
        std::string result = path;
        std::replace(result.begin(), result.end(), '\\', '/');
        while (!result.empty() && result.back() == '/')
        {
            result.pop_back();
        }
        return result;
    }

    void EditorApplication::InitProject(const std::string& path)
    {
        if (fs::exists(path))
        {
            if (!fs::is_directory(path))
            {
                Application::CrashWithMessage("Project path is not a directory.");
            }
        }
        else
        {
            fs::create_directories(path);
        }

        m_DataPath = GetNormalizedProjectPath(path);
        m_ProjectName = fs::path(m_DataPath).filename().string();
        m_ShaderCachePath = m_DataPath + "/Library/ShaderCache";

#ifdef ENGINE_RESOURCE_UNIX_PATH
        m_EngineResourcePath = ENGINE_RESOURCE_UNIX_PATH;
#else
        m_EngineResourcePath = PlatformUtils::GetExecutableDirectory() + "/Resources";
#endif

#ifdef ENGINE_SHADER_UNIX_PATH
        m_EngineShaderPath = ENGINE_SHADER_UNIX_PATH;
#else
        m_EngineShaderPath = PlatformUtils::GetExecutableDirectory() + "/Shaders";
#endif

#ifdef _DEBUG
        SetWindowTitle(StringUtils::Format("{} - March Engine [Debug]", m_ProjectName));
#else
        SetWindowTitle(StringUtils::Format("{} - March Engine [Release]", m_ProjectName));
#endif

        LOG_INFO("Welcome to March Engine!");
    }

    bool EditorApplication::IsEngineResourceEditable() const
    {
#ifdef ENGINE_RESOURCE_UNIX_PATH
        return true;
#else
        return false;
#endif
    }

    bool EditorApplication::IsEngineShaderEditable() const
    {
#ifdef ENGINE_SHADER_UNIX_PATH
        return true;
#else
        return false;
#endif
    }

    void EditorApplication::InitImGui()
    {
        m_ImGuiIniFilename = GetDataPath() + "/ProjectSettings/imgui.ini";

        // Setup Dear ImGui context
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.ConfigDockingTransparentPayload = true; // https://github.com/ocornut/imgui/issues/2361
        io.ConfigViewportsNoAutoMerge = true;
        io.ConfigViewportsNoTaskBarIcon = true;
        io.ConfigViewportsNoDecoration = true;
        io.ConfigViewportsNoDefaultParent = false;
        io.IniFilename = m_ImGuiIniFilename.c_str();

        ImGui_ImplWin32_Init(GetWindowHandle());
        ImGuiStyleManager::ApplyDefaultStyle();
        ReloadFonts();

        ImGui_ImplDX12_Init(GetGfxDevice());

        // Scene View Gizmo Style
        ImGuizmo::Style& style = ImGuizmo::GetStyle();
        style.RotationLineThickness = 3.0f;
        style.RotationOuterLineThickness = 2.0f;
    }

    void EditorApplication::OnQuit()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        m_SwapChain.reset();

        Display::DestroyMainDisplay();
        GfxTexture::ClearSamplerCache();
        ShaderUtils::ClearRootSignatureCache();

        DotNet::RuntimeInvoke(ManagedMethod::Application_FullGC);
        DotNet::DestroyRuntime();

        DestroyGfxDevice();
        GfxUtils::ReportLiveObjects();
    }

    void EditorApplication::CrashWithMessage(const std::string& title, const std::string& message, bool debugBreak)
    {
        if (m_ProgressBar)
        {
            // 强制关闭进度条，避免遮挡
            m_ProgressBar->EndEnabledScope();
            DEFER_FUNC() { m_ProgressBar->BeginEnabledScope(); };

            Application::CrashWithMessage(title, message, debugBreak);
        }
        else
        {
            Application::CrashWithMessage(title, message, debugBreak);
        }
    }

    void EditorApplication::DrawBaseImGui()
    {
        auto drawCenterToolButtons = [](float buttonHeight)
        {
            float width1 = EditorGUI::CalcButtonWidth(ICON_FA_PLAY) * 1.8f;
            float width2 = EditorGUI::CalcButtonWidth(ICON_FA_PAUSE) * 1.8f;
            float width3 = EditorGUI::CalcButtonWidth(ICON_FA_FORWARD_STEP) * 1.8f;
            float width4 = EditorGUI::CalcButtonWidth(ICON_FA_CAMERA) * 1.8f;

            float buttonWidths = width1 + width2 + width3 + width4;
            ImGui::SetCursorPosX((ImGui::GetContentRegionMax().x - buttonWidths) * 0.5f);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            {
                ImGui::BeginDisabled();
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(141, 193, 76, 255)); // 播放按钮显示为绿色
                ImGui::ButtonEx(ICON_FA_PLAY, ImVec2(width1, buttonHeight), ImGuiButtonFlags_None, ImDrawFlags_RoundCornersLeft);
                ImGui::PopStyleColor();
                ImGui::SetItemTooltip("Play");
                ImGui::SameLine();
                ImGui::ButtonEx(ICON_FA_PAUSE, ImVec2(width2, buttonHeight), ImGuiButtonFlags_None, ImDrawFlags_RoundCornersNone);
                ImGui::SetItemTooltip("Pause");
                ImGui::SameLine();
                ImGui::ButtonEx(ICON_FA_FORWARD_STEP, ImVec2(width3, buttonHeight), ImGuiButtonFlags_None, ImDrawFlags_RoundCornersNone);
                ImGui::SetItemTooltip("Step");
                ImGui::EndDisabled();

                ImGui::SameLine();

                if (FrameDebugger::IsCaptureAvailable() && ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_C, ImGuiInputFlags_RouteAlways))
                {
                    FrameDebugger::Capture();
                }

                ImGui::BeginDisabled(!FrameDebugger::IsCaptureAvailable());
                bool capture = ImGui::ButtonEx(ICON_FA_CAMERA, ImVec2(width4, buttonHeight), ImGuiButtonFlags_None, ImDrawFlags_RoundCornersRight);
                ImGui::SetItemTooltip("Capture Frame (Alt+C)");
                if (capture) FrameDebugger::Capture();
                ImGui::EndDisabled();
            }
            ImGui::PopStyleVar();
        };

        if (EditorGUI::BeginMainMenuBar())
        {
            // 替换 MenuBar 的 Clip Rect
            ImRect rect = ImGui::GetCurrentWindow()->Rect();
            ImGui::PushClipRect(rect.Min, rect.Max, false);

            // 替换 MenuBar 的 PosY
            float buttonHeight = ImGui::GetFrameHeight();
            ImVec2 initialCursorPos = ImGui::GetCursorPos();
            ImGui::SetCursorPosY((rect.GetHeight() - buttonHeight) * 0.5f);

            drawCenterToolButtons(buttonHeight);

            // 恢复原本 MenuBar 的设置
            ImGui::SetCursorPos(initialCursorPos);
            ImGui::PopClipRect();

            EditorGUI::EndMainMenuBar();
        }

        ConsoleWindow::DrawMainViewportSideBarConsole();
    }

    void EditorApplication::OnTick(bool willQuit)
    {
        m_ProgressBar->BeginEnabledScope();
        DEFER_FUNC() { m_ProgressBar->EndEnabledScope(); };

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // C# 在第一次初始化 EditorWindow 时要用到 ImGui 的 DockSpace
        EditorWindow::DockSpaceOverMainViewport();

        if (!m_IsInitialized)
        {
            // Initialization
            DotNet::RuntimeInvoke(ManagedMethod::Application_Initialize);
            DotNet::RuntimeInvoke(ManagedMethod::EditorApplication_Initialize);

            // Post Initialization
            DotNet::RuntimeInvoke(ManagedMethod::Application_PostInitialize);
            DotNet::RuntimeInvoke(ManagedMethod::EditorApplication_PostInitialize);

            m_IsInitialized = true;
        }

        if (!willQuit)
        {
            m_SwapChain->WaitForFrameLatency();
            m_ProgressBar->ReportAlive();

            RenderPipeline* rp = GetRenderPipeline();

            rp->PrepareFrameData();
            DrawBaseImGui();

            DotNet::RuntimeInvoke(ManagedMethod::Application_Tick);

            rp->Render();
            Gizmos::Render();

            ImGui::Render();
            ImGui_ImplDX12_RenderAndPresent(m_SwapChain.get());
        }
        else
        {
            DotNet::RuntimeInvoke(ManagedMethod::Application_Quit);

            // Skip rendering
            ImGui::EndFrame();
        }

        GfxDevice* device = GetGfxDevice();
        device->GetCommandManager()->SignalNextFrameFence(/* waitForGpuIdle */ false);
        device->CleanupResources();
    }

    static std::string GetFontPath(Application* app, std::string fontName)
    {
        return app->GetEngineResourcePath() + "/Fonts/" + fontName;
    }

    static std::string GetFontAwesomePath(Application* app, std::string fontName)
    {
        return app->GetEngineResourcePath() + "/FontAwesome/" + fontName;
    }

    void EditorApplication::ReloadFonts()
    {
        constexpr float FontSizeLatin = 15.0f;
        constexpr float FontSizeCJK = 19.0f;
        constexpr float FontSizeIcon = 13.0f;

        const float dpiScale = GetDisplayScale();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        // 英文字体
        ImFontConfig latinConfig{};
        latinConfig.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF(GetFontPath(this, "Inter-Regular.otf").c_str(), FontSizeLatin * dpiScale,
            &latinConfig, io.Fonts->GetGlyphRangesDefault());

        // 中文字体
        ImFontConfig cjkConfig{};
        cjkConfig.MergeMode = true;
        cjkConfig.PixelSnapH = true;
        cjkConfig.RasterizerDensity = 1.5f; // 稍微放大一点，更清晰
        io.Fonts->AddFontFromFileTTF(GetFontPath(this, "NotoSansSC-Regular.ttf").c_str(), FontSizeCJK * dpiScale,
            &cjkConfig, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

        // Font Awesome 图标字体
        float iconFontSizePixels = FontSizeIcon * dpiScale;
        static const ImWchar faIconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
        static const ImWchar fabIconsRanges[] = { ICON_MIN_FAB, ICON_MAX_16_FAB, 0 };

        ImFontConfig iconConfig;
        iconConfig.MergeMode = true;
        iconConfig.PixelSnapH = true;
        iconConfig.GlyphMinAdvanceX = iconFontSizePixels; // 让所有图标等宽
        iconConfig.GlyphMaxAdvanceX = iconFontSizePixels; // 让所有图标等宽

        // use FONT_ICON_FILE_NAME_FAR if you want regular instead of solid
        io.Fonts->AddFontFromFileTTF(GetFontAwesomePath(this, FONT_ICON_FILE_NAME_FAS).c_str(), iconFontSizePixels,
            &iconConfig, faIconsRanges);
        io.Fonts->AddFontFromFileTTF(GetFontAwesomePath(this, FONT_ICON_FILE_NAME_FAB).c_str(), iconFontSizePixels,
            &iconConfig, fabIconsRanges);

        io.Fonts->Build();
    }

    void EditorApplication::OnResize()
    {
        m_SwapChain->Resize(GetClientWidth(), GetClientHeight());
    }

    void EditorApplication::OnDisplayScaleChange()
    {
        LOG_TRACE("DPI Changed: {}", GetDisplayScale());

        ReloadFonts();
        ImGui_ImplDX12_ReloadFontTexture();
    }

    void EditorApplication::OnPaint()
    {
        Tick(/* willQuit */ false);
    }

    HICON EditorApplication::GetIcon()
    {
        return LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_ICON_MARCH_7TH));
    }

    COLORREF EditorApplication::GetBackgroundColor()
    {
        ImVec4 color = ImGuiStyleManager::GetSystemWindowBackgroundColor();
        return RGB(color.x * 255, color.y * 255, color.z * 255);
    }

    LRESULT EditorApplication::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_DESTROY)
        {
            // 必须在 WindowHandle 被销毁之前调用
            if (!DropManager::Uninitialize(GetWindowHandle()))
            {
                CrashWithMessage("Error", "Failed to uninitialize drag and drop manager.");
            }

            Quit(0);
            return 0;
        }

        if (ImGui_ImplWin32_WndProcHandler(GetWindowHandle(), msg, wParam, lParam))
        {
            return true;
        }

        return Application::HandleMessage(msg, wParam, lParam);
    }

    std::string EditorApplication::SaveFilePanelInProject(const std::string& title, const std::string& defaultName, const std::string& extension, const std::string& path) const
    {
        std::wstring wBasePathWinStyle = PlatformUtils::Windows::Utf8ToWide(GetDataPath());
        if (!path.empty())
        {
            wBasePathWinStyle += L'\\';
            wBasePathWinStyle += PlatformUtils::Windows::Utf8ToWide(path);

            if (wBasePathWinStyle.back() == L'\\' || wBasePathWinStyle.back() == L'/')
            {
                wBasePathWinStyle.pop_back();
            }
        }
        std::replace(wBasePathWinStyle.begin(), wBasePathWinStyle.end(), L'/', L'\\');

        std::wstring wExtension = PlatformUtils::Windows::Utf8ToWide(extension);

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

        std::wstring fileNameBuffer = PlatformUtils::Windows::Utf8ToWide(defaultName);
        fileNameBuffer.resize(MAX_PATH);

        std::wstring wTitle = PlatformUtils::Windows::Utf8ToWide(title);

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
            const auto resultStr = fileNameBuffer.c_str(); // 使用 c_str 忽略 buffer 后面大量 '\0'
            std::string result = PlatformUtils::Windows::WideToUtf8(resultStr);
            std::replace(result.begin(), result.end(), '\\', '/');
            return result.substr(GetDataPath().size() + 1); // 返回相对 Data 目录的路径
        }

        return "";
    }
}
