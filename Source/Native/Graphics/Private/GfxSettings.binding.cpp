#include "GfxSettings.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO GfxSettings_GetUseReversedZBuffer()
{
    retcs GfxSettings::UseReversedZBuffer;
}

NATIVE_EXPORT_AUTO GfxSettings_GetColorSpace()
{
    retcs GfxSettings::ColorSpace;
}
