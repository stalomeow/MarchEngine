#include "Shader.h"
#include "GfxDevice.h"
#include "HashUtils.h"
#include "Debug.h"

using namespace Microsoft::WRL;

namespace march
{
    // RootSignature 根据 Hash 复用
    static std::unordered_map<size_t, ComPtr<ID3D12RootSignature>> g_RootSignatures{};

    static D3D12_SHADER_VISIBILITY GetShaderVisibility(ShaderProgramType type)
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

    static void AddStaticSamplers(std::vector<CD3DX12_STATIC_SAMPLER_DESC>& samplers, ShaderProgram* program, D3D12_SHADER_VISIBILITY visibility)
    {
        auto& s = program->GetStaticSamplers();
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

    static ID3D12RootSignature* CreateRootSignature(ID3DBlob* serializedData)
    {
        LPVOID bufferPointer = serializedData->GetBufferPointer();
        SIZE_T bufferSize = serializedData->GetBufferSize();

        if (bufferSize % 4 != 0)
        {
            throw GfxException("Invalid root signature data size");
        }

        size_t hash = HashUtils::FNV1(static_cast<uint32_t*>(bufferPointer), bufferSize / 4);
        ComPtr<ID3D12RootSignature>& result = g_RootSignatures[hash];

        if (result == nullptr)
        {
            LOG_TRACE("Create new RootSignature");

            ID3D12Device4* device = GetGfxDevice()->GetD3D12Device();
            GFX_HR(device->CreateRootSignature(0, bufferPointer, bufferSize, IID_PPV_ARGS(result.GetAddressOf())));
        }
        else
        {
            LOG_TRACE("Reuse RootSignature");
        }

        return result.Get();
    }

    ID3D12RootSignature* ShaderPass::GetRootSignature(const ShaderKeywordSet& keywords)
    {
        if (auto it = m_RootSignatures.find(keywords); it != m_RootSignatures.end())
        {
            return it->second.Get();
        }

        std::vector<CD3DX12_ROOT_PARAMETER> params;
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> staticSamplers;
        std::vector<CD3DX12_DESCRIPTOR_RANGE> srvUavRanges;
        std::vector<CD3DX12_DESCRIPTOR_RANGE> samplerRanges;

        for (int32_t i = 0; i < static_cast<int32_t>(ShaderProgramType::NumTypes); i++)
        {
            ShaderProgram* program = GetProgram(static_cast<ShaderProgramType>(i), keywords);

            if (program == nullptr)
            {
                continue;
            }

            size_t srvUavStartIndex = srvUavRanges.size();
            size_t samplerStartIndex = samplerRanges.size();
            D3D12_SHADER_VISIBILITY visibility = GetShaderVisibility(static_cast<ShaderProgramType>(i));

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

        CD3DX12_ROOT_SIGNATURE_DESC desc(
            static_cast<UINT>(params.size()), params.data(),
            static_cast<UINT>(staticSamplers.size()), staticSamplers.data(),
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> serializedData = nullptr;
        ComPtr<ID3DBlob> error = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedData.GetAddressOf(), error.GetAddressOf());

        if (error != nullptr)
        {
            LOG_ERROR(reinterpret_cast<char*>(error->GetBufferPointer()));
        }

        GFX_HR(hr);

        ID3D12RootSignature* result = CreateRootSignature(serializedData.Get());
        m_RootSignatures[keywords] = result;
        return result;
    }

    void Shader::ClearRootSignatureCache()
    {
        g_RootSignatures.clear();
    }
}
