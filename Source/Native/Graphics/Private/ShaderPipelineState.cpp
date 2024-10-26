#include "Shader.h"
#include "GfxDevice.h"
#include "GfxExcept.h"
#include "GfxSettings.h"
#include "HashHelper.h"
#include "Debug.h"
#include "StringUtility.h"

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

        size_t hash = HashHelper::FNV1(static_cast<uint32_t*>(bufferPointer), bufferSize / 4);
        ComPtr<ID3D12RootSignature>& result = g_RootSignatures[hash];

        if (result == nullptr)
        {
            DEBUG_LOG_INFO("Create new RootSignature");

            ID3D12Device4* device = GetGfxDevice()->GetD3D12Device();
            GFX_HR(device->CreateRootSignature(0, bufferPointer, bufferSize, IID_PPV_ARGS(result.GetAddressOf())));
        }
        else
        {
            DEBUG_LOG_INFO("Reuse RootSignature");
        }

        return result.Get();
    }

    ID3D12RootSignature* ShaderPass::GetRootSignature()
    {
        if (m_RootSignature == nullptr)
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
                DEBUG_LOG_ERROR(reinterpret_cast<char*>(error->GetBufferPointer()));
            }

            GFX_HR(hr);

            m_RootSignature = CreateRootSignature(serializedData.Get());
        }

        return m_RootSignature.Get();
    }

    void Shader::ClearRootSignatureCache()
    {
        g_RootSignatures.clear();
    }

    PipelineInputElement::PipelineInputElement(
        PipelineInputSematicName semanticName,
        uint32_t semanticIndex,
        DXGI_FORMAT format,
        uint32_t inputSlot,
        D3D12_INPUT_CLASSIFICATION inputSlotClass,
        uint32_t instanceDataStepRate)
    {
        SemanticName = semanticName;
        SemanticIndex = semanticIndex;
        Format = format;
        InputSlot = inputSlot;
        InputSlotClass = inputSlotClass;
        InstanceDataStepRate = instanceDataStepRate;
    }

    struct PipelineInputDesc
    {
        std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout{};
        D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology{};
        size_t Hash{};
    };

    static std::vector<PipelineInputDesc> g_PipelineInputDescs{};

    static constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType(D3D12_PRIMITIVE_TOPOLOGY topology)
    {
        switch (topology)
        {
        case D3D_PRIMITIVE_TOPOLOGY_UNDEFINED:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

        case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

        case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
        case D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
        case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
        case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        default:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        }
    }

    int32_t Shader::GetInvalidPipelineInputDescId()
    {
        return -1;
    }

    int32_t Shader::CreatePipelineInputDesc(const std::vector<PipelineInputElement>& inputLayout, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
    {
        PipelineInputDesc& desc = g_PipelineInputDescs.emplace_back();
        desc.PrimitiveTopology = primitiveTopology;

        // Hash
        desc.Hash = HashHelper::FNV1(inputLayout.data(), inputLayout.size());
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = GetTopologyType(primitiveTopology);
        desc.Hash = HashHelper::FNV1(&topologyType, 1, desc.Hash); // PSO 里用的是 D3D12_PRIMITIVE_TOPOLOGY_TYPE

        // Input Layout
        for (const auto& input : inputLayout)
        {
            D3D12_INPUT_ELEMENT_DESC& element = desc.InputLayout.emplace_back();

            switch (input.SemanticName)
            {
            case PipelineInputSematicName::Position:
                element.SemanticName = "POSITION";
                break;

            case PipelineInputSematicName::Normal:
                element.SemanticName = "NORMAL";
                break;

            case PipelineInputSematicName::Tangent:
                element.SemanticName = "TANGENT";
                break;

            case PipelineInputSematicName::TexCoord:
                element.SemanticName = "TEXCOORD";
                break;

            case PipelineInputSematicName::Color:
                element.SemanticName = "COLOR";
                break;

            default:
                throw GfxException("Unknown input semantic name");
            }

            element.SemanticIndex = static_cast<UINT>(input.SemanticIndex);
            element.Format = input.Format;
            element.InputSlot = static_cast<UINT>(input.InputSlot);
            element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            element.InputSlotClass = input.InputSlotClass;
            element.InstanceDataStepRate = static_cast<UINT>(input.InstanceDataStepRate);
        }

        return static_cast<int32_t>(g_PipelineInputDescs.size() - 1);
    }

    D3D12_PRIMITIVE_TOPOLOGY Shader::GetPipelineInputDescPrimitiveTopology(int32_t inputDescId)
    {
        return g_PipelineInputDescs[inputDescId].PrimitiveTopology;
    }

    static size_t GetPipelineInputDescHash(int32_t inputDescId)
    {
        return g_PipelineInputDescs[inputDescId].Hash;
    }

    static const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetPipelineInputLayout(int32_t inputDescId)
    {
        return g_PipelineInputDescs[inputDescId].InputLayout;
    }

    static D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPipelineInputDescPrimitiveTopologyType(int32_t inputDescId)
    {
        return GetTopologyType(Shader::GetPipelineInputDescPrimitiveTopology(inputDescId));
    }

    size_t PipelineStateDesc::CalculateHash(const PipelineStateDesc& desc)
    {
        size_t hash = HashHelper::FNV1(desc.RTVFormats.data(), desc.RTVFormats.size());
        hash = HashHelper::FNV1(&desc.DSVFormat, 1, hash);
        hash = HashHelper::FNV1(&desc.SampleCount, 1, hash);
        hash = HashHelper::FNV1(&desc.SampleQuality, 1, hash);

        // bool 不够 4 字节
        uint32_t wireframe = desc.Wireframe ? 1 : 0;
        hash = HashHelper::FNV1(&wireframe, 1, hash);

        return hash;
    }

    static void SetShaderProgramIfExists(D3D12_SHADER_BYTECODE& s, const ShaderPass* pass, ShaderProgramType type)
    {
        ShaderProgram* program = pass->GetProgram(type);

        if (program == nullptr)
        {
            s.pShaderBytecode = nullptr;
            s.BytecodeLength = 0;
        }
        else
        {
            s.pShaderBytecode = program->GetBinaryData();
            s.BytecodeLength = static_cast<SIZE_T>(program->GetBinarySize());
        }
    }

    static inline void ApplyReversedZBuffer(D3D12_DEPTH_STENCIL_DESC& depthStencil)
    {
        if constexpr (!GfxSettings::UseReversedZBuffer())
        {
            return;
        }

        switch (depthStencil.DepthFunc)
        {
        case D3D12_COMPARISON_FUNC_LESS:
            depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
            break;

        case D3D12_COMPARISON_FUNC_LESS_EQUAL:
            depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            break;

        case D3D12_COMPARISON_FUNC_GREATER:
            depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            break;

        case D3D12_COMPARISON_FUNC_GREATER_EQUAL:
            depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            break;
        }
    }

    ID3D12PipelineState* ShaderPass::GetGraphicsPipelineState(int32_t inputDescId, const PipelineStateDesc& stateDesc, size_t stateDescHash)
    {
        size_t hash = HashHelper::FNV1(&stateDescHash, 1, GetPipelineInputDescHash(inputDescId));
        ComPtr<ID3D12PipelineState>& result = m_PipelineStates[hash];

        if (result == nullptr)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

            psoDesc.pRootSignature = GetRootSignature();

            SetShaderProgramIfExists(psoDesc.VS, this, ShaderProgramType::Vertex);
            SetShaderProgramIfExists(psoDesc.PS, this, ShaderProgramType::Pixel);

            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.BlendState.IndependentBlendEnable = m_Blends.size() > 1 ? TRUE : FALSE;

            for (int i = 0; i < m_Blends.size(); i++)
            {
                const ShaderPassBlendState& b = m_Blends[i];
                D3D12_RENDER_TARGET_BLEND_DESC& blendDesc = psoDesc.BlendState.RenderTarget[i];

                blendDesc.BlendEnable = b.Enable;
                blendDesc.LogicOpEnable = FALSE;
                blendDesc.SrcBlend = static_cast<D3D12_BLEND>(static_cast<int>(b.Rgb.Src) + 1);
                blendDesc.DestBlend = static_cast<D3D12_BLEND>(static_cast<int>(b.Rgb.Dest) + 1);
                blendDesc.BlendOp = static_cast<D3D12_BLEND_OP>(static_cast<int>(b.Rgb.Op) + 1);
                blendDesc.SrcBlendAlpha = static_cast<D3D12_BLEND>(static_cast<int>(b.Alpha.Src) + 1);
                blendDesc.DestBlendAlpha = static_cast<D3D12_BLEND>(static_cast<int>(b.Alpha.Dest) + 1);
                blendDesc.BlendOpAlpha = static_cast<D3D12_BLEND_OP>(static_cast<int>(b.Alpha.Op) + 1);
                blendDesc.RenderTargetWriteMask = static_cast<D3D12_COLOR_WRITE_ENABLE>(b.WriteMask);
            }

            psoDesc.SampleMask = UINT_MAX;

            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(static_cast<int>(m_Cull) + 1);
            psoDesc.RasterizerState.FillMode = stateDesc.Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = m_DepthState.Enable;
            psoDesc.DepthStencilState.DepthWriteMask = m_DepthState.Write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            psoDesc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(m_DepthState.Compare) + 1);
            psoDesc.DepthStencilState.StencilEnable = m_StencilState.Enable;
            psoDesc.DepthStencilState.StencilReadMask = static_cast<UINT8>(m_StencilState.ReadMask);
            psoDesc.DepthStencilState.StencilWriteMask = static_cast<UINT8>(m_StencilState.WriteMask);
            psoDesc.DepthStencilState.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(m_StencilState.FrontFace.FailOp) + 1);
            psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(m_StencilState.FrontFace.DepthFailOp) + 1);
            psoDesc.DepthStencilState.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(m_StencilState.FrontFace.PassOp) + 1);
            psoDesc.DepthStencilState.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(m_StencilState.FrontFace.Compare) + 1);
            psoDesc.DepthStencilState.BackFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(m_StencilState.BackFace.FailOp) + 1);
            psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(m_StencilState.BackFace.DepthFailOp) + 1);
            psoDesc.DepthStencilState.BackFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(m_StencilState.BackFace.PassOp) + 1);
            psoDesc.DepthStencilState.BackFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(m_StencilState.BackFace.Compare) + 1);
            ApplyReversedZBuffer(psoDesc.DepthStencilState);

            const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout = GetPipelineInputLayout(inputDescId);
            psoDesc.InputLayout.NumElements = static_cast<UINT>(inputLayout.size());
            psoDesc.InputLayout.pInputElementDescs = inputLayout.data();
            psoDesc.PrimitiveTopologyType = GetPipelineInputDescPrimitiveTopologyType(inputDescId);

            psoDesc.NumRenderTargets = static_cast<UINT>(stateDesc.RTVFormats.size());
            std::copy_n(stateDesc.RTVFormats.data(), stateDesc.RTVFormats.size(), psoDesc.RTVFormats);
            psoDesc.DSVFormat = stateDesc.DSVFormat;

            psoDesc.SampleDesc.Count = static_cast<UINT>(stateDesc.SampleCount);
            psoDesc.SampleDesc.Quality = static_cast<UINT>(stateDesc.SampleQuality);

            ID3D12Device4* device = GetGfxDevice()->GetD3D12Device();
            GFX_HR(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(result.GetAddressOf())));

#ifdef ENABLE_GFX_DEBUG_NAME
            const std::string& debugName = m_Shader->GetName() + " - " + m_Name;
            result->SetName(StringUtility::Utf8ToUtf16(debugName).c_str());
#endif

            DEBUG_LOG_INFO("Create Graphics PSO for '%s' Pass of '%s' Shader", m_Name.c_str(), m_Shader->GetName().c_str());
        }

        return result.Get();
    }
}
