#pragma once

namespace march
{
    struct PIX3 final
    {
        static bool IsLoaded();
        static void Load();
        static void CaptureSingleFrame();
    };
}
