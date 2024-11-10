#include "Shader.h"
#include "GfxExcept.h"
#include "GfxSettings.h"
#include "GfxHelpers.h"
#include "StringUtility.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <limits>
#include <unordered_set>
#include <d3d12shader.h> // Shader reflection

#undef min
#undef max

using namespace Microsoft::WRL;

namespace march
{
    ShaderProgram::ShaderProgram()
        : m_Hash{}
        , m_Keywords{}
        , m_Binary(nullptr)
        , m_ConstantBuffers{}
        , m_StaticSamplers{}
        , m_Textures{}
        , m_SrvUavRootParameterIndex(0)
        , m_SamplerRootParameterIndex(0)
    {
    }

    const ShaderProgram::HashType& ShaderProgram::GetHash() const
    {
        return m_Hash;
    }

    const ShaderKeywordSet& ShaderProgram::GetKeywords() const
    {
        return m_Keywords;
    }

    uint8_t* ShaderProgram::GetBinaryData() const
    {
        return reinterpret_cast<uint8_t*>(m_Binary->GetBufferPointer());
    }

    uint64_t ShaderProgram::GetBinarySize() const
    {
        return static_cast<uint64_t>(m_Binary->GetBufferSize());
    }

    const std::unordered_map<int32_t, ShaderConstantBuffer>& ShaderProgram::GetConstantBuffers() const
    {
        return m_ConstantBuffers;
    }

    const std::unordered_map<int32_t, ShaderStaticSampler>& ShaderProgram::GetStaticSamplers() const
    {
        return m_StaticSamplers;
    }

    const std::unordered_map<int32_t, ShaderTexture>& ShaderProgram::GetTextures() const
    {
        return m_Textures;
    }

    uint32_t ShaderProgram::GetSrvUavRootParameterIndex() const
    {
        return m_SrvUavRootParameterIndex;
    }

    uint32_t ShaderProgram::GetSamplerRootParameterIndex() const
    {
        return m_SamplerRootParameterIndex;
    }

    static size_t absdiff(size_t a, size_t b)
    {
        return a > b ? a - b : b - a;
    }

    ShaderProgram* ShaderPass::GetProgram(ShaderProgramType type, const ShaderKeywordSet& keywords) const
    {
        const std::vector<std::unique_ptr<ShaderProgram>>& programs = m_Programs[static_cast<int32_t>(type)];

        ShaderProgram* result = nullptr;
        size_t minDiff = std::numeric_limits<size_t>::max();
        size_t targetKeywordCount = keywords.GetEnabledKeywordCount();

        for (const std::unique_ptr<ShaderProgram>& program : programs)
        {
            const ShaderKeywordSet& ks = program->GetKeywords();
            size_t matchingCount = ks.GetMatchingKeywordCount(keywords);
            size_t enabledCount = ks.GetEnabledKeywordCount();

            // 没 match 的数量 + 多余的数量
            size_t diff = absdiff(targetKeywordCount, matchingCount) + absdiff(enabledCount, matchingCount);

            if (diff < minDiff)
            {
                result = program.get();
                minDiff = diff;
            }
        }

        return result;
    }

    ShaderProgram* ShaderPass::GetProgram(ShaderProgramType type, int32_t index) const
    {
        return m_Programs[static_cast<size_t>(type)][static_cast<size_t>(index)].get();
    }

    int32_t ShaderPass::GetProgramCount(ShaderProgramType type) const
    {
        return static_cast<int32_t>(m_Programs[static_cast<size_t>(type)].size());
    }

    static ComPtr<IDxcUtils> g_Utils = nullptr;
    static ComPtr<IDxcCompiler3> g_Compiler = nullptr;

    IDxcUtils* Shader::GetDxcUtils()
    {
        if (g_Utils == nullptr)
        {
            GFX_HR(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&g_Utils)));
        }

        return g_Utils.Get();
    }

    IDxcCompiler3* Shader::GetDxcCompiler()
    {
        if (g_Compiler == nullptr)
        {
            GFX_HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&g_Compiler)));
        }

        return g_Compiler.Get();
    }

    static std::string GetTargetProfile(const std::string& shaderModel, ShaderProgramType programType)
    {
        std::string model = shaderModel;
        std::replace(model.begin(), model.end(), '.', '_');

        std::string program;

        switch (programType)
        {
        case ShaderProgramType::Vertex:
            program = "vs";
            break;

        case ShaderProgramType::Pixel:
            program = "ps";
            break;

        default:
            program = "unknown";
            break;
        }

        return program + "_" + model;
    }

    struct ShaderConfig
    {
        std::string ShaderModel;
        bool EnableDebugInfo;
        std::string Entrypoints[static_cast<int32_t>(ShaderProgramType::NumTypes)];
        std::vector<std::vector<std::string>> MultiCompile;

        ShaderConfig() : Entrypoints{}, MultiCompile{}
        {
            ShaderModel = "6.0";
            EnableDebugInfo = false;
        }
    };

    static ShaderConfig GetShaderConfig(const std::string& source)
    {
        ShaderConfig config{};

        std::regex re(R"(^\s*#\s*pragma\s+(.*))", std::regex::ECMAScript);
        auto itEnd = std::sregex_iterator();

        for (auto it = std::sregex_iterator(source.begin(), source.end(), re); it != itEnd; ++it)
        {
            std::string temp{};
            std::vector<std::string> args{};
            std::istringstream iss((*it)[1]);

            while (iss >> temp)
            {
                args.emplace_back(std::move(temp));
            }

            if (args.empty())
            {
                continue;
            }

            if (args[0] == "target" && args.size() == 2)
            {
                config.ShaderModel = args[1];
            }
            else if (args[0] == "vs" && args.size() == 2)
            {
                config.Entrypoints[static_cast<int32_t>(ShaderProgramType::Vertex)] = args[1];
            }
            else if (args[0] == "ps" && args.size() == 2)
            {
                config.Entrypoints[static_cast<int32_t>(ShaderProgramType::Pixel)] = args[1];
            }
            else if (args[0] == "enable_debug_information" && args.size() == 1)
            {
                config.EnableDebugInfo = true;
            }
            else if (args[0] == "multi_compile" && args.size() > 1)
            {
                config.MultiCompile.emplace_back(args.begin() + 1, args.end());
            }
        }

        return config;
    }

    struct ShaderCompilationContext
    {
        IDxcUtils* Utils;
        IDxcCompiler3* Compiler;
        IDxcIncludeHandler* IncludeHandler;

        ShaderConfig Config;
        std::wstring FileName;
        std::wstring IncludePath;
        DxcBuffer Source;
    };

    static bool IsEmptyKeyword(const std::string& keyword)
    {
        // _ 表示没有 Keyword
        return keyword.empty() || std::all_of(keyword.begin(), keyword.end(), [](char c) { return c == '_'; });
    }

    bool ShaderPass::CompileRecursive(const ShaderCompilationContext& context, std::vector<std::string>& keywords, std::vector<std::string>& warnings, std::string& error)
    {
        if (keywords.size() < context.Config.MultiCompile.size())
        {
            const std::vector<std::string>& candidates = context.Config.MultiCompile[keywords.size()];

            for (size_t i = 0; i < candidates.size(); i++)
            {
                keywords.push_back(candidates[i]);
                bool success = CompileRecursive(context, keywords, warnings, error);
                keywords.pop_back();

                if (!success)
                {
                    return false;
                }
            }

            return true;
        }

        // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

        for (int32_t i = 0; i < static_cast<int32_t>(ShaderProgramType::NumTypes); i++)
        {
            if (context.Config.Entrypoints[i].empty())
            {
                continue;
            }

            ShaderProgramType type = static_cast<ShaderProgramType>(i);
            std::wstring wEntrypoint = StringUtility::Utf8ToUtf16(context.Config.Entrypoints[i]);
            std::wstring wTargetProfile = StringUtility::Utf8ToUtf16(GetTargetProfile(context.Config.ShaderModel, type));

            std::vector<LPCWSTR> pszArgs =
            {
                context.FileName.c_str(),           // Optional shader source file name for error reporting and for PIX shader source view.
                L"-E", wEntrypoint.c_str(),         // Entry point.
                L"-T", wTargetProfile.c_str(),      // Target.
                L"-I", context.IncludePath.c_str(), // Include directory.
                L"-Zpc",                            // Pack matrices in column-major order.
                L"-Zsb",                            // Compute Shader Hash considering only output binary
                L"-Ges",                            // Enable strict mode
                L"-O3",                             // Optimization Level 3 (Default)
            };

            if (context.Config.EnableDebugInfo)
            {
                pszArgs.push_back(L"-Zi"); // Enable debug information.
            }
            else
            {
                pszArgs.push_back(L"-Qstrip_debug");         // Strip debug information from 4_0+ shader bytecode
                pszArgs.push_back(L"-Qstrip_priv");          // Strip private data from shader bytecode
                pszArgs.push_back(L"-Qstrip_reflect");       // Strip reflection data from shader bytecode
                pszArgs.push_back(L"-Qstrip_rootsignature"); // Strip root signature data from shader bytecode
            }

            if constexpr (GfxSettings::UseReversedZBuffer())
            {
                pszArgs.push_back(L"-D");
                pszArgs.push_back(L"MARCH_REVERSED_Z=1");
            }

            if constexpr (GfxSettings::GetColorSpace() == GfxColorSpace::Gamma)
            {
                pszArgs.push_back(L"-D");
                pszArgs.push_back(L"MARCH_COLORSPACE_GAMMA=1");
            }

            std::vector<std::wstring> dynamicDefines =
            {
                L"MARCH_NEAR_CLIP_VALUE=" + std::to_wstring(GfxHelpers::GetNearClipPlaneDepth()),
                L"MARCH_FAR_CLIP_VALUE=" + std::to_wstring(GfxHelpers::GetFarClipPlaneDepth()),
            };

            std::unordered_set<std::string> uniqueKeywords{};

            for (const std::string& kw : keywords)
            {
                if (IsEmptyKeyword(kw) || !uniqueKeywords.insert(kw).second)
                {
                    continue;
                }

                dynamicDefines.push_back(StringUtility::Utf8ToUtf16(kw) + L"=1");
            }

            for (const auto& d : dynamicDefines)
            {
                pszArgs.push_back(L"-D");
                pszArgs.push_back(d.c_str());
            }

            //
            // Compile it with specified arguments.
            //
            ComPtr<IDxcResult> pResults = nullptr;
            GFX_HR(context.Compiler->Compile(
                &context.Source,                     // Source buffer.
                pszArgs.data(),                      // Array of pointers to arguments.
                static_cast<UINT32>(pszArgs.size()), // Number of arguments.
                context.IncludeHandler,              // User-provided interface to handle #include directives (optional).
                IID_PPV_ARGS(&pResults)              // Compiler output status, buffer, and errors.
            ));

            HRESULT hrStatus;
            GFX_HR(pResults->GetStatus(&hrStatus));
            bool failed = FAILED(hrStatus);

            ComPtr<IDxcBlobUtf8> pErrors = nullptr;
            GFX_HR(pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));

            // Note that d3dcompiler would return null if no errors or warnings are present.
            // IDxcCompiler3::Compile will always return an error buffer, but its length
            // will be zero if there are no warnings or errors.
            if (pErrors != nullptr && pErrors->GetStringLength() != 0)
            {
                if (failed)
                {
                    error = pErrors->GetStringPointer();
                }
                else
                {
                    warnings.push_back(pErrors->GetStringPointer());
                }
            }

            if (failed)
            {
                return false;
            }

            m_Programs[i].emplace_back(std::make_unique<ShaderProgram>());
            ShaderProgram* program = m_Programs[i].back().get();

            // 保存 Keyword
            for (const std::string& kw : uniqueKeywords)
            {
                m_Shader->m_KeywordSpace.AddKeyword(kw);
                program->m_Keywords.EnableKeyword(m_Shader->m_KeywordSpace, kw);
            }

            // 保存编译结果
            ComPtr<IDxcBlobUtf16> shaderName = nullptr;
            GFX_HR(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&program->m_Binary), &shaderName));

            // 暂时不输出 PDB 文件

            // 保存 Hash
            ComPtr<IDxcBlob> hash = nullptr;
            pResults->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&hash), nullptr);
            if (hash != nullptr)
            {
                DxcShaderHash* buf = static_cast<DxcShaderHash*>(hash->GetBufferPointer());
                std::copy_n(buf->HashDigest, std::size(buf->HashDigest), program->m_Hash);
            }

            // 反射
            ComPtr<IDxcBlob> pReflectionData;
            GFX_HR(pResults->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&pReflectionData), nullptr));
            if (pReflectionData != nullptr)
            {
                // Create reflection interface.
                DxcBuffer ReflectionData = {};
                ReflectionData.Encoding = DXC_CP_ACP;
                ReflectionData.Ptr = pReflectionData->GetBufferPointer();
                ReflectionData.Size = pReflectionData->GetBufferSize();

                ComPtr<ID3D12ShaderReflection> pReflection;
                context.Utils->CreateReflection(&ReflectionData, IID_PPV_ARGS(&pReflection));

                // Use reflection interface here.

                D3D12_SHADER_DESC shaderDesc = {};
                GFX_HR(pReflection->GetDesc(&shaderDesc));

                for (UINT i = 0; i < shaderDesc.BoundResources; i++)
                {
                    D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
                    GFX_HR(pReflection->GetResourceBindingDesc(i, &bindDesc));

                    switch (bindDesc.Type)
                    {
                    case D3D_SIT_CBUFFER:
                    {
                        D3D12_SHADER_BUFFER_DESC cbDesc = {};
                        GFX_HR(pReflection->GetConstantBufferByName(bindDesc.Name)->GetDesc(&cbDesc));

                        ShaderConstantBuffer& cb = program->m_ConstantBuffers[Shader::GetNameId(bindDesc.Name)];
                        cb.ShaderRegister = bindDesc.BindPoint;
                        cb.RegisterSpace = bindDesc.Space;
                        cb.UnalignedSize = cbDesc.Size;
                        break;
                    }

                    case D3D_SIT_TEXTURE:
                    {
                        ShaderTexture& tex = program->m_Textures[Shader::GetNameId(bindDesc.Name)];
                        tex.ShaderRegisterTexture = bindDesc.BindPoint;
                        tex.RegisterSpaceTexture = bindDesc.Space;
                        break;
                    }

                    case D3D_SIT_SAMPLER:
                    {
                        // 先假设全是 static sampler
                        ShaderStaticSampler& sampler = program->m_StaticSamplers[Shader::GetNameId(bindDesc.Name)];
                        sampler.ShaderRegister = bindDesc.BindPoint;
                        sampler.RegisterSpace = bindDesc.Space;
                        break;
                    }
                    }
                }

                // 记录 shader property location
                if (program->m_ConstantBuffers.count(Shader::GetMaterialConstantBufferId()) > 0)
                {
                    const std::string& cbName = Shader::GetIdName(Shader::GetMaterialConstantBufferId());
                    ID3D12ShaderReflectionConstantBuffer* cbMat = pReflection->GetConstantBufferByName(cbName.c_str());

                    D3D12_SHADER_BUFFER_DESC cbMatDesc = {};
                    if (SUCCEEDED(cbMat->GetDesc(&cbMatDesc)))
                    {
                        for (UINT i = 0; i < cbMatDesc.Variables; i++)
                        {
                            ID3D12ShaderReflectionVariable* var = cbMat->GetVariableByIndex(i);
                            D3D12_SHADER_VARIABLE_DESC varDesc = {};
                            GFX_HR(var->GetDesc(&varDesc));

                            ShaderPropertyLocation& loc = m_PropertyLocations[Shader::GetNameId(varDesc.Name)];
                            loc.Offset = varDesc.StartOffset;
                            loc.Size = varDesc.Size;
                        }
                    }
                }

                // 记录 texture sampler
                for (std::pair<const int32_t, ShaderTexture>& kv : program->m_Textures)
                {
                    int32_t samplerId = Shader::GetNameId("sampler" + Shader::GetIdName(kv.first));

                    if (auto it = program->m_StaticSamplers.find(samplerId); it != program->m_StaticSamplers.end())
                    {
                        kv.second.HasSampler = true;
                        kv.second.ShaderRegisterSampler = it->second.ShaderRegister;
                        kv.second.RegisterSpaceSampler = it->second.RegisterSpace;
                        program->m_StaticSamplers.erase(it);
                    }
                }
            }
        }

        return true;
    }

    bool ShaderPass::Compile(const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error)
    {
        ShaderCompilationContext context{};
        context.Utils = Shader::GetDxcUtils();
        context.Compiler = Shader::GetDxcCompiler();

        //
        // Create default include handler. (You can create your own...)
        //
        ComPtr<IDxcIncludeHandler> pIncludeHandler;
        GFX_HR(context.Utils->CreateDefaultIncludeHandler(&pIncludeHandler));
        context.IncludeHandler = pIncludeHandler.Get();

        context.Config = GetShaderConfig(source); // 预处理
        context.FileName = StringUtility::Utf8ToUtf16(filename);
        context.IncludePath = StringUtility::Utf8ToUtf16(Shader::GetEngineShaderPathUnixStyle());
        context.Source.Ptr = source.data();
        context.Source.Size = static_cast<SIZE_T>(source.size());
        context.Source.Encoding = DXC_CP_UTF8;

        std::vector<std::string> keywords{};
        return CompileRecursive(context, keywords, warnings, error);
    }
}
