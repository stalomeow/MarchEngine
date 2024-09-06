#include "Rendering/Shader.h"
#include "Core/Debug.h"
#include "Core/StringUtility.h"
#include "Rendering/GfxManager.h"
#include "Rendering/Mesh.hpp"
#include <d3d12shader.h>    // Shader reflection.
#include <algorithm>

using namespace Microsoft::WRL;

namespace dx12demo
{
    namespace
    {
        std::string GetTargetProfile(const std::string& shaderModel, ShaderProgramType programType)
        {
            std::string model = shaderModel;
            std::replace(model.begin(), model.end(), '.', '_');

            std::string program;

            switch (programType)
            {
            case dx12demo::ShaderProgramType::Vertex:
                program = "vs";
                break;

            case dx12demo::ShaderProgramType::Pixel:
                program = "ps";
                break;

            default:
                program = "unknown";
                break;
            }

            return program + "_" + model;
        }
    }

    void Shader::CompilePass(int passIndex,
        const std::string& filename,
        const std::string& program,
        const std::string& entrypoint,
        const std::string& shaderModel,
        ShaderProgramType programType)
    {
        // https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll

        ShaderPass& targetPass = *Passes[passIndex].get();
        IDxcUtils* pUtils = GetDxcUtils();
        IDxcCompiler3* pCompiler = GetDxcCompiler();

        //
        // Create default include handler. (You can create your own...)
        //
        ComPtr<IDxcIncludeHandler> pIncludeHandler;
        THROW_IF_FAILED(pUtils->CreateDefaultIncludeHandler(&pIncludeHandler));

        std::wstring wFilename = StringUtility::Utf8ToUtf16(filename);
        std::wstring wEntrypoint = StringUtility::Utf8ToUtf16(entrypoint);
        std::wstring wTargetProfile = StringUtility::Utf8ToUtf16(GetTargetProfile(shaderModel, programType));

        LPCWSTR pszArgs[] =
        {
            wFilename.c_str(),                     // Optional shader source file name for error reporting and for PIX shader source view.
            L"-E", wEntrypoint.c_str(),            // Entry point.
            L"-T", wTargetProfile.c_str(),         // Target.
            // L"-D", L"MYDEFINE=1",               // A single define.
            L"-Zi",                                           // Enable debug information.
            L"-I", L"C:/Projects/Graphics/dx12-demo/Shaders", // Include directory.
            // L"-Qstrip_reflect",                            // Strip reflection into a separate blob.
        };

        //
        // Open source file.
        //
        ComPtr<IDxcBlobEncoding> pSource = nullptr;
        THROW_IF_FAILED(pUtils->CreateBlob(program.data(), program.size(), DXC_CP_UTF8, &pSource));
        DxcBuffer Source = {};
        Source.Ptr = pSource->GetBufferPointer();
        Source.Size = pSource->GetBufferSize();
        Source.Encoding = DXC_CP_UTF8; // DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.

        //
        // Compile it with specified arguments.
        //
        ComPtr<IDxcResult> pResults;
        THROW_IF_FAILED(pCompiler->Compile(
            &Source,                // Source buffer.
            pszArgs,                // Array of pointers to arguments.
            _countof(pszArgs),      // Number of arguments.
            pIncludeHandler.Get(),  // User-provided interface to handle #include directives (optional).
            IID_PPV_ARGS(&pResults) // Compiler output status, buffer, and errors.
        ));

        //
        // Print errors if present.
        //
        ComPtr<IDxcBlobUtf8> pErrors = nullptr;
        THROW_IF_FAILED(pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr));
        // Note that d3dcompiler would return null if no errors or warnings are present.
        // IDxcCompiler3::Compile will always return an error buffer, but its length
        // will be zero if there are no warnings or errors.
        if (pErrors != nullptr && pErrors->GetStringLength() != 0)
        {
            DEBUG_LOG_ERROR("Compilation Failed; Warnings and Errors:\n%s\n", pErrors->GetStringPointer());
        }

        //
        // Quit if the compilation failed.
        //
        HRESULT hrStatus;
        THROW_IF_FAILED(pResults->GetStatus(&hrStatus));
        if (FAILED(hrStatus))
        {
            return;
        }

        //
        // Save shader binary.
        //
        ComPtr<IDxcBlob>* pShader = nullptr;

        switch (programType)
        {
        case dx12demo::ShaderProgramType::Vertex:
            pShader = &targetPass.VertexShader;
            break;

        case dx12demo::ShaderProgramType::Pixel:
            pShader = &targetPass.PixelShader;
            break;

        default:
            DEBUG_LOG_ERROR("Unknown ShaderProgramType: %d", (int)programType);
            return;
        }

        ComPtr<IDxcBlobUtf16> pShaderName = nullptr;
        THROW_IF_FAILED(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader->ReleaseAndGetAddressOf()), &pShaderName));

        //
        // Get separate reflection.
        //
        ComPtr<IDxcBlob> pReflectionData;
        THROW_IF_FAILED(pResults->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&pReflectionData), nullptr));
        if (pReflectionData != nullptr)
        {
            // Optionally, save reflection blob for later here.

            // Create reflection interface.
            DxcBuffer ReflectionData;
            ReflectionData.Encoding = DXC_CP_ACP;
            ReflectionData.Ptr = pReflectionData->GetBufferPointer();
            ReflectionData.Size = pReflectionData->GetBufferSize();

            ComPtr<ID3D12ShaderReflection> pReflection;
            pUtils->CreateReflection(&ReflectionData, IID_PPV_ARGS(&pReflection));

            // Use reflection interface here.

            D3D12_SHADER_DESC shaderDesc = {};
            THROW_IF_FAILED(pReflection->GetDesc(&shaderDesc));

            for (UINT i = 0; i < shaderDesc.BoundResources; i++)
            {
                D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
                THROW_IF_FAILED(pReflection->GetResourceBindingDesc(i, &bindDesc));

                switch (bindDesc.Type)
                {
                case D3D_SIT_CBUFFER:
                {
                    ID3D12ShaderReflectionConstantBuffer* cbMat = pReflection->GetConstantBufferByName(bindDesc.Name);
                    D3D12_SHADER_BUFFER_DESC cbDesc = {};
                    THROW_IF_FAILED(cbMat->GetDesc(&cbDesc));

                    ShaderPassConstantBuffer& cb = targetPass.ConstantBuffers[bindDesc.Name];
                    cb.ShaderRegister = bindDesc.BindPoint;
                    cb.RegisterSpace = bindDesc.Space;
                    cb.Size = cbDesc.Size;
                    break;
                }

                case D3D_SIT_TEXTURE:
                {
                    ShaderPassTextureProperty& tex = targetPass.TextureProperties[bindDesc.Name];

                    tex.ShaderRegisterTexture = bindDesc.BindPoint;
                    tex.RegisterSpaceTexture = bindDesc.Space;

                    std::string samplerName = std::string("sampler") + bindDesc.Name;
                    D3D12_SHADER_INPUT_BIND_DESC samplerDesc = {};
                    HRESULT hr = pReflection->GetResourceBindingDescByName(samplerName.c_str(), &samplerDesc);

                    if (SUCCEEDED(hr))
                    {
                        tex.HasSampler = true;
                        tex.ShaderRegisterSampler = samplerDesc.BindPoint;
                        tex.RegisterSpaceSampler = samplerDesc.Space;
                    }
                    else
                    {
                        tex.HasSampler = false;
                    }
                    break;
                }

                case D3D_SIT_SAMPLER:
                {
                    ShaderPassSampler& sampler = targetPass.Samplers[bindDesc.Name];
                    sampler.ShaderRegister = bindDesc.BindPoint;
                    sampler.RegisterSpace = bindDesc.Space;
                    break;
                }
                }
            }

            ID3D12ShaderReflectionConstantBuffer* cbMat = pReflection->GetConstantBufferByName(ShaderPass::MaterialCbName);
            D3D12_SHADER_BUFFER_DESC cbMatDesc = {};
            HRESULT hr = cbMat->GetDesc(&cbMatDesc);

            if (SUCCEEDED(hr))
            {
                for (UINT i = 0; i < cbMatDesc.Variables; i++)
                {
                    ID3D12ShaderReflectionVariable* var = cbMat->GetVariableByIndex(i);
                    D3D12_SHADER_VARIABLE_DESC varDesc = {};
                    THROW_IF_FAILED(var->GetDesc(&varDesc));

                    ShaderPassMaterialProperty& prop = targetPass.MaterialProperties[varDesc.Name];
                    prop.Offset = varDesc.StartOffset;
                    prop.Size = varDesc.Size;
                }
            }
        }
    }

    std::vector<CD3DX12_STATIC_SAMPLER_DESC> ShaderPass::CreateStaticSamplers()
    {
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> results;
        decltype(Samplers.end()) it;

        if ((it = Samplers.find("sampler_PointWrap")) != Samplers.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = results.emplace_back(
                it->second.ShaderRegister,        // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
        }

        if ((it = Samplers.find("sampler_PointClamp")) != Samplers.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = results.emplace_back(
                it->second.ShaderRegister,         // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
        }

        if ((it = Samplers.find("sampler_LinearWrap")) != Samplers.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = results.emplace_back(
                it->second.ShaderRegister,        // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
        }

        if ((it = Samplers.find("sampler_LinearClamp")) != Samplers.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = results.emplace_back(
                it->second.ShaderRegister,         // shaderRegister
                D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
        }

        if ((it = Samplers.find("sampler_AnisotropicWrap")) != Samplers.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = results.emplace_back(
                it->second.ShaderRegister,        // shaderRegister
                D3D12_FILTER_ANISOTROPIC,         // filter
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
        }

        if ((it = Samplers.find("sampler_AnisotropicClamp")) != Samplers.end())
        {
            CD3DX12_STATIC_SAMPLER_DESC& desc = results.emplace_back(
                it->second.ShaderRegister,         // shaderRegister
                D3D12_FILTER_ANISOTROPIC,          // filter
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
            desc.RegisterSpace = it->second.RegisterSpace;
        }

        return results;
    }

    void ShaderPass::CreateRootSignature()
    {
        std::vector<CD3DX12_DESCRIPTOR_RANGE> cbvSrvUavRanges;
        std::vector<CD3DX12_DESCRIPTOR_RANGE> samplerRanges;

        for (auto it = TextureProperties.begin(); it != TextureProperties.end(); ++it)
        {
            ShaderPassTextureProperty& texProp = it->second;

            cbvSrvUavRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1,
                texProp.ShaderRegisterTexture, texProp.RegisterSpaceTexture,
                D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
            texProp.TextureDescriptorTableIndex = cbvSrvUavRanges.size() - 1;

            if (texProp.HasSampler)
            {
                samplerRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1,
                    texProp.ShaderRegisterSampler, texProp.RegisterSpaceSampler,
                    D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                texProp.SamplerDescriptorTableIndex = samplerRanges.size() - 1;
            }
        }

        for (auto it = ConstantBuffers.begin(); it != ConstantBuffers.end(); ++it)
        {
            ShaderPassConstantBuffer& cbProp = it->second;
            cbvSrvUavRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1,
                cbProp.ShaderRegister, cbProp.RegisterSpace,
                D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
            cbProp.DescriptorTableIndex = cbvSrvUavRanges.size() - 1;
        }

        std::vector<CD3DX12_ROOT_PARAMETER> params;

        // TODO: optimize visibility
        // Perfomance TIP: Order from most frequent to least frequent.
        if (cbvSrvUavRanges.size() > 0)
        {
            params.emplace_back().InitAsDescriptorTable(cbvSrvUavRanges.size(), cbvSrvUavRanges.data(), D3D12_SHADER_VISIBILITY_ALL);
            m_CbvSrvUavCount = cbvSrvUavRanges.size();
            m_CbvSrvUavRootParamIndex = params.size() - 1;
        }
        else
        {
            m_CbvSrvUavCount = 0;
        }

        if (samplerRanges.size() > 0)
        {
            params.emplace_back().InitAsDescriptorTable(samplerRanges.size(), samplerRanges.data(), D3D12_SHADER_VISIBILITY_PIXEL);
            m_SamplerCount = samplerRanges.size();
            m_SamplerRootParamIndex = params.size() - 1;
        }
        else
        {
            m_SamplerCount = 0;
        }

        auto staticSamplers = CreateStaticSamplers();

        CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(static_cast<UINT>(params.size()), params.data(),
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
        THROW_IF_FAILED(hr);

        THROW_IF_FAILED(GetGfxManager().GetDevice()->CreateRootSignature(0,
            serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&m_RootSignature)));
    }
}
