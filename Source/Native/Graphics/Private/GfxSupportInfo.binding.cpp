#include "GfxSupportInfo.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO GfxSupportInfo_UseReversedZBuffer()
{
    retcs GfxSupportInfo::UseReversedZBuffer();
}
