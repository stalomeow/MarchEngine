#pragma once

#include "DotNetTypeTraits.h"
#include "DotNetMarshal.h"
#include "StringUtility.h"
#include "Debug.h"
#include <DirectXMath.h>
#include <stdint.h>

namespace march
{
    extern cs_t_convert g_cs_convert;
}

#define NATIVE_EXPORT_AUTO extern "C" __declspec(dllexport) auto __stdcall
#define retcs return ::march::g_cs_convert <<

using namespace std;
using namespace DirectX;
using namespace march;
using namespace march::StringUtility;
