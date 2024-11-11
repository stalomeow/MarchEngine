#include "GfxPipelineState.h"
#include "HashHelper.h"
#include "StringUtility.h"
#include "GfxExcept.h"
#include "Shader.h"
#include "Material.h"
#include "GfxSettings.h"
#include "GfxDevice.h"
#include "Debug.h"
#include <wrl.h>

using namespace Microsoft::WRL;

namespace march
{
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

    int32_t GfxPipelineState::CreateInputDesc(const std::vector<PipelineInputElement>& inputLayout, D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
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

    D3D12_PRIMITIVE_TOPOLOGY GfxPipelineState::GetInputDescPrimitiveTopology(int32_t inputDescId)
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
        return GetTopologyType(GfxPipelineState::GetInputDescPrimitiveTopology(inputDescId));
    }

    static void SetShaderProgramIfExists(D3D12_SHADER_BYTECODE& s, const ShaderPass* pass, ShaderProgramType type, const ShaderKeywordSet& keywords)
    {
        ShaderProgram* program = pass->GetProgram(type, keywords);

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

    template<size_t Bits>
    static size_t HashBitset(const std::bitset<Bits>& b, size_t hash)
    {
        using word_type = decltype(b._Getword(0));

        constexpr size_t bitsPerWord = sizeof(word_type) * CHAR_BIT;
        constexpr size_t wordCount = (Bits == 0) ? 0 : (Bits - 1) / bitsPerWord + 1;

        for (size_t i = 0; i < wordCount; i++)
        {
            word_type word = b._Getword(i);
            hash = HashHelper::FNV1(&word, 1, hash);
        }

        return hash;
    }

    template<typename T, typename Intermediate>
    static T& ResolveShaderPassVar(ShaderPassVar<T>& v, const std::function<Intermediate(int32_t)>& resolve)
    {
        if (v.IsDynamic)
        {
            v.Value = static_cast<T>(resolve(v.PropertyId));
            v.IsDynamic = false;
        }

        return v.Value;
    }

    size_t ShaderPassRenderState::Resolve(ShaderPassRenderState& state,
        const std::function<bool(int32_t, int32_t*)>& intResolver,
        const std::function<bool(int32_t, float*)>& floatResolver)
    {
        std::function<int32_t(int32_t)> resolveInt = [&intResolver, &floatResolver](int32_t id)
        {
            if (int32_t i = 0; intResolver(id, &i)) return i;
            if (float f = 0.0f; floatResolver(id, &f)) return static_cast<int32_t>(f);
            return 0;
        };

        std::function<bool(int32_t)> resolveBool = [&intResolver, &floatResolver](int32_t id)
        {
            if (int32_t i = 0; intResolver(id, &i)) return i != 0;
            if (float f = 0.0f; floatResolver(id, &f)) return f != 0.0f;
            return false;
        };

        std::function<float(int32_t)> resolveFloat = [&intResolver, &floatResolver](int32_t id)
        {
            if (float f = 0.0f; floatResolver(id, &f)) return f;
            if (int32_t i = 0; intResolver(id, &i)) return static_cast<float>(i);
            return 0.0f;
        };

        size_t hash = HashHelper::FNV1(&ResolveShaderPassVar(state.Cull, resolveInt));

        for (ShaderPassBlendState& blend : state.Blends)
        {
            uint32_t enabled = blend.Enable ? 1 : 0; // bool 不够 4 字节
            hash = HashHelper::FNV1(&enabled, 1, hash);
            hash = HashHelper::FNV1(&ResolveShaderPassVar(blend.WriteMask, resolveInt), 1, hash);
            hash = HashHelper::FNV1(&ResolveShaderPassVar(blend.Rgb.Src, resolveInt), 1, hash);
            hash = HashHelper::FNV1(&ResolveShaderPassVar(blend.Rgb.Dest, resolveInt), 1, hash);
            hash = HashHelper::FNV1(&ResolveShaderPassVar(blend.Rgb.Op, resolveInt), 1, hash);
            hash = HashHelper::FNV1(&ResolveShaderPassVar(blend.Alpha.Src, resolveInt), 1, hash);
            hash = HashHelper::FNV1(&ResolveShaderPassVar(blend.Alpha.Dest, resolveInt), 1, hash);
            hash = HashHelper::FNV1(&ResolveShaderPassVar(blend.Alpha.Op, resolveInt), 1, hash);
        }

        uint32_t depthEnabled = state.DepthState.Enable ? 1 : 0; // bool 不够 4 字节
        hash = HashHelper::FNV1(&depthEnabled, 1, hash);
        uint32_t depthWrite = ResolveShaderPassVar(state.DepthState.Write, resolveBool) ? 1 : 0; // bool 不够 4 字节
        hash = HashHelper::FNV1(&depthWrite, 1, hash);
        hash = HashHelper::FNV1(&ResolveShaderPassVar(state.DepthState.Compare, resolveInt), 1, hash);

        uint32_t stencilEnabled = state.StencilState.Enable ? 1 : 0; // bool 不够 4 字节
        hash = HashHelper::FNV1(&stencilEnabled, 1, hash);
        uint32_t stencilRef = ResolveShaderPassVar(state.StencilState.Ref, resolveInt); // uint8_t 不够 4 字节
        hash = HashHelper::FNV1(&stencilRef, 1, hash);
        uint32_t stencilReadMask = ResolveShaderPassVar(state.StencilState.ReadMask, resolveInt); // uint8_t 不够 4 字节
        hash = HashHelper::FNV1(&stencilReadMask, 1, hash);
        uint32_t stencilWriteMask = ResolveShaderPassVar(state.StencilState.WriteMask, resolveInt); // uint8_t 不够 4 字节
        hash = HashHelper::FNV1(&stencilWriteMask, 1, hash);
        hash = HashHelper::FNV1(&ResolveShaderPassVar(state.StencilState.FrontFace.Compare, resolveInt), 1, hash);
        hash = HashHelper::FNV1(&ResolveShaderPassVar(state.StencilState.FrontFace.PassOp, resolveInt), 1, hash);
        hash = HashHelper::FNV1(&ResolveShaderPassVar(state.StencilState.FrontFace.FailOp, resolveInt), 1, hash);
        hash = HashHelper::FNV1(&ResolveShaderPassVar(state.StencilState.FrontFace.DepthFailOp, resolveInt), 1, hash);
        hash = HashHelper::FNV1(&ResolveShaderPassVar(state.StencilState.BackFace.Compare, resolveInt), 1, hash);
        hash = HashHelper::FNV1(&ResolveShaderPassVar(state.StencilState.BackFace.PassOp, resolveInt), 1, hash);
        hash = HashHelper::FNV1(&ResolveShaderPassVar(state.StencilState.BackFace.FailOp, resolveInt), 1, hash);
        hash = HashHelper::FNV1(&ResolveShaderPassVar(state.StencilState.BackFace.DepthFailOp, resolveInt), 1, hash);

        return hash;
    }

    ID3D12PipelineState* GfxPipelineState::GetGraphicsState(Material* material, int32_t passIndex, int32_t inputDescId, const PipelineStateDesc& stateDesc, size_t stateDescHash)
    {
        Shader* shader = material->GetShader();
        if (shader == nullptr)
        {
            return nullptr;
        }

        ShaderPass* pass = shader->GetPass(passIndex);
        const ShaderKeywordSet& keywords = material->GetKeywords();

        size_t hash = 0;
        const ShaderPassRenderState& rs = material->GetResolvedRenderState(passIndex, &hash);
        hash = HashBitset(keywords.m_Keywords, hash);
        size_t inputDescHash = GetPipelineInputDescHash(inputDescId);
        hash = HashHelper::FNV1(&inputDescHash, 1, hash);
        hash = HashHelper::FNV1(&stateDescHash, 1, hash);

        ComPtr<ID3D12PipelineState>& result = pass->m_PipelineStates[hash];

        if (result == nullptr)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

            psoDesc.pRootSignature = pass->GetRootSignature(keywords);

            SetShaderProgramIfExists(psoDesc.VS, pass, ShaderProgramType::Vertex, keywords);
            SetShaderProgramIfExists(psoDesc.PS, pass, ShaderProgramType::Pixel, keywords);

            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.BlendState.IndependentBlendEnable = rs.Blends.size() > 1 ? TRUE : FALSE;

            for (int i = 0; i < rs.Blends.size(); i++)
            {
                const ShaderPassBlendState& b = rs.Blends[i];
                D3D12_RENDER_TARGET_BLEND_DESC& blendDesc = psoDesc.BlendState.RenderTarget[i];

                blendDesc.BlendEnable = b.Enable;
                blendDesc.LogicOpEnable = FALSE;
                blendDesc.SrcBlend = static_cast<D3D12_BLEND>(static_cast<int>(b.Rgb.Src.Value) + 1);
                blendDesc.DestBlend = static_cast<D3D12_BLEND>(static_cast<int>(b.Rgb.Dest.Value) + 1);
                blendDesc.BlendOp = static_cast<D3D12_BLEND_OP>(static_cast<int>(b.Rgb.Op.Value) + 1);
                blendDesc.SrcBlendAlpha = static_cast<D3D12_BLEND>(static_cast<int>(b.Alpha.Src.Value) + 1);
                blendDesc.DestBlendAlpha = static_cast<D3D12_BLEND>(static_cast<int>(b.Alpha.Dest.Value) + 1);
                blendDesc.BlendOpAlpha = static_cast<D3D12_BLEND_OP>(static_cast<int>(b.Alpha.Op.Value) + 1);
                blendDesc.RenderTargetWriteMask = static_cast<D3D12_COLOR_WRITE_ENABLE>(b.WriteMask.Value);
            }

            psoDesc.SampleMask = UINT_MAX;

            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(static_cast<int>(rs.Cull.Value) + 1);
            psoDesc.RasterizerState.FillMode = stateDesc.Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = rs.DepthState.Enable;
            psoDesc.DepthStencilState.DepthWriteMask = rs.DepthState.Write.Value ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            psoDesc.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(rs.DepthState.Compare.Value) + 1);
            psoDesc.DepthStencilState.StencilEnable = rs.StencilState.Enable;
            psoDesc.DepthStencilState.StencilReadMask = static_cast<UINT8>(rs.StencilState.ReadMask.Value);
            psoDesc.DepthStencilState.StencilWriteMask = static_cast<UINT8>(rs.StencilState.WriteMask.Value);
            psoDesc.DepthStencilState.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rs.StencilState.FrontFace.FailOp.Value) + 1);
            psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rs.StencilState.FrontFace.DepthFailOp.Value) + 1);
            psoDesc.DepthStencilState.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rs.StencilState.FrontFace.PassOp.Value) + 1);
            psoDesc.DepthStencilState.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(rs.StencilState.FrontFace.Compare.Value) + 1);
            psoDesc.DepthStencilState.BackFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rs.StencilState.BackFace.FailOp.Value) + 1);
            psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rs.StencilState.BackFace.DepthFailOp.Value) + 1);
            psoDesc.DepthStencilState.BackFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(static_cast<int>(rs.StencilState.BackFace.PassOp.Value) + 1);
            psoDesc.DepthStencilState.BackFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(rs.StencilState.BackFace.Compare.Value) + 1);
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
            const std::string& debugName = shader->GetName() + " - " + pass->GetName();
            result->SetName(StringUtility::Utf8ToUtf16(debugName).c_str());
#endif

            LOG_TRACE("Create Graphics PSO for '%s' Pass of '%s' Shader", pass->GetName().c_str(), shader->GetName().c_str());
        }

        return result.Get();
    }
}
