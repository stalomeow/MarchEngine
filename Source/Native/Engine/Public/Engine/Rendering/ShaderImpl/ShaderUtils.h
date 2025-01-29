#pragma once

#include "Engine/Ints.h"
#include <string>
#include <dxcapi.h>

namespace march
{
    struct ShaderUtils
    {
        static std::string GetEngineShaderPathUnixStyle();

        static int32 GetIdFromString(const std::string& str);
        static const std::string& GetStringFromId(int32 id);

        static IDxcUtils* GetDxcUtils();
        static IDxcCompiler3* GetDxcCompiler();

        static void ClearRootSignatureCache();
    };
}
