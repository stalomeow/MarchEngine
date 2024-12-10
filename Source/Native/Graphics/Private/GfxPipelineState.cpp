#include "GfxPipelineState.h"
#include "HashUtils.h"
#include "StringUtils.h"
#include "Shader.h"
#include "Material.h"
#include "GfxSettings.h"
#include "GfxUtils.h"
#include "GfxDevice.h"
#include "Debug.h"
#include <wrl.h>

using namespace Microsoft::WRL;

namespace march
{
    GfxInputDesc::GfxInputDesc(D3D12_PRIMITIVE_TOPOLOGY topology, const std::vector<GfxInputElement>& elements)
        : m_PrimitiveTopology(topology), m_Layout{}
    {
        for (const GfxInputElement& input : elements)
        {
            D3D12_INPUT_ELEMENT_DESC& desc = m_Layout.emplace_back();

            switch (input.Semantic)
            {
            case GfxSemantic::Position:
                desc.SemanticName = "POSITION";
                desc.SemanticIndex = 0;
                break;

            case GfxSemantic::Normal:
                desc.SemanticName = "NORMAL";
                desc.SemanticIndex = 0;
                break;

            case GfxSemantic::Tangent:
                desc.SemanticName = "TANGENT";
                desc.SemanticIndex = 0;
                break;

            case GfxSemantic::Color:
                desc.SemanticName = "COLOR";
                desc.SemanticIndex = 0;
                break;

            case GfxSemantic::TexCoord0:
                desc.SemanticName = "TEXCOORD";
                desc.SemanticIndex = 0;
                break;

            case GfxSemantic::TexCoord1:
                desc.SemanticName = "TEXCOORD";
                desc.SemanticIndex = 1;
                break;

            case GfxSemantic::TexCoord2:
                desc.SemanticName = "TEXCOORD";
                desc.SemanticIndex = 2;
                break;

            case GfxSemantic::TexCoord3:
                desc.SemanticName = "TEXCOORD";
                desc.SemanticIndex = 3;
                break;

            case GfxSemantic::TexCoord4:
                desc.SemanticName = "TEXCOORD";
                desc.SemanticIndex = 4;
                break;

            case GfxSemantic::TexCoord5:
                desc.SemanticName = "TEXCOORD";
                desc.SemanticIndex = 5;
                break;

            case GfxSemantic::TexCoord6:
                desc.SemanticName = "TEXCOORD";
                desc.SemanticIndex = 6;
                break;

            case GfxSemantic::TexCoord7:
                desc.SemanticName = "TEXCOORD";
                desc.SemanticIndex = 7;
                break;

            default:
                throw GfxException("Unknown input semantic name");
            }

            desc.Format = input.Format;
            desc.InputSlot = static_cast<UINT>(input.InputSlot);
            desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            desc.InputSlotClass = input.InputSlotClass;
            desc.InstanceDataStepRate = static_cast<UINT>(input.InstanceDataStepRate);
        }

        D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = GetPrimitiveTopologyType();
        m_Hash = HashUtils::FNV1(elements.data(), elements.size());
        m_Hash = HashUtils::FNV1(&topologyType, 1, m_Hash); // PSO 里用的是 D3D12_PRIMITIVE_TOPOLOGY_TYPE
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE GfxInputDesc::GetPrimitiveTopologyType() const
    {
        return GfxUtils::GetTopologyType(m_PrimitiveTopology);
    }

    GfxOutputDesc::GfxOutputDesc()
        : RTVFormats{}
        , DSVFormat(DXGI_FORMAT_UNKNOWN)
        , SampleCount(1)
        , SampleQuality(0)
        , Wireframe(false)
        , m_IsDirty(true)
        , m_Hash(0)
    {
    }

    void GfxOutputDesc::MarkDirty()
    {
        m_IsDirty = true;
    }

    size_t GfxOutputDesc::GetHash() const
    {
        if (m_IsDirty)
        {
            m_IsDirty = false;

            m_Hash = HashUtils::FNV1(RTVFormats.data(), RTVFormats.size());
            m_Hash = HashUtils::FNV1(&DSVFormat, 1, m_Hash);
            m_Hash = HashUtils::FNV1(&SampleCount, 1, m_Hash);
            m_Hash = HashUtils::FNV1(&SampleQuality, 1, m_Hash);

            // bool 不够 4 字节
            uint32_t wireframe = Wireframe ? 1 : 0;
            m_Hash = HashUtils::FNV1(&wireframe, 1, m_Hash);
        }

        return m_Hash;
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

    size_t GfxPipelineState::ResolveShaderPassRenderState(ShaderPassRenderState& state,
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

        size_t hash = HashUtils::FNV1(&ResolveShaderPassVar(state.Cull, resolveInt));

        for (ShaderPassBlendState& blend : state.Blends)
        {
            uint32_t enabled = blend.Enable ? 1 : 0; // bool 不够 4 字节
            hash = HashUtils::FNV1(&enabled, 1, hash);
            hash = HashUtils::FNV1(&ResolveShaderPassVar(blend.WriteMask, resolveInt), 1, hash);
            hash = HashUtils::FNV1(&ResolveShaderPassVar(blend.Rgb.Src, resolveInt), 1, hash);
            hash = HashUtils::FNV1(&ResolveShaderPassVar(blend.Rgb.Dest, resolveInt), 1, hash);
            hash = HashUtils::FNV1(&ResolveShaderPassVar(blend.Rgb.Op, resolveInt), 1, hash);
            hash = HashUtils::FNV1(&ResolveShaderPassVar(blend.Alpha.Src, resolveInt), 1, hash);
            hash = HashUtils::FNV1(&ResolveShaderPassVar(blend.Alpha.Dest, resolveInt), 1, hash);
            hash = HashUtils::FNV1(&ResolveShaderPassVar(blend.Alpha.Op, resolveInt), 1, hash);
        }

        uint32_t depthEnabled = state.DepthState.Enable ? 1 : 0; // bool 不够 4 字节
        hash = HashUtils::FNV1(&depthEnabled, 1, hash);
        uint32_t depthWrite = ResolveShaderPassVar(state.DepthState.Write, resolveBool) ? 1 : 0; // bool 不够 4 字节
        hash = HashUtils::FNV1(&depthWrite, 1, hash);
        hash = HashUtils::FNV1(&ResolveShaderPassVar(state.DepthState.Compare, resolveInt), 1, hash);

        uint32_t stencilEnabled = state.StencilState.Enable ? 1 : 0; // bool 不够 4 字节
        hash = HashUtils::FNV1(&stencilEnabled, 1, hash);
        uint32_t stencilRef = ResolveShaderPassVar(state.StencilState.Ref, resolveInt); // uint8_t 不够 4 字节
        hash = HashUtils::FNV1(&stencilRef, 1, hash);
        uint32_t stencilReadMask = ResolveShaderPassVar(state.StencilState.ReadMask, resolveInt); // uint8_t 不够 4 字节
        hash = HashUtils::FNV1(&stencilReadMask, 1, hash);
        uint32_t stencilWriteMask = ResolveShaderPassVar(state.StencilState.WriteMask, resolveInt); // uint8_t 不够 4 字节
        hash = HashUtils::FNV1(&stencilWriteMask, 1, hash);
        hash = HashUtils::FNV1(&ResolveShaderPassVar(state.StencilState.FrontFace.Compare, resolveInt), 1, hash);
        hash = HashUtils::FNV1(&ResolveShaderPassVar(state.StencilState.FrontFace.PassOp, resolveInt), 1, hash);
        hash = HashUtils::FNV1(&ResolveShaderPassVar(state.StencilState.FrontFace.FailOp, resolveInt), 1, hash);
        hash = HashUtils::FNV1(&ResolveShaderPassVar(state.StencilState.FrontFace.DepthFailOp, resolveInt), 1, hash);
        hash = HashUtils::FNV1(&ResolveShaderPassVar(state.StencilState.BackFace.Compare, resolveInt), 1, hash);
        hash = HashUtils::FNV1(&ResolveShaderPassVar(state.StencilState.BackFace.PassOp, resolveInt), 1, hash);
        hash = HashUtils::FNV1(&ResolveShaderPassVar(state.StencilState.BackFace.FailOp, resolveInt), 1, hash);
        hash = HashUtils::FNV1(&ResolveShaderPassVar(state.StencilState.BackFace.DepthFailOp, resolveInt), 1, hash);

        return hash;
    }

    static void SetShaderProgramIfExists(D3D12_SHADER_BYTECODE& s, ShaderPass* pass, ShaderProgramType type, const ShaderKeywordSet& keywords)
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
        if constexpr (!GfxSettings::UseReversedZBuffer)
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

    ID3D12PipelineState* GfxPipelineState::GetGraphicsPSO(Material* material, int32_t passIndex, const GfxInputDesc& inputDesc, const GfxOutputDesc& outputDesc)
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
        size_t programsHash = pass->GetProgramMatch(keywords).Hash;
        hash = HashUtils::FNV1(&programsHash, 1, hash);
        size_t inputDescHash = inputDesc.GetHash();
        hash = HashUtils::FNV1(&inputDescHash, 1, hash);
        size_t outputDescHash = outputDesc.GetHash();
        hash = HashUtils::FNV1(&outputDescHash, 1, hash);

        ComPtr<ID3D12PipelineState>& result = pass->m_PipelineStates[hash];

        if (result == nullptr)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

            psoDesc.pRootSignature = pass->GetRootSignature(keywords);

            SetShaderProgramIfExists(psoDesc.VS, pass, ShaderProgramType::Vertex, keywords);
            SetShaderProgramIfExists(psoDesc.PS, pass, ShaderProgramType::Pixel, keywords);
            SetShaderProgramIfExists(psoDesc.DS, pass, ShaderProgramType::Domain, keywords);
            SetShaderProgramIfExists(psoDesc.HS, pass, ShaderProgramType::Hull, keywords);
            SetShaderProgramIfExists(psoDesc.GS, pass, ShaderProgramType::Geometry, keywords);

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
            psoDesc.RasterizerState.FillMode = outputDesc.Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

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

            psoDesc.InputLayout.NumElements = static_cast<UINT>(inputDesc.GetLayout().size());
            psoDesc.InputLayout.pInputElementDescs = inputDesc.GetLayout().data();
            psoDesc.PrimitiveTopologyType = inputDesc.GetPrimitiveTopologyType();

            psoDesc.NumRenderTargets = static_cast<UINT>(outputDesc.RTVFormats.size());
            std::copy_n(outputDesc.RTVFormats.data(), outputDesc.RTVFormats.size(), psoDesc.RTVFormats);
            psoDesc.DSVFormat = outputDesc.DSVFormat;

            psoDesc.SampleDesc.Count = static_cast<UINT>(outputDesc.SampleCount);
            psoDesc.SampleDesc.Quality = static_cast<UINT>(outputDesc.SampleQuality);

            ID3D12Device4* device = GetGfxDevice()->GetD3DDevice4();
            GFX_HR(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(result.GetAddressOf())));

#ifdef ENABLE_GFX_DEBUG_NAME
            const std::string& debugName = shader->GetName() + " - " + pass->GetName();
            result->SetName(StringUtils::Utf8ToUtf16(debugName).c_str());
#endif

            LOG_TRACE("Create Graphics PSO for '%s' Pass of '%s' Shader", pass->GetName().c_str(), shader->GetName().c_str());
        }

        return result.Get();
    }
}
