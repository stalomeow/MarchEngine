#pragma once

#include "DotNetTypeTraits.h"
#include "DotNetMarshal.h"
#include "StringUtility.h"
#include <DirectXMath.h>
#include <stdint.h>

using namespace std;
using namespace DirectX;
using namespace march;
using namespace march::StringUtility;

#define NATIVE_EXPORT_AUTO extern "C" __declspec(dllexport) auto __stdcall
#define return_cs(expr) return ::march::to_cs(expr)
