#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/ShaderUtils.h"
#include "Engine/Misc/PathUtils.h"
#include "Engine/Misc/StringUtils.h"
#include <unordered_map>
#include <stdexcept>
#include <wrl.h>

using namespace Microsoft::WRL;

namespace march
{
    std::string ShaderUtils::GetEngineShaderPathUnixStyle()
    {
#ifdef ENGINE_SHADER_UNIX_PATH
        return ENGINE_SHADER_UNIX_PATH;
#endif

        return PathUtils::GetWorkingDirectoryUtf8(PathStyle::Unix) + "/Shaders";
    }

    static std::unordered_map<std::string, int32> g_StringIdMap{};
    static int32 g_NextStringId = 0;

    int32 ShaderUtils::GetIdFromString(const std::string& str)
    {
        auto p = g_StringIdMap.try_emplace(str, g_NextStringId);
        if (p.second) ++g_NextStringId;
        return p.first->second;
    }

    const std::string& ShaderUtils::GetStringFromId(int32 id)
    {
        if (id < g_NextStringId)
        {
            for (const auto& p : g_StringIdMap)
            {
                if (p.second == id)
                {
                    return p.first;
                }
            }
        }

        throw std::invalid_argument(StringUtils::Format("Invalid string id: {}", id));
    }

    static ComPtr<IDxcUtils> g_Utils = nullptr;
    static ComPtr<IDxcCompiler3> g_Compiler = nullptr;

    IDxcUtils* ShaderUtils::GetDxcUtils()
    {
        if (g_Utils == nullptr)
        {
            CHECK_HR(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&g_Utils)));
        }

        return g_Utils.Get();
    }

    IDxcCompiler3* ShaderUtils::GetDxcCompiler()
    {
        if (g_Compiler == nullptr)
        {
            CHECK_HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&g_Compiler)));
        }

        return g_Compiler.Get();
    }
}
