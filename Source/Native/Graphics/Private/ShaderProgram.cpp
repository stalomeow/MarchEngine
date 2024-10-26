#include "Shader.h"
#include "GfxExcept.h"
#include "GfxSettings.h"
#include "GfxHelpers.h"
#include "StringUtility.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <d3d12shader.h> // Shader reflection

using namespace Microsoft::WRL;

namespace march
{
    ShaderProgram::ShaderProgram()
        : m_Hash{}
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

        ShaderConfig() : Entrypoints{}
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
        }

        return config;
    }

    bool ShaderPass::Compile(const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error)
    {
        // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

        IDxcUtils* utils = Shader::GetDxcUtils();
        IDxcCompiler3* compiler = Shader::GetDxcCompiler();

        //
        // Create default include handler. (You can create your own...)
        //
        ComPtr<IDxcIncludeHandler> pIncludeHandler;
        GFX_HR(utils->CreateDefaultIncludeHandler(&pIncludeHandler));

        ShaderConfig config = GetShaderConfig(source); // 预处理
        std::wstring wFilename = StringUtility::Utf8ToUtf16(filename);
        std::wstring wIncludePath = StringUtility::Utf8ToUtf16(Shader::GetEngineShaderPathUnixStyle());

        DxcBuffer Source = {};
        Source.Ptr = source.data();
        Source.Size = static_cast<SIZE_T>(source.size());
        Source.Encoding = DXC_CP_UTF8;

        for (int32_t i = 0; i < static_cast<int32_t>(ShaderProgramType::NumTypes); i++)
        {
            if (config.Entrypoints[i].empty())
            {
                m_Programs[i].reset();
                continue;
            }

            ShaderProgramType type = static_cast<ShaderProgramType>(i);
            std::wstring wEntrypoint = StringUtility::Utf8ToUtf16(config.Entrypoints[i]);
            std::wstring wTargetProfile = StringUtility::Utf8ToUtf16(GetTargetProfile(config.ShaderModel, type));

            std::vector<LPCWSTR> pszArgs =
            {
                wFilename.c_str(),             // Optional shader source file name for error reporting and for PIX shader source view.
                L"-E", wEntrypoint.c_str(),    // Entry point.
                L"-T", wTargetProfile.c_str(), // Target.
                L"-I", wIncludePath.c_str(),   // Include directory.
                L"-Zpc",                       // Pack matrices in column-major order.
                L"-Zsb",                       // Compute Shader Hash considering only output binary
                L"-Ges",                       // Enable strict mode
                L"-O3",                        // Optimization Level 3 (Default)
            };

            if (config.EnableDebugInfo)
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

            for (const auto& d : dynamicDefines)
            {
                pszArgs.push_back(L"-D");
                pszArgs.push_back(d.c_str());
            }

            //
            // Compile it with specified arguments.
            //
            ComPtr<IDxcResult> pResults = nullptr;
            GFX_HR(compiler->Compile(
                &Source,                             // Source buffer.
                pszArgs.data(),                      // Array of pointers to arguments.
                static_cast<UINT32>(pszArgs.size()), // Number of arguments.
                pIncludeHandler.Get(),               // User-provided interface to handle #include directives (optional).
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

            m_Programs[i] = std::make_unique<ShaderProgram>();
            ShaderProgram* program = m_Programs[i].get();

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
                utils->CreateReflection(&ReflectionData, IID_PPV_ARGS(&pReflection));

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
}
