#include "pch.h"
#include "Engine/Graphics/RenderDoc.h"
#include "Engine/Debug.h"
#include "renderdoc_app.h"
#include <Windows.h>

namespace march
{
    static RENDERDOC_API_1_5_0* g_Api = nullptr;

    bool RenderDoc::IsLoaded()
    {
        return g_Api != nullptr;
    }

    void RenderDoc::Load()
    {
        if (IsLoaded())
        {
            return;
        }

        // 如果使用 RenderDoc 启动 App 的话，不重复加载 dll
        HMODULE hModule = GetModuleHandleA("renderdoc.dll");

        if (!hModule)
        {
            hModule = LoadLibraryA(GetLibraryPath().c_str());
        }

        if (!hModule)
        {
            LOG_ERROR("Failed to load RenderDoc library");
            return;
        }

        auto RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(hModule, "RENDERDOC_GetAPI"));
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, reinterpret_cast<void**>(&g_Api));

        if (ret != 1)
        {
            g_Api = nullptr;
            LOG_ERROR("Failed to get RenderDoc API. Return Code: %d", ret);
            return;
        }

        g_Api->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None); // 不显示 overlay

        //// 开启 API 验证和调试输出
        //g_Api->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, 1);
        //g_Api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);

        g_Api->SetCaptureKeys(nullptr, 0);
    }

    void RenderDoc::CaptureSingleFrame()
    {
        if (!IsLoaded())
        {
            return;
        }

        g_Api->TriggerCapture();

        if (g_Api->IsTargetControlConnected())
        {
            g_Api->ShowReplayUI();
        }
        else
        {
            g_Api->LaunchReplayUI(1, nullptr);
        }
    }

    uint32_t RenderDoc::GetNumCaptures()
    {
        if (!IsLoaded())
        {
            return 0;
        }

        return g_Api->GetNumCaptures();
    }

    std::tuple<int32_t, int32_t, int32_t> RenderDoc::GetVersion()
    {
        if (!IsLoaded())
        {
            return std::make_tuple(0, 0, 0);
        }

        int verMajor = 0;
        int verMinor = 0;
        int verPatch = 0;
        g_Api->GetAPIVersion(&verMajor, &verMinor, &verPatch);
        return std::make_tuple(static_cast<int32_t>(verMajor), static_cast<int32_t>(verMinor), static_cast<int32_t>(verPatch));
    }

    std::string RenderDoc::GetLibraryPath()
    {
        // TODO find renderdoc on the fly
        return "C:\\Program Files\\RenderDoc\\renderdoc.dll";
    }
}
