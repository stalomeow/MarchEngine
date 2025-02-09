#include "pch.h"
#include "Engine/Profiling/FrameDebugger.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Application.h"
#include "Engine/Debug.h"
#include "renderdoc_app.h"
#include <filesystem>
#include <fmt/chrono.h>
#include <pix3.h>

namespace fs = std::filesystem;

namespace march
{
    struct RenderDocAPI
    {
        static inline RENDERDOC_API_1_5_0* pApi = nullptr;

        static bool Load()
        {
            HMODULE hModule = LoadLibraryA("C:\\Program Files\\RenderDoc\\renderdoc.dll");

            if (hModule == NULL)
            {
                LOG_ERROR("Failed to load RenderDoc library");
                return false;
            }

            auto RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(hModule, "RENDERDOC_GetAPI"));
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, reinterpret_cast<void**>(&pApi));

            if (ret != 1)
            {
                pApi = nullptr;
                LOG_ERROR("Failed to get RenderDoc API. Return Code: {}", ret);
                return false;
            }

            pApi->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None); // 不显示 overlay

            //// 开启 API 验证和调试输出
            //pApi->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, 1);
            //pApi->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);

            pApi->SetCaptureKeys(nullptr, 0);
            return true;
        }

        static void Capture()
        {
            pApi->TriggerCapture();

            if (pApi->IsTargetControlConnected())
            {
                pApi->ShowReplayUI();
            }
            else
            {
                pApi->LaunchReplayUI(1, nullptr);
            }
        }
    };

    struct PIXAPI
    {
        static bool Load()
        {
            HMODULE hModule = PIXLoadLatestWinPixGpuCapturerLibrary();

            if (hModule == NULL)
            {
                LOG_ERROR("Failed to load PIX library");
                return false;
            }

            if (FAILED(PIXSetHUDOptions(PIX_HUD_SHOW_ON_NO_WINDOWS)))
            {
                LOG_WARNING("Failed to set PIX HUD options");
            }

            return true;
        }

        static void Capture()
        {
            fs::path path = StringUtils::Utf8ToUtf16(GetApp()->GetDataPath() + "/Captures");

            if (!fs::exists(path) && !fs::create_directories(path))
            {
                LOG_ERROR("Failed to create directory: {}", path.string());
            }

            path /= StringUtils::Format(L"{:%Y-%m-%d-%H-%M-%S}.wpix", fmt::localtime(std::time(nullptr)));

            if (SUCCEEDED(PIXGpuCaptureNextFrames(path.c_str(), 1)))
            {
                PIXOpenCaptureInUI(path.c_str());
            }
            else
            {
                LOG_ERROR("Failed to capture PIX frame");
            }
        }
    };

    static std::optional<FrameDebuggerPlugin> g_LoadedPlugin = std::nullopt;

    std::optional<FrameDebuggerPlugin> FrameDebugger::GetLoadedPlugin()
    {
        return g_LoadedPlugin;
    }

    void FrameDebugger::LoadPlugin(FrameDebuggerPlugin plugin)
    {
        if (g_LoadedPlugin)
        {
            LOG_ERROR("Frame debugger has already loaded one plugin: '{}'", *g_LoadedPlugin);
            return;
        }

        bool success;

        switch (plugin)
        {
        case FrameDebuggerPlugin::RenderDoc:
            success = RenderDocAPI::Load();
            break;
        case FrameDebuggerPlugin::PIX:
            success = PIXAPI::Load();
            break;
        default:
            LOG_ERROR("Unsupported frame debugger plugin: '{}'", plugin);
            success = false;
            break;
        }

        if (success)
        {
            g_LoadedPlugin = plugin;
        }
    }

    void FrameDebugger::Capture()
    {
        if (!g_LoadedPlugin)
        {
            LOG_WARNING("No frame debugger plugin loaded");
            return;
        }

        switch (*g_LoadedPlugin)
        {
        case FrameDebuggerPlugin::RenderDoc:
            RenderDocAPI::Capture();
            break;
        case FrameDebuggerPlugin::PIX:
            PIXAPI::Capture();
            break;
        default:
            LOG_ERROR("Unsupported frame debugger plugin: '{}'", *g_LoadedPlugin);
            break;
        }
    }
}
