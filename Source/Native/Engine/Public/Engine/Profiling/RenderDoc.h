#pragma once

#include <stdint.h>
#include <tuple>
#include <string>

namespace march
{
    struct RenderDoc final
    {
        static bool IsLoaded();
        static void Load();
        static void CaptureSingleFrame();
        static uint32_t GetNumCaptures();
        static std::tuple<int32_t, int32_t, int32_t> GetVersion();
        static std::string GetLibraryPath();
    };
}
