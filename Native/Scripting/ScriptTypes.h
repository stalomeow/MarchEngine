#pragma once

#include "Core/StringUtility.h"
#include <stdint.h>
#include <string>

#define CSHARP_API(returnType) returnType __stdcall

namespace dx12demo
{
    // https://learn.microsoft.com/en-us/dotnet/standard/native-interop/best-practices#common-windows-data-types

    typedef int32_t CSharpInt;
    typedef float CSharpFloat;
    typedef double CSharpDouble;

    static_assert(sizeof(wchar_t) == 2, "wchar_t is not 2 bytes wide.");
    typedef wchar_t CSharpChar;

    extern "C" typedef struct
    {
        CSharpChar* FixedData;
        CSharpInt Length;
    } CSharpString;

    inline std::string CSharpString_ToUtf8(const CSharpString& self)
    {
        return StringUtility::Utf16ToUtf8(self.FixedData, self.Length);
    }

    inline std::string CSharpString_ToAnsi(const CSharpString& self)
    {
        return StringUtility::Utf16ToAnsi(self.FixedData, self.Length);
    }
}
