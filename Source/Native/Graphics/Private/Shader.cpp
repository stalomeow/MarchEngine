#include "Shader.h"
#include "Debug.h"
#include "StringUtility.h"
#include "GfxDevice.h"
#include "GfxSettings.h"
#include "GfxHelpers.h"
#include "GfxExcept.h"
#include "GfxTexture.h"
#include "PathHelper.h"
#include <d3d12shader.h>    // Shader reflection.
#include <algorithm>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace march
{
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

    GfxTexture* ShaderProperty::GetDefaultTexture() const
    {
        if (Type != ShaderPropertyType::Texture)
        {
            throw GfxException("Property is not a texture type");
        }

        switch (DefaultValue.Texture)
        {
        case ShaderDefaultTexture::Black:
            return GfxTexture::GetDefaultBlack();

        case ShaderDefaultTexture::White:
            return GfxTexture::GetDefaultWhite();

        default:
            throw GfxException("Unknown default texture type");
        }
    }

    const std::string& ShaderPass::GetName() const
    {
        return m_Name;
    }

    const std::unordered_map<int32_t, ShaderPropertyLocation>& ShaderPass::GetPropertyLocations() const
    {
        return m_PropertyLocations;
    }

    ShaderProgram* ShaderPass::GetProgram(ShaderProgramType type) const
    {
        return m_Programs[static_cast<int32_t>(type)].get();
    }

    ShaderProgram* ShaderPass::CreateProgram(ShaderProgramType type)
    {
        int32_t index = static_cast<int32_t>(type);
        m_Programs[index] = std::make_unique<ShaderProgram>();
        return m_Programs[index].get();
    }

    const ShaderPassCullMode& ShaderPass::GetCull() const
    {
        return m_Cull;
    }

    const std::vector<ShaderPassBlendState>& ShaderPass::GetBlends() const
    {
        return m_Blends;
    }

    const ShaderPassDepthState& ShaderPass::GetDepthState() const
    {
        return m_DepthState;
    }

    const ShaderPassStencilState& ShaderPass::GetStencilState() const
    {
        return m_StencilState;
    }

    ID3D12RootSignature* ShaderPass::GetRootSignature() const
    {
        return m_RootSignature.Get();
    }

    const std::unordered_map<int32_t, ShaderProperty>& Shader::GetProperties() const
    {
        return m_Properties;
    }

    ShaderPass* Shader::GetPass(int32_t index) const
    {
        if (index < 0 || index > m_Passes.size())
        {
            throw GfxException("Invalid pass index");
        }

        return m_Passes[index].get();
    }

    int32_t Shader::GetPassCount() const
    {
        return static_cast<int32_t>(m_Passes.size());
    }

    int32_t Shader::GetVersion() const
    {
        return m_Version;
    }

    std::string Shader::GetEngineShaderPathUnixStyle()
    {
#ifdef ENGINE_SHADER_UNIX_PATH
        return ENGINE_SHADER_UNIX_PATH;
#endif

        return PathHelper::GetWorkingDirectoryUtf8(PathStyle::Unix) + "/Shaders";
    }

    Microsoft::WRL::ComPtr<IDxcUtils> Shader::s_Utils = nullptr;
    Microsoft::WRL::ComPtr<IDxcCompiler3> Shader::s_Compiler = nullptr;

    IDxcUtils* Shader::GetDxcUtils()
    {
        if (s_Utils == nullptr)
        {
            GFX_HR(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_Utils)));
        }

        return s_Utils.Get();
    }

    IDxcCompiler3* Shader::GetDxcCompiler()
    {
        if (s_Compiler == nullptr)
        {
            GFX_HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_Compiler)));
        }

        return s_Compiler.Get();
    }

    std::unordered_map<std::string, int32_t> Shader::s_NameIdMap{};
    int32_t Shader::s_NextNameId = 0;

    int32_t Shader::GetNameId(const std::string& name)
    {
        if (auto it = s_NameIdMap.find(name); it != s_NameIdMap.end())
        {
            return it->second;
        }

        int32_t result = s_NextNameId++;
        s_NameIdMap[name] = result;
        return result;
    }

    const std::string& Shader::GetIdName(int32_t id)
    {
        if (id < s_NextNameId)
        {
            for (const auto& it : s_NameIdMap)
            {
                if (it.second == id)
                {
                    return it.first;
                }
            }
        }

        throw std::runtime_error("Invalid shader property id");
    }

    int32_t Shader::s_MaterialConstantBufferId = Shader::GetNameId("cbMaterial");

    int32_t Shader::GetMaterialConstantBufferId()
    {
        return s_MaterialConstantBufferId;
    }

    static std::string GetTargetProfile(const std::string& shaderModel, ShaderProgramType programType)
    {
        std::string model = shaderModel;
        std::replace(model.begin(), model.end(), '.', '_');

        std::string program;

        switch (programType)
        {
        case march::ShaderProgramType::Vertex:
            program = "vs";
            break;

        case march::ShaderProgramType::Pixel:
            program = "ps";
            break;

        default:
            program = "unknown";
            break;
        }

        return program + "_" + model;
    }

    bool Shader::CompilePass(int32_t passIndex,
        const std::string& filename,
        const std::string& program,
        const std::string& entrypoint,
        const std::string& shaderModel,
        ShaderProgramType programType)
    {
        // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

        ShaderPass* pass = GetPass(passIndex);
        IDxcUtils* utils = GetDxcUtils();
        IDxcCompiler3* compiler = GetDxcCompiler();

        //
        // Create default include handler. (You can create your own...)
        //
        ComPtr<IDxcIncludeHandler> pIncludeHandler;
        GFX_HR(utils->CreateDefaultIncludeHandler(&pIncludeHandler));

        std::wstring wFilename = StringUtility::Utf8ToUtf16(filename);
        std::wstring wEntrypoint = StringUtility::Utf8ToUtf16(entrypoint);
        std::wstring wTargetProfile = StringUtility::Utf8ToUtf16(GetTargetProfile(shaderModel, programType));
        std::wstring wIncludePath = StringUtility::Utf8ToUtf16(GetEngineShaderPathUnixStyle());

        std::vector<LPCWSTR> pszArgs =
        {
            wFilename.c_str(),             // Optional shader source file name for error reporting and for PIX shader source view.
            L"-E", wEntrypoint.c_str(),    // Entry point.
            L"-T", wTargetProfile.c_str(), // Target.
            // L"-D", L"MYDEFINE=1",       // A single define.
            L"-Zi",                        // Enable debug information.
            L"-I", wIncludePath.c_str(),   // Include directory.
            // L"-Qstrip_reflect",         // Strip reflection into a separate blob.
        };

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
        // Open source file.
        //
        ComPtr<IDxcBlobEncoding> pSource = nullptr;
        GFX_HR(utils->CreateBlob(program.data(), static_cast<UINT32>(program.size()), DXC_CP_UTF8, &pSource));
        DxcBuffer Source = {};
        Source.Ptr = pSource->GetBufferPointer();
        Source.Size = pSource->GetBufferSize();
        Source.Encoding = DXC_CP_UTF8; // DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.

        //
        // Compile it with specified arguments.
        //
        ComPtr<IDxcResult> pResults;
        GFX_HR(compiler->Compile(
            &Source,                             // Source buffer.
            pszArgs.data(),                      // Array of pointers to arguments.
            static_cast<UINT32>(pszArgs.size()), // Number of arguments.
            pIncludeHandler.Get(),               // User-provided interface to handle #include directives (optional).
            IID_PPV_ARGS(&pResults)              // Compiler output status, buffer, and errors.
        ));

        //
        // Print errors if present.
        //
        ComPtr<IDxcBlobUtf8> pErrors = nullptr;
        GFX_HR(pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));
        // Note that d3dcompiler would return null if no errors or warnings are present.
        // IDxcCompiler3::Compile will always return an error buffer, but its length
        // will be zero if there are no warnings or errors.
        if (pErrors != nullptr && pErrors->GetStringLength() != 0)
        {
            DEBUG_LOG_ERROR(pErrors->GetStringPointer());
        }

        //
        // Quit if the compilation failed.
        //
        HRESULT hrStatus;
        GFX_HR(pResults->GetStatus(&hrStatus));
        if (FAILED(hrStatus))
        {
            return false;
        }

        //
        // Save shader binary.
        //
        ShaderProgram* shaderProgram = pass->CreateProgram(programType);
        ComPtr<IDxcBlobUtf16> shaderName = nullptr;
        GFX_HR(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderProgram->m_Binary), &shaderName));

        //
        // Get separate reflection.
        //
        ComPtr<IDxcBlob> pReflectionData;
        GFX_HR(pResults->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&pReflectionData), nullptr));
        if (pReflectionData != nullptr)
        {
            // Create reflection interface.
            DxcBuffer ReflectionData;
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

                    ShaderConstantBuffer& cb = shaderProgram->m_ConstantBuffers[GetNameId(bindDesc.Name)];
                    cb.ShaderRegister = bindDesc.BindPoint;
                    cb.RegisterSpace = bindDesc.Space;
                    cb.UnalignedSize = cbDesc.Size;
                    break;
                }

                case D3D_SIT_TEXTURE:
                {
                    ShaderTexture& tex = shaderProgram->m_Textures[GetNameId(bindDesc.Name)];
                    tex.ShaderRegisterTexture = bindDesc.BindPoint;
                    tex.RegisterSpaceTexture = bindDesc.Space;
                    break;
                }

                case D3D_SIT_SAMPLER:
                {
                    // 先假设全是 static sampler
                    ShaderStaticSampler& sampler = shaderProgram->m_StaticSamplers[GetNameId(bindDesc.Name)];
                    sampler.ShaderRegister = bindDesc.BindPoint;
                    sampler.RegisterSpace = bindDesc.Space;
                    break;
                }
                }
            }

            // 记录 shader property location
            if (shaderProgram->m_ConstantBuffers.count(GetMaterialConstantBufferId()) > 0)
            {
                const std::string& cbName = GetIdName(GetMaterialConstantBufferId());
                ID3D12ShaderReflectionConstantBuffer* cbMat = pReflection->GetConstantBufferByName(cbName.c_str());

                D3D12_SHADER_BUFFER_DESC cbMatDesc = {};
                if (SUCCEEDED(cbMat->GetDesc(&cbMatDesc)))
                {
                    for (UINT i = 0; i < cbMatDesc.Variables; i++)
                    {
                        ID3D12ShaderReflectionVariable* var = cbMat->GetVariableByIndex(i);
                        D3D12_SHADER_VARIABLE_DESC varDesc = {};
                        GFX_HR(var->GetDesc(&varDesc));

                        ShaderPropertyLocation& loc = pass->m_PropertyLocations[GetNameId(varDesc.Name)];
                        loc.Offset = varDesc.StartOffset;
                        loc.Size = varDesc.Size;
                    }
                }
            }

            // 记录 texture sampler
            for (std::pair<const int32_t, ShaderTexture>& kv : shaderProgram->m_Textures)
            {
                int32_t samplerId = GetNameId("sampler" + GetIdName(kv.first));

                if (auto it = shaderProgram->m_StaticSamplers.find(samplerId); it != shaderProgram->m_StaticSamplers.end())
                {
                    kv.second.HasSampler = true;
                    kv.second.ShaderRegisterSampler = it->second.ShaderRegister;
                    kv.second.RegisterSpaceSampler = it->second.RegisterSpace;
                    shaderProgram->m_StaticSamplers.erase(it);
                }
            }
        }

        return true;
    }

    D3D12_SHADER_VISIBILITY ShaderPass::ToShaderVisibility(ShaderProgramType type)
    {
        switch (type)
        {
        case ShaderProgramType::Vertex:
            return D3D12_SHADER_VISIBILITY_VERTEX;

        case ShaderProgramType::Pixel:
            return D3D12_SHADER_VISIBILITY_PIXEL;

        default:
            throw GfxException("Unknown shader program type");
        }
    }

    void ShaderPass::CreateRootSignature()
    {
        std::vector<CD3DX12_ROOT_PARAMETER> params;
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> staticSamplers;
        std::vector<CD3DX12_DESCRIPTOR_RANGE> srvUavRanges;
        std::vector<CD3DX12_DESCRIPTOR_RANGE> samplerRanges;

        for (int32_t i = 0; i < static_cast<int32_t>(ShaderProgramType::NumTypes); i++)
        {
            ShaderProgram* program = m_Programs[i].get();

            if (program == nullptr)
            {
                continue;
            }

            size_t srvUavStartIndex = srvUavRanges.size();
            size_t samplerStartIndex = samplerRanges.size();
            D3D12_SHADER_VISIBILITY visibility = ToShaderVisibility(static_cast<ShaderProgramType>(i));

            for (auto& kv : program->m_Textures)
            {
                ShaderTexture& tex = kv.second;

                srvUavRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1,
                    tex.ShaderRegisterTexture, tex.RegisterSpaceTexture, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                tex.TextureDescriptorTableIndex = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex - 1);

                if (tex.HasSampler)
                {
                    samplerRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1,
                        tex.ShaderRegisterSampler, tex.RegisterSpaceSampler, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                    tex.SamplerDescriptorTableIndex = static_cast<uint32_t>(samplerRanges.size() - samplerStartIndex - 1);
                }
            }

            // TODO: Performance TIP: Order from most frequent to least frequent.

            for (auto& kv : program->m_ConstantBuffers)
            {
                ShaderConstantBuffer& cb = kv.second;
                params.emplace_back().InitAsConstantBufferView(cb.ShaderRegister, cb.RegisterSpace, visibility);
                cb.RootParameterIndex = static_cast<uint32_t>(params.size() - 1);
            }

            if (srvUavRanges.size() > srvUavStartIndex)
            {
                uint32_t count = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex);
                params.emplace_back().InitAsDescriptorTable(count, srvUavRanges.data() + srvUavStartIndex, visibility);
                program->m_SrvUavRootParameterIndex = static_cast<uint32_t>(params.size() - 1);
            }

            if (samplerRanges.size() > samplerStartIndex)
            {
                uint32_t count = static_cast<uint32_t>(samplerRanges.size() - samplerStartIndex);
                params.emplace_back().InitAsDescriptorTable(count, samplerRanges.data() + samplerStartIndex, visibility);
                program->m_SamplerRootParameterIndex = static_cast<uint32_t>(params.size() - 1);
            }

            AddStaticSamplers(staticSamplers, program, visibility);
        }

        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
            static_cast<UINT>(params.size()), params.data(),
            static_cast<UINT>(staticSamplers.size()), staticSamplers.data(),
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> serializedRootSig = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
            serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

        if (errorBlob != nullptr)
        {
            DEBUG_LOG_ERROR(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        }
        GFX_HR(hr);

        GFX_HR(GetGfxDevice()->GetD3D12Device()->CreateRootSignature(0,
            serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&m_RootSignature)));
    }

    void ShaderPass::AddStaticSamplers(std::vector<CD3DX12_STATIC_SAMPLER_DESC>& samplers, ShaderProgram* program, D3D12_SHADER_VISIBILITY visibility)
    {
        auto& s = program->m_StaticSamplers;
        decltype(s.end()) it;

        if ((it = s.find(Shader::GetNameId("sampler_PointWrap"))) != s.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = samplers.emplace_back(
                it->second.ShaderRegister,        // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
            desc.ShaderVisibility = visibility;
        }

        if ((it = s.find(Shader::GetNameId("sampler_PointClamp"))) != s.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = samplers.emplace_back(
                it->second.ShaderRegister,         // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
            desc.ShaderVisibility = visibility;
        }

        if ((it = s.find(Shader::GetNameId("sampler_LinearWrap"))) != s.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = samplers.emplace_back(
                it->second.ShaderRegister,        // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
            desc.ShaderVisibility = visibility;
        }

        if ((it = s.find(Shader::GetNameId("sampler_LinearClamp"))) != s.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = samplers.emplace_back(
                it->second.ShaderRegister,         // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
            desc.ShaderVisibility = visibility;
        }

        if ((it = s.find(Shader::GetNameId("sampler_AnisotropicWrap"))) != s.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = samplers.emplace_back(
                it->second.ShaderRegister,        // shaderRegister
                D3D12_FILTER_ANISOTROPIC,         // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
            desc.ShaderVisibility = visibility;
        }

        if ((it = s.find(Shader::GetNameId("sampler_AnisotropicClamp"))) != s.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = samplers.emplace_back(
                it->second.ShaderRegister,         // shaderRegister
                D3D12_FILTER_ANISOTROPIC,          // filter
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
            desc.ShaderVisibility = visibility;
        }
    }
}
