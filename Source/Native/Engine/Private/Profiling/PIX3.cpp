#include "pch.h"
#include "Engine/Profiling/PIX3.h"
#include "Engine/Debug.h"
#include <pix3.h>

namespace march
{
    static bool g_IsPIX3Loaded = false;

    bool PIX3::IsLoaded()
    {
        return g_IsPIX3Loaded;
    }

    void PIX3::Load()
    {
        if (IsLoaded())
        {
            return;
        }

        HMODULE hModule = PIXLoadLatestWinPixGpuCapturerLibrary();

        if (hModule == NULL)
        {
            LOG_ERROR("Failed to load PIX library");
            return;
        }

        if (FAILED(PIXSetHUDOptions(PIX_HUD_SHOW_ON_NO_WINDOWS)))
        {
            LOG_WARNING("Failed to set PIX HUD options");
        }

        g_IsPIX3Loaded = true;
    }

    void PIX3::CaptureSingleFrame()
    {
        if (!IsLoaded())
        {
            return;
        }

        if (FAILED(PIXGpuCaptureNextFrames(L"C:\\Users\\10247\\Desktop\\test.wpix", 3)))
        {
            LOG_ERROR("Failed to capture PIX frame");
        }
    }
}
