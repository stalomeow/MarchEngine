#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Misc/PlatformUtils.h"
#include <cmath>
#include <wrl.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>

using namespace Microsoft::WRL;

namespace march
{
    float GfxUtils::SRGBToLinearSpace(float x)
    {
        // Approximately pow(x, 2.2)
        return (x < 0.04045f) ? (x / 12.92f) : std::powf((x + 0.055f) / 1.055f, 2.4f);
    }

    float GfxUtils::LinearToSRGBSpace(float x)
    {
        // Approximately pow(x, 1.0 / 2.2)
        return (x < 0.0031308f) ? (12.92f * x) : (1.055f * std::powf(x, 1.0f / 2.4f) - 0.055f);
    }

    void GfxUtils::ReportLiveObjects()
    {
        ComPtr<IDXGIDebug1> debug = nullptr;
        CHECK_HR(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)));
        CHECK_HR(debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL));
    }

    void GfxUtils::SetName(ID3D12Object* obj, const std::string& name)
    {
#ifdef ENABLE_GFX_DEBUG_NAME
        CHECK_HR(obj->SetName(PlatformUtils::Windows::Utf8ToWide(name).c_str()));
#endif
    }
}
