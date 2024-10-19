#include "GfxSupportInfo.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO GfxSupportInfo_UseReversedZBuffer()
{
    retcs GfxSupportInfo::UseReversedZBuffer();
}

NATIVE_EXPORT_AUTO GfxSupportInfo_GetColorSpace()
{
    retcs GfxSupportInfo::GetColorSpace();
}
