#pragma once

#include "Engine/Ints.h"
#include <dxcapi.h>
#include <string>
#include <vector>

namespace march
{
    struct ShaderUtils
    {
        static int32 GetIdFromString(const std::string& str);
        static const std::string& GetStringFromId(int32 id);

        static IDxcUtils* GetDxcUtils();
        static IDxcCompiler3* GetDxcCompiler();

        static void ClearRootSignatureCache();

        static bool HasCachedShaderProgram(const std::vector<uint8_t>& hash);
        static void DeleteCachedShaderProgram(const std::vector<uint8_t>& hash);
    };
}
