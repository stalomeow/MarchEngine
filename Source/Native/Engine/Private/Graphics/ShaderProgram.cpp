#include "pch.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxSettings.h"
#include "Engine/Graphics/GfxUtils.h"
#include "Engine/StringUtils.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <unordered_set>
#include <functional>

using namespace Microsoft::WRL;

namespace march
{
    void ShaderProgramHash::SetData(const DxcShaderHash& hash)
    {
        std::copy_n(hash.HashDigest, std::size(hash.HashDigest), Data);
    }

    bool ShaderProgramHash::operator==(const ShaderProgramHash& other) const
    {
        return memcmp(Data, other.Data, sizeof(Data)) == 0;
    }

    bool ShaderProgramHash::operator!=(const ShaderProgramHash& other) const
    {
        return memcmp(Data, other.Data, sizeof(Data)) != 0;
    }

    ShaderProgram::ShaderProgram()
        : m_Hash{}
        , m_Keywords{}
        , m_Binary(nullptr)
        , m_SrvCbvBuffers{}
        , m_SrvTextures{}
        , m_UavBuffers{}
        , m_UavTextures{}
        , m_StaticSamplers{}
        , m_ThreadGroupSizeX{}
        , m_ThreadGroupSizeY{}
        , m_ThreadGroupSizeZ{}
    {
    }

    D3D12_SHADER_VISIBILITY ShaderPass::GetShaderVisibility(size_t programType)
    {
        switch (static_cast<ShaderProgramType>(programType))
        {
        case ShaderProgramType::Vertex:   return D3D12_SHADER_VISIBILITY_VERTEX;
        case ShaderProgramType::Pixel:    return D3D12_SHADER_VISIBILITY_PIXEL;
        case ShaderProgramType::Domain:   return D3D12_SHADER_VISIBILITY_DOMAIN;
        case ShaderProgramType::Hull:     return D3D12_SHADER_VISIBILITY_HULL;
        case ShaderProgramType::Geometry: return D3D12_SHADER_VISIBILITY_GEOMETRY;
        default:                          throw GfxException("Unknown shader program type");
        }
    }

    bool ShaderPass::GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType)
    {
        if (key == "vs") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Vertex); return true; }
        if (key == "ps") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Pixel); return true; }
        if (key == "ds") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Domain); return true; }
        if (key == "hs") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Hull); return true; }
        if (key == "gs") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Geometry); return true; }
        return false;
    }

    std::string ShaderPass::GetTargetProfile(const std::string& shaderModel, size_t programType)
    {
        std::string model = shaderModel;
        std::replace(model.begin(), model.end(), '.', '_');

        std::string program;
        switch (static_cast<ShaderProgramType>(programType))
        {
        case ShaderProgramType::Vertex:   program = "vs"; break;
        case ShaderProgramType::Pixel:    program = "ps"; break;
        case ShaderProgramType::Domain:   program = "ds"; break;
        case ShaderProgramType::Hull:     program = "hs"; break;
        case ShaderProgramType::Geometry: program = "gs"; break;
        default:                          program = "unknown"; break;
        }

        return program + "_" + model;
    }

    void ShaderPass::RecordConstantBufferCallback(ID3D12ShaderReflectionConstantBuffer* cbuffer)
    {
        D3D12_SHADER_BUFFER_DESC bufferDesc{};
        GFX_HR(cbuffer->GetDesc(&bufferDesc));

        // 记录 material 的 shader property location
        if (Shader::GetNameId(bufferDesc.Name) == Shader::GetMaterialConstantBufferId())
        {
            for (UINT i = 0; i < bufferDesc.Variables; i++)
            {
                ID3D12ShaderReflectionVariable* var = cbuffer->GetVariableByIndex(i);
                D3D12_SHADER_VARIABLE_DESC varDesc{};
                GFX_HR(var->GetDesc(&varDesc));

                ShaderPropertyLocation& loc = m_PropertyLocations[Shader::GetNameId(varDesc.Name)];
                loc.Offset = static_cast<uint32_t>(varDesc.StartOffset);
                loc.Size = static_cast<uint32_t>(varDesc.Size);
            }
        }
    }

    D3D12_SHADER_VISIBILITY ComputeShaderKernel::GetShaderVisibility(size_t programType)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_shader_visibility
        // The compute queue always uses D3D12_SHADER_VISIBILITY_ALL because it has only one active stage.
        // The 3D queue can choose values, but if it uses D3D12_SHADER_VISIBILITY_ALL,
        // all shader stages can access whatever is bound at the root signature slot.
        return D3D12_SHADER_VISIBILITY_ALL;
    }

    bool ComputeShaderKernel::GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType)
    {
        return false;
    }

    std::string ComputeShaderKernel::GetTargetProfile(const std::string& shaderModel, size_t programType)
    {
        std::string model = shaderModel;
        std::replace(model.begin(), model.end(), '.', '_');
        return "cs_" + model;
    }

    void ComputeShaderKernel::RecordEntrypointCallback(size_t programType, std::string& entrypoint)
    {
        entrypoint = GetName();
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

    static bool EnumeratePragmas(const std::string& source, const std::function<bool(const std::vector<std::string>&)>& fn)
    {
        std::regex re(R"(^\s*#\s*pragma\s+(.*))", std::regex::ECMAScript);
        auto itEnd = std::sregex_iterator();

        std::vector<std::string> args{};

        for (auto it = std::sregex_iterator(source.begin(), source.end(), re); it != itEnd; ++it)
        {
            args.clear();
            std::istringstream iss((*it)[1]);

            std::string temp{};
            while (iss >> temp)
            {
                args.emplace_back(std::move(temp));
            }

            if (!args.empty() && !fn(args))
            {
                return false;
            }
        }

        return true;
    }

    template <size_t NumProgramTypes>
    struct ShaderConfig
    {
        std::string ShaderModel;
        bool EnableDebugInfo;
        std::string Entrypoints[NumProgramTypes];
        std::vector<std::vector<std::string>> MultiCompile;
        ShaderKeywordSpace TempMultiCompileKeywordSpace; // MultiCompile 编译时使用的临时 KeywordSpace

        ShaderConfig()
            : ShaderModel("6.0")
            , EnableDebugInfo(false)
            , Entrypoints{}
            , MultiCompile{}
            , TempMultiCompileKeywordSpace{}
        {
        }
    };

    template <size_t NumProgramTypes>
    struct ShaderCompilationContext
    {
        IDxcUtils* Utils;
        IDxcCompiler3* Compiler;
        IDxcIncludeHandler* IncludeHandler;

        ShaderConfig<NumProgramTypes> Config;
        std::wstring FileName;
        std::wstring IncludePath;
        DxcBuffer Source;

        ShaderKeywordSpace& KeywordSpace; // Shader 中保存的 KeywordSpace
        std::unordered_set<ShaderKeywordSet::DataType> CompiledKeywordSets;
        std::vector<std::string> Keywords;
        std::vector<std::string>& Warnings;
        std::string& Error;

        ShaderCompilationContext(ShaderKeywordSpace& keywordSpace, std::vector<std::string>& warnings, std::string& error)
            : Utils(nullptr)
            , Compiler(nullptr)
            , IncludeHandler(nullptr)
            , Config{}
            , FileName{}
            , IncludePath{}
            , Source{}
            , KeywordSpace(keywordSpace)
            , CompiledKeywordSets{}
            , Keywords{}
            , Warnings(warnings)
            , Error(error)
        {
        }

        bool ShouldCompileKeywords()
        {
            ShaderKeywordSet keywordSet{};

            for (const std::string& kw : Keywords)
            {
                if (!kw.empty())
                {
                    keywordSet.EnableKeyword(Config.TempMultiCompileKeywordSpace, kw);
                }
            }

            // 如果已经编译过了，就不再编译
            return CompiledKeywordSets.insert(keywordSet.GetData()).second;
        }
    };

    struct ShaderProgramUtils
    {
        template <size_t NumProgramTypes>
        static bool PreprocessAndGetShaderConfig(
            ShaderProgramGroup<NumProgramTypes>& programGroup,
            const std::string& source,
            ShaderConfig<NumProgramTypes>& config,
            std::string& error)
        {
            return EnumeratePragmas(source, [&](const std::vector<std::string>& args) -> bool
            {
                if (args.size() > 1 && args[0] == "multi_compile")
                {
                    std::unordered_set<std::string> uniqueKeywords{};

                    for (size_t i = 1; i < args.size(); i++)
                    {
                        // _ 表示没有 Keyword，替换为空字符串
                        if (std::all_of(args[i].begin(), args[i].end(), [](char c) { return c == '_'; }))
                        {
                            uniqueKeywords.insert("");
                        }
                        else
                        {
                            ShaderKeywordSpace::AddKeywordResult result = config.TempMultiCompileKeywordSpace.AddKeyword(args[i]);

                            if (result == ShaderKeywordSpace::AddKeywordResult::OutOfSpace)
                            {
                                error = "Too many keywords!";
                                return false;
                            }

                            uniqueKeywords.insert(args[i]);
                        }
                    }

                    config.MultiCompile.emplace_back(uniqueKeywords.begin(), uniqueKeywords.end());
                }
                else if (args.size() == 1)
                {
                    if (args[0] == "enable_debug_information")
                    {
                        config.EnableDebugInfo = true;
                    }
                }
                else if (args.size() == 2)
                {
                    if (args[0] == "target")
                    {
                        config.ShaderModel = args[1];
                    }
                    else if (size_t epIndex = 0; programGroup.GetEntrypointProgramType(args[0], &epIndex))
                    {
                        config.Entrypoints[epIndex] = args[1];
                    }
                }

                return true;
            });
        }

        template <size_t NumProgramTypes>
        static bool CompileRecursive(
            ShaderProgramGroup<NumProgramTypes>& programGroup,
            ShaderCompilationContext<NumProgramTypes>& context)
        {
            if (context.Keywords.size() < context.Config.MultiCompile.size())
            {
                const std::vector<std::string>& candidates = context.Config.MultiCompile[context.Keywords.size()];

                for (size_t i = 0; i < candidates.size(); i++)
                {
                    context.Keywords.push_back(candidates[i]);
                    bool success = CompileRecursive(programGroup, context);
                    context.Keywords.pop_back();

                    if (!success)
                    {
                        return false;
                    }
                }

                return true;
            }

            if (!context.ShouldCompileKeywords())
            {
                return true;
            }

            // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                if (context.Config.Entrypoints[i].empty())
                {
                    continue;
                }

                std::wstring wEntrypoint = StringUtils::Utf8ToUtf16(context.Config.Entrypoints[i]);
                std::wstring wTargetProfile = StringUtils::Utf8ToUtf16(programGroup.GetTargetProfile(context.Config.ShaderModel, i));

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

                if constexpr (GfxSettings::UseReversedZBuffer)
                {
                    pszArgs.push_back(L"-D");
                    pszArgs.push_back(L"MARCH_REVERSED_Z=1");
                }

                if constexpr (GfxSettings::ColorSpace == GfxColorSpace::Gamma)
                {
                    pszArgs.push_back(L"-D");
                    pszArgs.push_back(L"MARCH_COLORSPACE_GAMMA=1");
                }

                std::vector<std::wstring> dynamicDefines =
                {
                    L"MARCH_NEAR_CLIP_VALUE=" + std::to_wstring(GfxUtils::NearClipPlaneDepth),
                    L"MARCH_FAR_CLIP_VALUE=" + std::to_wstring(GfxUtils::FarClipPlaneDepth),
                };

                for (const std::string& kw : context.Keywords)
                {
                    if (!kw.empty())
                    {
                        dynamicDefines.push_back(StringUtils::Utf8ToUtf16(kw) + L"=1");
                    }
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
                        context.Error = pErrors->GetStringPointer();
                    }
                    else
                    {
                        context.Warnings.push_back(pErrors->GetStringPointer());
                    }
                }

                if (failed)
                {
                    return false;
                }

                ShaderProgram* program = programGroup.m_Programs[i].emplace_back(std::make_unique<ShaderProgram>()).get();

                // 保存 Keyword
                for (const std::string& kw : context.Keywords)
                {
                    if (!kw.empty())
                    {
                        context.KeywordSpace.AddKeyword(kw);
                        program->m_Keywords.EnableKeyword(context.KeywordSpace, kw);
                    }
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
                    program->m_Hash.SetData(*static_cast<DxcShaderHash*>(hash->GetBufferPointer()));
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

                    UINT threadGroupSize[3]{};
                    pReflection->GetThreadGroupSize(&threadGroupSize[0], &threadGroupSize[1], &threadGroupSize[2]);
                    program->m_ThreadGroupSizeX = static_cast<uint32_t>(threadGroupSize[0]);
                    program->m_ThreadGroupSizeY = static_cast<uint32_t>(threadGroupSize[1]);
                    program->m_ThreadGroupSizeZ = static_cast<uint32_t>(threadGroupSize[2]);

                    D3D12_SHADER_DESC shaderDesc = {};
                    GFX_HR(pReflection->GetDesc(&shaderDesc));

                    // 记录所有资源
                    for (UINT i = 0; i < shaderDesc.BoundResources; i++)
                    {
                        D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
                        GFX_HR(pReflection->GetResourceBindingDesc(i, &bindDesc));

                        // TODO rt acceleration structure
                        // TODO uav readback texture

                        switch (bindDesc.Type)
                        {
                        case D3D_SIT_CBUFFER:
                        {
                            ID3D12ShaderReflectionConstantBuffer* cb = pReflection->GetConstantBufferByName(bindDesc.Name);
                            D3D12_SHADER_BUFFER_DESC cbDesc{};
                            GFX_HR(cb->GetDesc(&cbDesc));

                            ShaderBuffer& buffer = program->m_SrvCbvBuffers.emplace_back();
                            buffer.Id = Shader::GetNameId(bindDesc.Name);
                            buffer.ShaderRegister = static_cast<uint32_t>(bindDesc.BindPoint);
                            buffer.RegisterSpace = static_cast<uint32_t>(bindDesc.Space);
                            buffer.ConstantBufferSize = static_cast<uint32_t>(cbDesc.Size);
                            programGroup.RecordConstantBufferCallback(cb);
                            break;
                        }

                        // TODO: tbuffer 怎么用
                        // TODO: 如何绑定 Buffer<T>；Buffer<T> 算是 tbuffer 吗
                        // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-object-buffer
                        case D3D_SIT_TBUFFER:
                        case D3D_SIT_STRUCTURED:
                        case D3D_SIT_BYTEADDRESS:
                        {
                            ShaderBuffer& buffer = program->m_SrvCbvBuffers.emplace_back();
                            buffer.Id = Shader::GetNameId(bindDesc.Name);
                            buffer.ShaderRegister = static_cast<uint32_t>(bindDesc.BindPoint);
                            buffer.RegisterSpace = static_cast<uint32_t>(bindDesc.Space);
                            buffer.ConstantBufferSize = 0;
                            break;
                        }

                        case D3D_SIT_UAV_RWSTRUCTURED:
                        case D3D_SIT_UAV_RWBYTEADDRESS:
                        case D3D_SIT_UAV_APPEND_STRUCTURED:
                        case D3D_SIT_UAV_CONSUME_STRUCTURED:
                        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                        {
                            ShaderBuffer& buffer = program->m_UavBuffers.emplace_back();
                            buffer.Id = Shader::GetNameId(bindDesc.Name);
                            buffer.ShaderRegister = static_cast<uint32_t>(bindDesc.BindPoint);
                            buffer.RegisterSpace = static_cast<uint32_t>(bindDesc.Space);
                            buffer.ConstantBufferSize = 0;
                            break;
                        }

                        case D3D_SIT_TEXTURE:
                        {
                            ShaderTexture& tex = program->m_SrvTextures.emplace_back();
                            tex.Id = Shader::GetNameId(bindDesc.Name);
                            tex.ShaderRegisterTexture = static_cast<uint32_t>(bindDesc.BindPoint);
                            tex.RegisterSpaceTexture = static_cast<uint32_t>(bindDesc.Space);
                            tex.HasSampler = false; // 先假设没有 sampler
                            tex.ShaderRegisterSampler = 0;
                            tex.RegisterSpaceSampler = 0;
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

                        // https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_shader_input_type
                        // The shader resource is a read-and-write buffer or texture.
                        case D3D_SIT_UAV_RWTYPED:
                        {
                            bool isTexture;

                            switch (bindDesc.Dimension)
                            {
                            case D3D_SRV_DIMENSION_TEXTURE1D:
                            case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                            case D3D_SRV_DIMENSION_TEXTURE2D:
                            case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                            case D3D_SRV_DIMENSION_TEXTURE2DMS:
                            case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
                            case D3D_SRV_DIMENSION_TEXTURE3D:
                            case D3D_SRV_DIMENSION_TEXTURECUBE:
                            case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                                isTexture = true;
                                break;
                            default:
                                isTexture = false;
                                break;
                            }

                            if (isTexture)
                            {
                                ShaderTexture& tex = program->m_UavTextures.emplace_back();
                                tex.Id = Shader::GetNameId(bindDesc.Name);
                                tex.ShaderRegisterTexture = static_cast<uint32_t>(bindDesc.BindPoint);
                                tex.RegisterSpaceTexture = static_cast<uint32_t>(bindDesc.Space);
                                tex.HasSampler = false; // uav 没有 sampler
                                tex.ShaderRegisterSampler = 0;
                                tex.RegisterSpaceSampler = 0;
                            }
                            else
                            {
                                ShaderBuffer& buffer = program->m_UavBuffers.emplace_back();
                                buffer.Id = Shader::GetNameId(bindDesc.Name);
                                buffer.ShaderRegister = static_cast<uint32_t>(bindDesc.BindPoint);
                                buffer.RegisterSpace = static_cast<uint32_t>(bindDesc.Space);
                                buffer.ConstantBufferSize = 0;
                            }

                            break;
                        }
                        }
                    }

                    // 记录 texture sampler
                    for (ShaderTexture& tex : program->m_SrvTextures)
                    {
                        int32_t samplerId = Shader::GetNameId("sampler" + Shader::GetIdName(tex.Id));

                        if (auto it = program->m_StaticSamplers.find(samplerId); it != program->m_StaticSamplers.end())
                        {
                            tex.HasSampler = true;
                            tex.ShaderRegisterSampler = it->second.ShaderRegister;
                            tex.RegisterSpaceSampler = it->second.RegisterSpace;
                            program->m_StaticSamplers.erase(it); // 移除假的 static sampler
                        }
                    }
                }
            }

            return true;
        }

        template <size_t NumProgramTypes>
        static bool Compile(
            ShaderProgramGroup<NumProgramTypes>& programGroup,
            ShaderKeywordSpace& keywordSpace,
            const std::string& filename,
            const std::string& source,
            std::vector<std::string>& warnings,
            std::string& error)
        {
            ShaderCompilationContext<NumProgramTypes> context{ keywordSpace, warnings, error };
            context.Utils = Shader::GetDxcUtils();
            context.Compiler = Shader::GetDxcCompiler();

            //
            // Create default include handler. (You can create your own...)
            //
            ComPtr<IDxcIncludeHandler> pIncludeHandler;
            GFX_HR(context.Utils->CreateDefaultIncludeHandler(&pIncludeHandler));
            context.IncludeHandler = pIncludeHandler.Get();

            if (!PreprocessAndGetShaderConfig(programGroup, source, context.Config, error))
            {
                return false;
            }

            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                programGroup.RecordEntrypointCallback(i, context.Config.Entrypoints[i]);
            }

            context.FileName = StringUtils::Utf8ToUtf16(filename);
            context.IncludePath = StringUtils::Utf8ToUtf16(Shader::GetEngineShaderPathUnixStyle());
            context.Source.Ptr = source.data();
            context.Source.Size = static_cast<SIZE_T>(source.size());
            context.Source.Encoding = DXC_CP_UTF8;

            return CompileRecursive(programGroup, context);
        }
    };

    bool ShaderPass::Compile(ShaderKeywordSpace& keywordSpace, const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error)
    {
        return ShaderProgramUtils::Compile(*this, keywordSpace, filename, source, warnings, error);
    }

    bool ComputeShader::Compile(const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error)
    {
        m_KeywordSpace.Clear();
        m_Kernels.clear();

        EnumeratePragmas(source, [this](const std::vector<std::string>& args) -> bool
        {
            if (args.size() > 1 && args[0] == "kernel")
            {
                ComputeShaderKernel* kernel = m_Kernels.emplace_back(std::make_unique<ComputeShaderKernel>()).get();
                kernel->m_Name = args[1];
            }

            return true;
        });

        for (std::unique_ptr<ComputeShaderKernel>& kernel : m_Kernels)
        {
            if (!ShaderProgramUtils::Compile(*kernel.get(), m_KeywordSpace, filename, source, warnings, error))
            {
                m_KeywordSpace.Clear();
                m_Kernels.clear();
                return false;
            }
        }

        return true;
    }
}
