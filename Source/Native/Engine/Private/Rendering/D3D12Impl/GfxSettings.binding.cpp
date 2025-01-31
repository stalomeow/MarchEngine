#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxSettings.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO GfxSettings_GetUseReversedZBuffer()
{
    retcs GfxSettings::UseReversedZBuffer;
}

NATIVE_EXPORT_AUTO GfxSettings_GetColorSpace()
{
    retcs GfxSettings::ColorSpace;
}
