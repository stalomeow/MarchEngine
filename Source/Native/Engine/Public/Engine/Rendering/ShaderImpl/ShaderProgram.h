#pragma once

#include "Engine/Ints.h"
#include "Engine/HashUtils.h"
#include "Engine/StringUtils.h"
#include "Engine/Rendering/ShaderImpl/ShaderKeyword.h"
#include "Engine/Rendering/ShaderImpl/ShaderUtils.h"
#include "Engine/Graphics/GfxDevice.h"
#include <directx/d3dx12.h>
#include <d3d12shader.h> // Shader reflection
#include <dxcapi.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <limits>
#include <functional>

namespace march
{
    struct alignas(4) ShaderProgramHash
    {
        uint8_t Data[16];

        void SetData(const DxcShaderHash& hash) { std::copy_n(hash.HashDigest, std::size(hash.HashDigest), Data); }

        bool operator==(const ShaderProgramHash& other) const { return memcmp(Data, other.Data, sizeof(Data)) == 0; }

        bool operator!=(const ShaderProgramHash& other) const { return memcmp(Data, other.Data, sizeof(Data)) != 0; }
    };

    struct ShaderProgramBuffer
    {
        int32_t Id;
        uint32_t ShaderRegister;
        uint32_t RegisterSpace;
        uint32_t ConstantBufferSize; // 只有 Constant Buffer 有值，其他为 0
    };

    struct ShaderProgramTexture
    {
        int32_t Id;
        uint32_t ShaderRegisterTexture;
        uint32_t RegisterSpaceTexture;

        bool HasSampler;
        uint32_t ShaderRegisterSampler;
        uint32_t RegisterSpaceSampler;
    };

    struct ShaderProgramStaticSampler
    {
        int32_t Id;
        uint32_t ShaderRegister;
        uint32_t RegisterSpace;
    };

    class ShaderProgram final
    {
        template <size_t>
        friend class ShaderProgramGroup;

        friend struct ShaderCompilationInternalUtils;

        ShaderProgramHash m_Hash{};
        ShaderKeywordSet m_Keywords{};
        Microsoft::WRL::ComPtr<IDxcBlob> m_Binary = nullptr;

        std::vector<ShaderProgramBuffer> m_SrvCbvBuffers{};
        std::vector<ShaderProgramTexture> m_SrvTextures{};
        std::vector<ShaderProgramBuffer> m_UavBuffers{};
        std::vector<ShaderProgramTexture> m_UavTextures{};
        std::vector<ShaderProgramStaticSampler> m_StaticSamplers{};

        uint32_t m_ThreadGroupSizeX = 0;
        uint32_t m_ThreadGroupSizeY = 0;
        uint32_t m_ThreadGroupSizeZ = 0;

    public:
        const ShaderProgramHash& GetHash() const { return m_Hash; }
        const ShaderKeywordSet& GetKeywords() const { return m_Keywords; }
        const uint8_t* GetBinaryData() const { return reinterpret_cast<const uint8_t*>(m_Binary->GetBufferPointer()); }
        uint64_t GetBinarySize() const { return static_cast<uint64_t>(m_Binary->GetBufferSize()); }

        const std::vector<ShaderProgramBuffer>& GetSrvCbvBuffers() const { return m_SrvCbvBuffers; }
        const std::vector<ShaderProgramTexture>& GetSrvTextures() const { return m_SrvTextures; }
        const std::vector<ShaderProgramBuffer>& GetUavBuffers() const { return m_UavBuffers; }
        const std::vector<ShaderProgramTexture>& GetUavTextures() const { return m_UavTextures; }
        const std::vector<ShaderProgramStaticSampler>& GetStaticSamplers() const { return m_StaticSamplers; }

        void GetThreadGroupSize(uint32_t* pOutX, uint32_t* pOutY, uint32_t* pOutZ) const
        {
            *pOutX = m_ThreadGroupSizeX;
            *pOutY = m_ThreadGroupSizeY;
            *pOutZ = m_ThreadGroupSizeZ;
        }
    };

    // srv/cbv buffer 都使用 root srv/cbv
    struct ShaderParamSrvCbvBuffer
    {
        int32_t Id;
        uint32_t RootParameterIndex;
        bool IsConstantBuffer;
    };

    // srv texture 在 srv/uav table 中的位置和 sampler 在 sampler table 中的位置
    struct ShaderParamSrvTexture
    {
        int32_t Id;
        uint32_t DescriptorTableSlotTexture;
        std::optional<uint32_t> DescriptorTableSlotSampler;
    };

    // uav buffer 在 srv/uav table 中的位置
    struct ShaderParamUavBuffer
    {
        int32_t Id;
        uint32_t DescriptorTableSlot;
    };

    // uav texture 在 srv/uav table 中的位置
    struct ShaderParamUavTexture
    {
        int32_t Id;
        uint32_t DescriptorTableSlot;
    };

    template <size_t _NumProgramTypes>
    class ShaderRootSignature final
    {
        template <size_t>
        friend class ShaderProgramGroup;

    public:
        static constexpr size_t NumProgramTypes = _NumProgramTypes;

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

        struct
        {
            std::optional<uint32_t> SrvUavTableRootParamIndex = std::nullopt;
            std::optional<uint32_t> SamplerTableRootParamIndex = std::nullopt;

            std::vector<ShaderParamSrvCbvBuffer> SrvCbvBuffers{};
            std::vector<ShaderParamSrvTexture> SrvTextures{};
            std::vector<ShaderParamUavBuffer> UavBuffers{};
            std::vector<ShaderParamUavTexture> UavTextures{};
        } m_Params[NumProgramTypes]{};

        const auto& GetParam(size_t index) const
        {
            if (index >= NumProgramTypes)
            {
                throw std::out_of_range(StringUtils::Format("Program type '{}' is out of range", index));
            }

            return m_Params[index];
        }

    public:
        ID3D12RootSignature* GetD3DRootSignature() const { return m_RootSignature.Get(); }

        auto GetSrvUavTableRootParamIndex(size_t programType) const { return GetParam(programType).SrvUavTableRootParamIndex; }
        auto GetSamplerTableRootParamIndex(size_t programType) const { return GetParam(programType).SamplerTableRootParamIndex; }
        const auto& GetSrvCbvBuffers(size_t programType) const { return GetParam(programType).SrvCbvBuffers; }
        const auto& GetSrvTextures(size_t programType) const { return GetParam(programType).SrvTextures; }
        const auto& GetUavBuffers(size_t programType) const { return GetParam(programType).UavBuffers; }
        const auto& GetUavTextures(size_t programType) const { return GetParam(programType).UavTextures; }
    };

    template <size_t _NumProgramTypes>
    class ShaderProgramGroup
    {
    public:
        static constexpr size_t NumProgramTypes = _NumProgramTypes;
        using RootSignatureType = ShaderRootSignature<NumProgramTypes>;

    protected:
        struct ProgramMatch
        {
            std::optional<size_t> Indices[NumProgramTypes]{};
            size_t Hash{};
        };

        std::string m_Name{};
        std::vector<std::unique_ptr<ShaderProgram>> m_Programs[NumProgramTypes]{};

        std::unordered_map<ShaderKeywordSet, ProgramMatch> m_ProgramMatches{};
        std::unordered_map<size_t, std::unique_ptr<RootSignatureType>> m_RootSignatures{};
        std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_PipelineStates{};

        const ProgramMatch& GetProgramMatch(const ShaderKeywordSet& keywords)
        {
            auto [it, isNew] = m_ProgramMatches.try_emplace(keywords);

            if (isNew)
            {
                DefaultHash hash{};
                ProgramMatch& m = it->second;
                size_t targetKeywordCount = keywords.GetNumEnabledKeywords();

                for (size_t i = 0; i < NumProgramTypes; i++)
                {
                    size_t minDiff = std::numeric_limits<size_t>::max();
                    m.Indices[i] = std::nullopt;

                    for (size_t j = 0; j < m_Programs[i].size(); j++)
                    {
                        const ShaderKeywordSet& ks = m_Programs[i][j]->GetKeywords();
                        size_t matchingCount = ks.GetNumMatchingKeywords(keywords);
                        size_t currentKeywordCount = ks.GetNumEnabledKeywords();

                        // diff = 没 match 的数量 + 多余的数量
                        if (size_t diff = targetKeywordCount + currentKeywordCount - 2 * matchingCount; diff < minDiff)
                        {
                            minDiff = diff;
                            m.Indices[i] = j;
                        }
                    }

                    if (m.Indices[i])
                    {
                        hash.Append(m_Programs[i][*m.Indices[i]]->GetHash());
                    }
                }

                m.Hash = *hash;
            }

            return it->second;
        }

    public:
        const std::string& GetName() const { return m_Name; }

        template <typename T>
        ShaderProgram* GetProgram(T type, const ShaderKeywordSet& keywords)
        {
            size_t typeIndex = static_cast<size_t>(type);
            std::optional<size_t> programIndex = GetProgramMatch(keywords).Indices[typeIndex];
            return programIndex ? m_Programs[typeIndex][*programIndex].get() : nullptr;
        }

        template <typename T>
        ShaderProgram* GetProgram(T type, size_t index) const
        {
            return m_Programs[static_cast<size_t>(type)][index].get();
        }

        template <typename T>
        size_t GetProgramCount(T type) const
        {
            return m_Programs[static_cast<size_t>(type)].size();
        }

        RootSignatureType* GetRootSignature(const ShaderKeywordSet& keywords);

        virtual ~ShaderProgramGroup() = default;

    protected:
        virtual D3D12_SHADER_VISIBILITY GetShaderVisibility(size_t programType) = 0;
        virtual bool GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType) = 0;
        virtual std::string GetTargetProfile(const std::string& shaderModel, size_t programType) = 0;
        virtual void RecordEntrypointCallback(size_t programType, std::string& entrypoint) = 0;
        virtual bool RecordConstantBufferCallback(ID3D12ShaderReflectionConstantBuffer* cbuffer, std::string& error) = 0;

    private:
        struct CompilationConfig
        {
            std::string ShaderModel = "6.0";
            bool EnableDebugInfo = false;
            std::string Entrypoints[NumProgramTypes]{};
            std::vector<std::vector<std::string>> MultiCompile{};
            ShaderKeywordSpace TempMultiCompileKeywordSpace{}; // MultiCompile 编译时使用的临时 KeywordSpace
        };

        struct CompilationContext
        {
            IDxcUtils* Utils = nullptr;
            IDxcCompiler3* Compiler = nullptr;
            IDxcIncludeHandler* IncludeHandler = nullptr;

            CompilationConfig Config{};
            std::wstring FileName{};
            std::wstring IncludePath{};
            DxcBuffer Source{};

            ShaderKeywordSpace* KeywordSpace = nullptr; // Shader 中保存的 KeywordSpace
            std::unordered_set<ShaderKeywordSet> CompiledKeywordSets{};
            std::vector<std::string> Keywords{};
            std::vector<std::string>* Warnings = nullptr;
            std::string* Error = nullptr;

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
                return CompiledKeywordSets.insert(keywordSet).second;
            }
        };

        bool PreprocessAndGetCompilationConfig(const std::string& source, CompilationConfig& config, std::string& error);
        bool CompileRecursive(CompilationContext& context);
        Microsoft::WRL::ComPtr<IDxcResult> CompileEntrypoint(CompilationContext& context, const std::wstring& entrypoint, const std::wstring& targetProfile);

    protected:
        bool Compile(
            ShaderKeywordSpace& keywordSpace,
            const std::string& filename,
            const std::string& source,
            std::vector<std::string>& warnings,
            std::string& error)
        {
            CompilationContext context{};
            context.Utils = ShaderUtils::GetDxcUtils();
            context.Compiler = ShaderUtils::GetDxcCompiler();

            //
            // Create default include handler. (You can create your own...)
            //
            ComPtr<IDxcIncludeHandler> pIncludeHandler;
            GFX_HR(context.Utils->CreateDefaultIncludeHandler(&pIncludeHandler));
            context.IncludeHandler = pIncludeHandler.Get();

            if (!PreprocessAndGetCompilationConfig(source, context.Config, error))
            {
                return false;
            }

            context.FileName = StringUtils::Utf8ToUtf16(filename);
            context.IncludePath = StringUtils::Utf8ToUtf16(ShaderUtils::GetEngineShaderPathUnixStyle());
            context.Source.Ptr = source.data();
            context.Source.Size = static_cast<SIZE_T>(source.size());
            context.Source.Encoding = DXC_CP_UTF8;
            context.KeywordSpace = &keywordSpace;
            context.Warnings = &warnings;
            context.Error = &error;

            return CompileRecursive(context);
        }
    };

    struct ShaderRootSignatureInternalUtils
    {
        static void AddStaticSamplers(std::vector<CD3DX12_STATIC_SAMPLER_DESC>& samplers, ShaderProgram* program, D3D12_SHADER_VISIBILITY visibility);
        static Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc);
    };

    template <size_t _NumProgramTypes>
    ShaderProgramGroup<_NumProgramTypes>::RootSignatureType* ShaderProgramGroup<_NumProgramTypes>::GetRootSignature(const ShaderKeywordSet& keywords)
    {
        const ProgramMatch& m = GetProgramMatch(keywords);

        if (auto it = m_RootSignatures.find(m.Hash); it != m_RootSignatures.end())
        {
            return it->second.get();
        }

        std::vector<CD3DX12_ROOT_PARAMETER> params{};
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> staticSamplers{};
        std::vector<CD3DX12_DESCRIPTOR_RANGE> srvUavRanges{};
        std::vector<CD3DX12_DESCRIPTOR_RANGE> samplerRanges{};
        std::unique_ptr<RootSignatureType> result = std::make_unique<RootSignatureType>();

        for (size_t i = 0; i < NumProgramTypes; i++)
        {
            if (!m.Indices[i])
            {
                continue;
            }

            ShaderProgram* program = m_Programs[i][*m.Indices[i]].get();
            size_t srvUavStartIndex = srvUavRanges.size();
            size_t samplerStartIndex = samplerRanges.size();
            D3D12_SHADER_VISIBILITY visibility = GetShaderVisibility(i);

            for (const ShaderProgramTexture& tex : program->GetSrvTextures())
            {
                ShaderParamSrvTexture& p = result->m_Params[i].SrvTextures.emplace_back();
                p.Id = tex.Id;

                srvUavRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1,
                    tex.ShaderRegisterTexture, tex.RegisterSpaceTexture, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                p.DescriptorTableSlotTexture = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex - 1);

                if (tex.HasSampler)
                {
                    samplerRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1,
                        tex.ShaderRegisterSampler, tex.RegisterSpaceSampler, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                    p.DescriptorTableSlotSampler = static_cast<uint32_t>(samplerRanges.size() - samplerStartIndex - 1);
                }
                else
                {
                    p.DescriptorTableSlotSampler = std::nullopt;
                }
            }

            for (const ShaderProgramBuffer& buf : program->GetUavBuffers())
            {
                ShaderParamUavBuffer& p = result->m_Params[i].UavBuffers.emplace_back();
                p.Id = buf.Id;

                srvUavRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1,
                    buf.ShaderRegister, buf.RegisterSpace, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                p.DescriptorTableSlot = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex - 1);
            }

            for (const ShaderProgramTexture& tex : program->GetUavTextures())
            {
                ShaderParamUavTexture& p = result->m_Params[i].UavTextures.emplace_back();
                p.Id = tex.Id;

                srvUavRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1,
                    tex.ShaderRegisterTexture, tex.RegisterSpaceTexture, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                p.DescriptorTableSlot = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex - 1);
            }

            // TODO: Performance TIP: Order from most frequent to least frequent.

            for (const ShaderProgramBuffer& buf : program->GetSrvCbvBuffers())
            {
                ShaderParamSrvCbvBuffer& p = result->m_Params[i].SrvCbvBuffers.emplace_back();
                p.Id = buf.Id;

                if (buf.ConstantBufferSize == 0)
                {
                    params.emplace_back().InitAsShaderResourceView(buf.ShaderRegister, buf.RegisterSpace, visibility);
                    p.IsConstantBuffer = false;
                }
                else
                {
                    params.emplace_back().InitAsConstantBufferView(buf.ShaderRegister, buf.RegisterSpace, visibility);
                    p.IsConstantBuffer = true;
                }

                p.RootParameterIndex = static_cast<uint32_t>(params.size() - 1);
            }

            if (srvUavRanges.size() > srvUavStartIndex)
            {
                uint32_t count = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex);
                params.emplace_back().InitAsDescriptorTable(count, srvUavRanges.data() + srvUavStartIndex, visibility);
                result->m_Params[i].SrvUavTableRootParamIndex = static_cast<uint32_t>(params.size() - 1);
            }
            else
            {
                result->m_Params[i].SrvUavTableRootParamIndex = std::nullopt;
            }

            if (samplerRanges.size() > samplerStartIndex)
            {
                uint32_t count = static_cast<uint32_t>(samplerRanges.size() - samplerStartIndex);
                params.emplace_back().InitAsDescriptorTable(count, samplerRanges.data() + samplerStartIndex, visibility);
                result->m_Params[i].SamplerTableRootParamIndex = static_cast<uint32_t>(params.size() - 1);
            }
            else
            {
                result->m_Params[i].SamplerTableRootParamIndex = std::nullopt;
            }

            ShaderRootSignatureInternalUtils::AddStaticSamplers(staticSamplers, program, visibility);
        }

        CD3DX12_ROOT_SIGNATURE_DESC desc(
            static_cast<UINT>(params.size()), params.data(),
            static_cast<UINT>(staticSamplers.size()), staticSamplers.data(),
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        result->m_RootSignature = ShaderRootSignatureInternalUtils::CreateRootSignature(desc);
        return (m_RootSignatures[m.Hash] = std::move(result)).get();
    }

    struct ShaderCompilationInternalUtils
    {
        static bool EnumeratePragmas(const std::string& source, const std::function<bool(const std::vector<std::string>&)>& fn);
        static void AppendEngineMacros(std::vector<std::wstring>& m);
        static void SaveCompilationResults(
            IDxcUtils* utils,
            Microsoft::WRL::ComPtr<IDxcResult> pResults,
            ShaderProgram* program,
            const std::function<void(ID3D12ShaderReflectionConstantBuffer*)>& recordConstantBufferCallback);
    };

    template <size_t _NumProgramTypes>
    bool ShaderProgramGroup<_NumProgramTypes>::PreprocessAndGetCompilationConfig(const std::string& source, CompilationConfig& config, std::string& error)
    {
        return ShaderCompilationInternalUtils::EnumeratePragmas(source, [&](const std::vector<std::string>& args) -> bool
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
                        if (!config.TempMultiCompileKeywordSpace.RegisterKeyword(args[i]))
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
                else if (size_t epIndex = 0; GetEntrypointProgramType(args[0], &epIndex))
                {
                    config.Entrypoints[epIndex] = args[1];
                }
            }

            return true;
        });
    }

    template <size_t _NumProgramTypes>
    bool ShaderProgramGroup<_NumProgramTypes>::CompileRecursive(CompilationContext& context)
    {
        // 组合 Keywords
        if (context.Keywords.size() < context.Config.MultiCompile.size())
        {
            const std::vector<std::string>& candidates = context.Config.MultiCompile[context.Keywords.size()];

            for (size_t i = 0; i < candidates.size(); i++)
            {
                context.Keywords.push_back(candidates[i]);
                bool success = CompileRecursive(context);
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

        for (size_t i = 0; i < NumProgramTypes; i++)
        {
            RecordEntrypointCallback(i, context.Config.Entrypoints[i]);

            if (context.Config.Entrypoints[i].empty())
            {
                continue;
            }

            std::wstring wEntrypoint = StringUtils::Utf8ToUtf16(context.Config.Entrypoints[i]);
            std::wstring wTargetProfile = StringUtils::Utf8ToUtf16(GetTargetProfile(context.Config.ShaderModel, i));
            Microsoft::WRL::ComPtr<IDxcResult> pResults = CompileEntrypoint(context, wEntrypoint, wTargetProfile);

            // 编译失败
            if (pResults == nullptr)
            {
                return false;
            }

            ShaderProgram* program = m_Programs[i].emplace_back(std::make_unique<ShaderProgram>()).get();

            // 保存 Keyword
            for (const std::string& kw : context.Keywords)
            {
                if (!kw.empty())
                {
                    context.KeywordSpace->RegisterKeyword(kw);
                    program->m_Keywords.EnableKeyword(*context.KeywordSpace, kw);
                }
            }

            ShaderCompilationInternalUtils::SaveCompilationResults(context.Utils, pResults, program, [this](ID3D12ShaderReflectionConstantBuffer* cbuffer)
            {
                RecordConstantBufferCallback(cbuffer);
            });
        }

        return true;
    }

    template <size_t _NumProgramTypes>
    Microsoft::WRL::ComPtr<IDxcResult> ShaderProgramGroup<_NumProgramTypes>::CompileEntrypoint(CompilationContext& context, const std::wstring& entrypoint, const std::wstring& targetProfile)
    {
        // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

        std::vector<LPCWSTR> pszArgs =
        {
            context.FileName.c_str(),           // Optional shader source file name for error reporting and for PIX shader source view.
            L"-E", entrypoint.c_str(),          // Entry point.
            L"-T", targetProfile.c_str(),       // Target.
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

        std::vector<std::wstring> defines{};
        ShaderCompilationInternalUtils::AppendEngineMacros(defines);

        for (const std::string& kw : context.Keywords)
        {
            if (!kw.empty())
            {
                defines.push_back(StringUtils::Utf8ToUtf16(kw) + L"=1");
            }
        }

        for (const auto& d : defines)
        {
            pszArgs.push_back(L"-D");
            pszArgs.push_back(d.c_str());
        }

        //
        // Compile it with specified arguments.
        //
        Microsoft::WRL::ComPtr<IDxcResult> pResults = nullptr;
        GFX_HR(context.Compiler->Compile(
            &context.Source,                     // Source buffer.
            pszArgs.data(),                      // Array of pointers to arguments.
            static_cast<UINT32>(pszArgs.size()), // Number of arguments.
            context.IncludeHandler,              // User-provided interface to handle #include directives (optional).
            IID_PPV_ARGS(&pResults)              // Compiler output status, buffer, and errors.
        ));

        // Note that d3dcompiler would return null if no errors or warnings are present.
        // IDxcCompiler3::Compile will always return an error buffer, but its length
        // will be zero if there are no warnings or errors.
        Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors = nullptr;
        GFX_HR(pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));

        HRESULT hrStatus;
        GFX_HR(pResults->GetStatus(&hrStatus));

        if (FAILED(hrStatus))
        {
            *context.Error = pErrors->GetStringPointer();
            return nullptr;
        }

        if (pErrors->GetStringLength() > 0)
        {
            context.Warnings->push_back(pErrors->GetStringPointer());
        }

        return pResults;
    }
}
