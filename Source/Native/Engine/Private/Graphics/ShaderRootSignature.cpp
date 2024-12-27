#include "pch.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/HashUtils.h"
#include "Engine/Debug.h"
#include <vector>
#include <string>

using namespace Microsoft::WRL;

namespace march
{
    // ID3D12RootSignature 根据 Hash 复用
    static std::unordered_map<size_t, ComPtr<ID3D12RootSignature>> g_GlobalRootSignaturePool{};

    static D3D12_SHADER_VISIBILITY GetShaderVisibility(ShaderProgramType type)
    {
        // TODO compute, amplification, mesh

        switch (type)
        {
        case ShaderProgramType::Vertex:
            return D3D12_SHADER_VISIBILITY_VERTEX;
        case ShaderProgramType::Pixel:
            return D3D12_SHADER_VISIBILITY_PIXEL;
        case ShaderProgramType::Domain:
            return D3D12_SHADER_VISIBILITY_DOMAIN;
        case ShaderProgramType::Hull:
            return D3D12_SHADER_VISIBILITY_HULL;
        case ShaderProgramType::Geometry:
            return D3D12_SHADER_VISIBILITY_GEOMETRY;
        default:
            throw GfxException("Unknown shader program type");
        }
    }

    static void AddStaticSamplers(std::vector<CD3DX12_STATIC_SAMPLER_DESC>& samplers, ShaderProgram* program, D3D12_SHADER_VISIBILITY visibility)
    {
        static const std::vector<std::pair<std::string, D3D12_FILTER>> filters =
        {
            { "Point"    , D3D12_FILTER_MIN_MAG_MIP_POINT        },
            { "Linear"   , D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT },
            { "Trilinear", D3D12_FILTER_MIN_MAG_MIP_LINEAR       },
        };

        static const std::vector<std::pair<std::string, D3D12_TEXTURE_ADDRESS_MODE>> wraps =
        {
            { "Repeat"    , D3D12_TEXTURE_ADDRESS_MODE_WRAP        },
            { "Clamp"     , D3D12_TEXTURE_ADDRESS_MODE_CLAMP       },
            { "Mirror"    , D3D12_TEXTURE_ADDRESS_MODE_MIRROR      },
            { "MirrorOnce", D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE },
        };

        const std::unordered_map<int32_t, ShaderStaticSampler>& s = program->GetStaticSamplers();

        for (const auto& [filterName, filterValue] : filters)
        {
            for (const auto& [wrapName, wrapValue] : wraps)
            {
                if (auto it = s.find(Shader::GetNameId("sampler_" + filterName + wrapName)); it != s.end())
                {
                    CD3DX12_STATIC_SAMPLER_DESC& desc = samplers.emplace_back(
                        it->second.ShaderRegister, filterValue, wrapValue, wrapValue, wrapValue);
                    desc.RegisterSpace = it->second.RegisterSpace;
                    desc.ShaderVisibility = visibility;
                }
            }
        }

        // Anisotropic
        for (int aniso = 1; aniso <= 16; aniso++)
        {
            for (const auto& [wrapName, wrapValue] : wraps)
            {
                if (auto it = s.find(Shader::GetNameId("sampler_Aniso" + std::to_string(aniso) + wrapName)); it != s.end())
                {
                    CD3DX12_STATIC_SAMPLER_DESC& desc = samplers.emplace_back(
                        it->second.ShaderRegister, D3D12_FILTER_ANISOTROPIC, wrapValue, wrapValue, wrapValue);
                    desc.MaxAnisotropy = static_cast<UINT>(aniso);
                    desc.RegisterSpace = it->second.RegisterSpace;
                    desc.ShaderVisibility = visibility;
                }
            }
        }
    }

    static ComPtr<ID3D12RootSignature> CreateRootSignature(ID3DBlob* serializedData)
    {
        LPVOID bufferPointer = serializedData->GetBufferPointer();
        SIZE_T bufferSize = serializedData->GetBufferSize();

        if (bufferSize % 4 != 0)
        {
            throw GfxException("Invalid root signature data size");
        }

        DefaultHash hash{};
        hash.Append(bufferPointer, bufferSize);
        ComPtr<ID3D12RootSignature>& result = g_GlobalRootSignaturePool[*hash];

        if (result == nullptr)
        {
            LOG_TRACE("Create new RootSignature");

            ID3D12Device4* device = GetGfxDevice()->GetD3DDevice4();
            GFX_HR(device->CreateRootSignature(0, bufferPointer, bufferSize, IID_PPV_ARGS(result.GetAddressOf())));
        }
        else
        {
            LOG_TRACE("Reuse RootSignature");
        }

        return result;
    }

    GfxRootSignature* ShaderPass::GetRootSignature(const ShaderKeywordSet& keywords)
    {
        const ShaderPass::ProgramMatch& m = GetProgramMatch(keywords);

        if (auto it = m_RootSignatures.find(m.Hash); it != m_RootSignatures.end())
        {
            return it->second.get();
        }

        std::vector<CD3DX12_ROOT_PARAMETER> params{};
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> staticSamplers{};
        std::vector<CD3DX12_DESCRIPTOR_RANGE> srvUavRanges{};
        std::vector<CD3DX12_DESCRIPTOR_RANGE> samplerRanges{};
        std::unique_ptr<GfxRootSignature> rootSignature = std::make_unique<GfxRootSignature>();

        for (int32_t i = 0; i < ShaderProgram::NumTypes; i++)
        {
            if (m.Indices[i] == -1)
            {
                continue;
            }

            ShaderProgram* program = m_Programs[i][static_cast<size_t>(m.Indices[i])].get();
            size_t srvUavStartIndex = srvUavRanges.size();
            size_t samplerStartIndex = samplerRanges.size();
            D3D12_SHADER_VISIBILITY visibility = GetShaderVisibility(static_cast<ShaderProgramType>(i));
            auto& rootSignatureBindings = rootSignature->m_Bindings[i];

            for (const ShaderTexture& tex : program->GetSrvTextures())
            {
                GfxRootSignature::TextureBinding& binding = rootSignatureBindings.SrvTextureTableSlots.emplace_back();
                binding.Id = tex.Id;

                srvUavRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1,
                    tex.ShaderRegisterTexture, tex.RegisterSpaceTexture, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                binding.BindPointTexture = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex - 1);

                if (tex.HasSampler)
                {
                    samplerRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1,
                        tex.ShaderRegisterSampler, tex.RegisterSpaceSampler, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                    binding.BindPointSampler = static_cast<uint32_t>(samplerRanges.size() - samplerStartIndex - 1);
                }
                else
                {
                    binding.BindPointSampler = std::nullopt;
                }
            }

            for (const ShaderBuffer& buf : program->GetUavBuffers())
            {
                GfxRootSignature::UavBinding& binding = rootSignatureBindings.UavBufferTableSlots.emplace_back();
                binding.Id = buf.Id;

                srvUavRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1,
                    buf.ShaderRegister, buf.RegisterSpace, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                binding.BindPoint = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex - 1);
            }

            for (const ShaderTexture& tex : program->GetUavTextures())
            {
                GfxRootSignature::UavBinding& binding = rootSignatureBindings.UavTextureTableSlots.emplace_back();
                binding.Id = tex.Id;

                srvUavRanges.emplace_back(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1,
                    tex.ShaderRegisterTexture, tex.RegisterSpaceTexture, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
                binding.BindPoint = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex - 1);
            }

            // TODO: Performance TIP: Order from most frequent to least frequent.

            for (const ShaderBuffer& buf : program->GetSrvCbvBuffers())
            {
                GfxRootSignature::BufferBinding& binding = rootSignatureBindings.SrvCbvBufferRootParamIndices.emplace_back();
                binding.Id = buf.Id;

                if (buf.ConstantBufferSize == 0)
                {
                    params.emplace_back().InitAsShaderResourceView(buf.ShaderRegister, buf.RegisterSpace, visibility);
                    binding.IsConstantBuffer = false;
                }
                else
                {
                    params.emplace_back().InitAsConstantBufferView(buf.ShaderRegister, buf.RegisterSpace, visibility);
                    binding.IsConstantBuffer = true;
                }

                binding.BindPoint = static_cast<uint32_t>(params.size() - 1);
            }

            if (srvUavRanges.size() > srvUavStartIndex)
            {
                uint32_t count = static_cast<uint32_t>(srvUavRanges.size() - srvUavStartIndex);
                params.emplace_back().InitAsDescriptorTable(count, srvUavRanges.data() + srvUavStartIndex, visibility);
                rootSignatureBindings.SrvUavTableRootParamIndex = static_cast<uint32_t>(params.size() - 1);
            }
            else
            {
                rootSignatureBindings.SrvUavTableRootParamIndex = std::nullopt;
            }

            if (samplerRanges.size() > samplerStartIndex)
            {
                uint32_t count = static_cast<uint32_t>(samplerRanges.size() - samplerStartIndex);
                params.emplace_back().InitAsDescriptorTable(count, samplerRanges.data() + samplerStartIndex, visibility);
                rootSignatureBindings.SamplerTableRootParamIndex = static_cast<uint32_t>(params.size() - 1);
            }
            else
            {
                rootSignatureBindings.SamplerTableRootParamIndex = std::nullopt;
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

        rootSignature->m_RootSignature = CreateRootSignature(serializedData.Get());
        return (m_RootSignatures[m.Hash] = std::move(rootSignature)).get();
    }

    void Shader::ClearRootSignatureCache()
    {
        g_GlobalRootSignaturePool.clear();
    }
}
