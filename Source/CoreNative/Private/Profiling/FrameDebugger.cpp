#include "pch.h"
#include "Engine/Profiling/FrameDebugger.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Misc/TimeUtils.h"
#include "Engine/Application.h"
#include "Engine/Debug.h"
#include "renderdoc_app.h"
#include <filesystem>
#include <fmt/chrono.h>
#include <pix3.h>
#include <array>

namespace fs = std::filesystem;

namespace march
{
    struct RenderDocPlugin
    {
        static constexpr FrameDebuggerPlugin Type = FrameDebuggerPlugin::RenderDoc;

        // Ref: https://renderdoc.org/docs/in_application_api.html

        static inline RENDERDOC_API_1_5_0* pApi = nullptr;

        static bool Load()
        {
            HMODULE hModule = LoadLibraryA("C:\\Program Files\\RenderDoc\\renderdoc.dll");

            if (hModule == NULL)
            {
                LOG_ERROR("Failed to load RenderDoc library");
                return false;
            }

            auto RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(reinterpret_cast<void*>(GetProcAddress(hModule, "RENDERDOC_GetAPI")));
            int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, reinterpret_cast<void**>(&pApi));

            if (ret != 1)
            {
                pApi = nullptr;
                LOG_ERROR("Failed to get RenderDoc API. Return Code: {}", ret);
                return false;
            }

            pApi->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None); // 不显示 overlay
            pApi->SetCaptureKeys(nullptr, 0);
            return true;
        }

        static void Capture(uint32_t numFrames)
        {
            pApi->TriggerMultiFrameCapture(numFrames);

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

    struct PIXPlugin
    {
        static constexpr FrameDebuggerPlugin Type = FrameDebuggerPlugin::PIX;

        // Ref: https://devblogs.microsoft.com/pix/winpixeventruntime/
        // Ref: https://devblogs.microsoft.com/pix/programmatic-capture/
        // Ref: https://devblogs.microsoft.com/pix/gpu-captures/

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

        static void Capture(uint32_t numFrames)
        {
            fs::path path = StringUtils::Utf8ToUtf16(GetApp()->GetDataPath() + "/Logs");

            if (!fs::exists(path) && !fs::create_directories(path))
            {
                LOG_ERROR("Failed to create directory: {}", path.string());
                return;
            }

            path /= StringUtils::Format(L"Capture-{:%Y-%m-%d-%H-%M-%S}.wpix", TimeUtils::GetLocalTime());

            if (SUCCEEDED(PIXGpuCaptureNextFrames(path.c_str(), static_cast<UINT32>(numFrames))))
            {
                PIXOpenCaptureInUI(path.c_str());
            }
            else
            {
                LOG_ERROR("Failed to capture PIX frame");
            }
        }
    };

    template <typename... _Plugin>
    class FrameDebuggerPluginManagerImpl
    {
        static constexpr size_t NumMaxPlugin = 3;
        static_assert(sizeof...(_Plugin) <= NumMaxPlugin, "Too many frame debugger plugin");

        static constexpr auto s_LoadFuncs = []()
        {
            std::array<bool(*)(), NumMaxPlugin> results{};
            ((results[static_cast<size_t>(_Plugin::Type)] = &_Plugin::Load), ...);
            return results;
        }();

        static constexpr auto s_CaptureFuncs = []()
        {
            std::array<void(*)(uint32_t), NumMaxPlugin> results{};
            ((results[static_cast<size_t>(_Plugin::Type)] = &_Plugin::Capture), ...);
            return results;
        }();

        static inline std::optional<FrameDebuggerPlugin> s_LoadedPlugin = std::nullopt;

    public:
        static auto GetLoadedPlugin() { return s_LoadedPlugin; }

        static void Load(FrameDebuggerPlugin plugin)
        {
            if (s_LoadedPlugin)
            {
                LOG_ERROR("Frame debugger has already loaded one plugin: '{}'", *s_LoadedPlugin);
                return;
            }

            if (auto fn = s_LoadFuncs[static_cast<size_t>(plugin)])
            {
                if (fn())
                {
                    s_LoadedPlugin = plugin;
                }
            }
            else
            {
                LOG_ERROR("Unsupported frame debugger plugin: '{}'", plugin);
            }
        }

        static void Capture(uint32_t numFrames)
        {
            if (!s_LoadedPlugin)
            {
                LOG_WARNING("No frame debugger plugin is loaded");
                return;
            }

            if (auto fn = s_CaptureFuncs[static_cast<size_t>(*s_LoadedPlugin)])
            {
                fn(numFrames);
            }
            else
            {
                LOG_ERROR("Unsupported frame debugger plugin: '{}'", *s_LoadedPlugin);
            }
        }
    };

    using FrameDebuggerPluginManager = FrameDebuggerPluginManagerImpl<RenderDocPlugin, PIXPlugin>;

    uint32_t FrameDebugger::NumFramesToCapture = 1;

    std::optional<FrameDebuggerPlugin> FrameDebugger::GetLoadedPlugin()
    {
        return FrameDebuggerPluginManager::GetLoadedPlugin();
    }

    bool FrameDebugger::IsPluginLoaded(FrameDebuggerPlugin plugin)
    {
        return FrameDebuggerPluginManager::GetLoadedPlugin() == plugin;
    }

    void FrameDebugger::LoadPlugin(FrameDebuggerPlugin plugin)
    {
        FrameDebuggerPluginManager::Load(plugin);
    }

    bool FrameDebugger::IsCaptureAvailable()
    {
        return FrameDebuggerPluginManager::GetLoadedPlugin().has_value();
    }

    void FrameDebugger::Capture()
    {
        FrameDebuggerPluginManager::Capture(NumFramesToCapture);
    }
}
