#include "GfxSettings.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO GfxSettings_UseReversedZBuffer()
{
    retcs GfxSettings::UseReversedZBuffer();
}

NATIVE_EXPORT_AUTO GfxSettings_GetColorSpace()
{
    retcs GfxSettings::GetColorSpace();
}
