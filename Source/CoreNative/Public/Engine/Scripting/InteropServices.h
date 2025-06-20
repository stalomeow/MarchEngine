#pragma once

#include "Engine/Scripting/DotNetTypeTraits.h"
#include "Engine/Scripting/DotNetMarshal.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Debug.h"
#include "Engine/Object.h"
#include <DirectXMath.h>
#include <stdint.h>

namespace march
{
    extern cs_t_convert g_cs_convert;
}

#define NATIVE_EXPORT_AUTO extern "C" __declspec(dllexport) auto __stdcall
#define retcs return ::march::g_cs_convert <<

using namespace DirectX;
using namespace march;
