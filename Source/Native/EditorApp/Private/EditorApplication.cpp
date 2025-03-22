#include "pch.h"
#include "resource.h"
#include "Editor/EditorApplication.h"
#include "Editor/BusyProgressBar.h"
#include "Editor/EditorGUI.h"
#include "Editor/ConsoleWindow.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/Rendering/RenderPipeline.h"
#include "Engine/Rendering/Display.h"
#include "Engine/ImGui/IconsFontAwesome6.h"
#include "Engine/ImGui/IconsFontAwesome6Brands.h"
#include "Engine/ImGui/ImGuiBackend.h"
#include "Engine/ImGui/ImGuiStyleManager.h"
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
#include <stdexcept>
#include <filesystem>
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
        , m_DataPath{}
        , m_EngineResourcePath{}
        , m_EngineShaderPath{}
        , m_ImGuiIniFilename{}
        , m_IsInitialized(false)
    {
    }

    EditorApplication::~EditorApplication() = default;

    void EditorApplication::OnStart(const std::vector<std::string>& args)
    {
        constexpr auto programName = "EditorApp";
        argparse::ArgumentParser program(programName, "1.0", argparse::default_arguments::none);

        program.add_argument("--project").metavar("PATH").help("Specify the project path").required();

        auto& gfx = program.add_mutually_exclusive_group();
        gfx.add_argument("--renderdoc").help("Load RenderDoc plugin").flag();
        gfx.add_argument("--pix").help("Load PIX plugin").flag();
        gfx.add_argument("--d3d12-debug-layer").help("Enable D3D12 debug layer").flag();

        try
        {
            std::vector<std::string> fullArgs{};
            fullArgs.push_back(programName); // argparse 要求第一个参数是程序名称，但是 Windows 传给我们的参数列表没有程序名称
            fullArgs.insert(fullArgs.end(), args.begin(), args.end());
            program.parse_args(fullArgs);
        }
        catch (const std::exception& err)
        {
            ShowErrorMessageBox(err.what(), program.help().str());
            std::exit(1);
        }

        InitProject(program.get("--project"));

        GfxDeviceDesc desc{};

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

        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = 1024;
        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = 64;
        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = 64;
        desc.OfflineDescriptorPageSizes[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = 64;
        desc.OnlineViewDescriptorHeapSize = 10000;
        desc.OnlineSamplerDescriptorHeapSize = 2048;

        DotNet::InitRuntime(); // 越早越好，mixed debugger 需要 runtime 加载完后才能工作
        GfxDevice* device = InitGfxDevice(desc);

        m_SwapChain = std::make_unique<GfxSwapChain>(device, GetWindowHandle(), GetClientWidth(), GetClientHeight());
        m_ProgressBar = std::make_unique<BusyProgressBar>("March 7th is working", 300 /* ms */);

        Display::CreateMainDisplay(10, 10); // dummy display

        InitImGui();
    }

    void EditorApplication::InitProject(const std::string& path)
    {
        if (fs::exists(path))
        {
            if (!fs::is_directory(path))
            {
                ShowErrorMessageBox("Project path is not a directory.");
                std::exit(1);
            }
        }
        else
        {
            fs::create_directories(path);
        }

        m_DataPath = path;

        // Normalize path
        std::replace(m_DataPath.begin(), m_DataPath.end(), '\\', '/');
        while (!m_DataPath.empty() && m_DataPath.back() == '/')
        {
            m_DataPath.pop_back();
        }

#ifdef ENGINE_RESOURCE_UNIX_PATH
        m_EngineResourcePath = ENGINE_RESOURCE_UNIX_PATH;
#else
        m_EngineResourcePath = PathUtils::GetWorkingDirectoryUtf8(PathStyle::Unix) + "/Resources";
#endif

#ifdef ENGINE_SHADER_UNIX_PATH
        m_EngineShaderPath = ENGINE_SHADER_UNIX_PATH;
#else
        m_EngineShaderPath = PathUtils::GetWorkingDirectoryUtf8(PathStyle::Unix) + "/Shaders";
#endif

        SetWindowTitle(StringUtils::Format("March Engine <DX12> - {}", m_DataPath));
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
        io.IniFilename = m_ImGuiIniFilename.c_str();
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.ConfigDockingAlwaysTabBar = true;

        ImGui_ImplWin32_Init(GetWindowHandle());
        ImGuiStyleManager::ApplyDefaultStyle();
        ReloadFonts();

        ImGui_ImplDX12_Init(GetGfxDevice(), "Engine/Shaders/DearImGui.shader");

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

    void EditorApplication::DrawBaseImGui()
    {
        // Main Menu Bar 占位
        if (EditorGUI::BeginMainMenuBar())
        {
            EditorGUI::EndMainMenuBar();
        }

        if (EditorGUI::BeginMainViewportSideBar("##SingleLineToolbar", ImGuiDir_Up, ImGui::GetFrameHeight()))
        {
            // Frame Stats
            DrawFrameStats();
            ImGui::SameLine();

            // Center Buttons
            float width1 = EditorGUI::CalcButtonWidth(ICON_FA_PLAY) * 1.8f;
            float width2 = EditorGUI::CalcButtonWidth(ICON_FA_PAUSE) * 1.8f;
            float width3 = EditorGUI::CalcButtonWidth(ICON_FA_FORWARD_STEP) * 1.8f;
            float width4 = EditorGUI::CalcButtonWidth(ICON_FA_CAMERA) * 1.8f;
            float buttonWidth = width1 + width2 + width3 + width4;
            float contentTotalWidth = ImGui::GetContentRegionMax().x;
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - (contentTotalWidth + buttonWidth) * 0.5f);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3, 0));

            ImGui::BeginDisabled();
            ImGui::Button(ICON_FA_PLAY, ImVec2(width1, ImGui::GetFrameHeight()));
            ImGui::SameLine();
            ImGui::Button(ICON_FA_PAUSE, ImVec2(width2, ImGui::GetFrameHeight()));
            ImGui::SameLine();
            ImGui::Button(ICON_FA_FORWARD_STEP, ImVec2(width3, ImGui::GetFrameHeight()));
            ImGui::EndDisabled();

            ImGui::SameLine();

            if (FrameDebugger::IsCaptureAvailable() && ImGui::Shortcut(ImGuiMod_Alt | ImGuiKey_C, ImGuiInputFlags_RouteAlways))
            {
                FrameDebugger::Capture();
            }

            ImGui::BeginDisabled(!FrameDebugger::IsCaptureAvailable());
            bool capture = ImGui::Button(ICON_FA_CAMERA, ImVec2(width4, ImGui::GetFrameHeight()));
            ImGui::SetItemTooltip("Capture Frames (Alt+C)");
            if (capture) FrameDebugger::Capture();
            ImGui::EndDisabled();

            ImGui::PopStyleVar();
        }
        EditorGUI::EndMainViewportSideBar();

        ConsoleWindow::DrawMainViewportSideBarConsole();
    }

    void EditorApplication::OnTick(bool willQuit)
    {
        BusyProgressBar::EnabledScope busyScope(m_ProgressBar.get());

        m_SwapChain->WaitForFrameLatency();

        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {
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

            if (willQuit)
            {
                DotNet::RuntimeInvoke(ManagedMethod::Application_Quit);
            }
            else
            {
                m_ProgressBar->ReportAlive();
                RenderPipeline* rp = GetRenderPipeline();

                rp->PrepareFrameData();
                DrawBaseImGui();

                DotNet::RuntimeInvoke(ManagedMethod::Application_Tick);
                rp->Render();

                ImGui::Render();
                ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_SwapChain->GetBackBuffer());
            }
        }

        ImGui::EndFrame();
        GetGfxDevice()->EndFrame();

        if (!willQuit)
        {
            m_SwapChain->Present();
        }
    }

    void EditorApplication::OnResize()
    {
        m_SwapChain->Resize(GetClientWidth(), GetClientHeight());
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

    void EditorApplication::OnDisplayScaleChange()
    {
        LOG_TRACE("DPI Changed: {}", GetDisplayScale());

        ReloadFonts();
        ImGui_ImplDX12_ReloadFontTexture();
    }

    void EditorApplication::OnPaint()
    {
        OnTick(false);
    }

    HICON EditorApplication::GetIcon()
    {
        return LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_ICON_MARCH_7TH));
    }

    LRESULT EditorApplication::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result;

        if (ImGui_ImplWin32_WndProcHandler(GetWindowHandle(), msg, wParam, lParam))
        {
            result = true;
        }
        else
        {
            result = Application::HandleMessage(msg, wParam, lParam);
        }

        return result;
    }

    void EditorApplication::DrawFrameStats()
    {
        // Code computes the average frames per second, and also the average time it takes to render one frame.
        // These stats are appended to the window caption bar.

        static int fps = 0;
        static int frameCnt = 0;
        static float timeElapsed = 0.0f;

        frameCnt++;

        // Compute averages over one second period.
        if ((GetElapsedTime() - timeElapsed) >= 1.0f)
        {
            fps = frameCnt; // fps = frameCnt / 1

            // Reset for next average.
            frameCnt = 0;
            timeElapsed += 1.0f;
        }

        constexpr auto fpsLabel = "FPS:";
        constexpr auto fpsSlash = "/";
        std::string fpsText = std::to_string(fps);
        std::string mspfText = StringUtils::Format("{:.1f} ms", 1000.0f / fps);

        float width
            = ImGui::CalcTextSize(fpsLabel).x
            + ImGui::CalcTextSize(fpsText.c_str()).x
            + ImGui::CalcTextSize(fpsSlash).x
            + ImGui::CalcTextSize(mspfText.c_str()).x
            + ImGui::GetStyle().ItemSpacing.x * 3;
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - width);

        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(fpsLabel);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextUnformatted(fpsText.c_str());
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(fpsSlash);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextUnformatted(mspfText.c_str());
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
