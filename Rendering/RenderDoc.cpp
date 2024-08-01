#include "Rendering/RenderDoc.h"
#include "Core/Debug.h"

namespace dx12demo
{
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
            DEBUG_LOG_ERROR("Failed to load RenderDoc library");
            return;
        }

        auto RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(hModule, "RENDERDOC_GetAPI"));
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, reinterpret_cast<void**>(&m_Api));

        if (ret != 1)
        {
            m_Api = nullptr;
            DEBUG_LOG_ERROR("Failed to get RenderDoc API. Return Code: %d", ret);
            return;
        }

        m_Api->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None); // 不显示 overlay
        m_Api->SetCaptureKeys(nullptr, 0);
    }

    void RenderDoc::CaptureSingleFrame() const
    {
        if (!IsLoaded())
        {
            return;
        }

        m_Api->TriggerCapture();

        if (m_Api->IsTargetControlConnected())
        {
            m_Api->ShowReplayUI();
        }
        else
        {
            m_Api->LaunchReplayUI(1, nullptr);
        }
    }

    uint32_t RenderDoc::GetNumCaptures() const
    {
        if (!IsLoaded())
        {
            return 0;
        }

        return m_Api->GetNumCaptures();
    }

    std::tuple<int, int, int> RenderDoc::GetVersion() const
    {
        if (!IsLoaded())
        {
            return std::make_tuple(0, 0, 0);
        }

        int verMajor = 0;
        int verMinor = 0;
        int verPatch = 0;
        m_Api->GetAPIVersion(&verMajor, &verMinor, &verPatch);
        return std::make_tuple(verMajor, verMinor, verPatch);
    }

    std::string RenderDoc::GetLibraryPath() const
    {
        // TODO find renderdoc on the fly
        return "C:\\Program Files\\RenderDoc\\renderdoc.dll";
    }
}
