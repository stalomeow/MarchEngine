#pragma once

#include <optional>
#include <fmt/base.h>

namespace march
{
    enum class FrameDebuggerPlugin
    {
        RenderDoc,
        PIX,
        NsightGraphics,
    };

    struct FrameDebugger final
    {
        static uint32_t NumFramesToCapture;

        static std::optional<FrameDebuggerPlugin> GetLoadedPlugin();

        static bool IsPluginLoaded(FrameDebuggerPlugin plugin);

        static void LoadPlugin(FrameDebuggerPlugin plugin);

        static bool IsCaptureAvailable();

        static void Capture();
    };
}

// Ref: https://fmt.dev/11.1/api/#formatting-user-defined-types
template <>
struct fmt::formatter<march::FrameDebuggerPlugin> : formatter<string_view>
{
    // parse is inherited from formatter<string_view>.

    format_context::iterator format(march::FrameDebuggerPlugin plugin, format_context& ctx) const
    {
        string_view name = "Unknown Plugin";
        switch (plugin)
        {
        case march::FrameDebuggerPlugin::RenderDoc:      name = "RenderDoc";              break;
        case march::FrameDebuggerPlugin::PIX:            name = "PIX";                    break;
        case march::FrameDebuggerPlugin::NsightGraphics: name = "NVIDIA Nsight Graphics"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};
