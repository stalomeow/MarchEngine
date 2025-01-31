#include "pch.h"
#include "Engine/Graphics/GfxPipelineState.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/Material.h"
#include "Engine/Graphics/GfxSettings.h"
#include "Engine/Graphics/GfxUtils.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/HashUtils.h"
#include "Engine/Debug.h"
#include <wrl.h>

using namespace Microsoft::WRL;

namespace march
{
    GfxInputDesc::GfxInputDesc(D3D12_PRIMITIVE_TOPOLOGY topology, const std::vector<GfxInputElement>& elements)
        : m_PrimitiveTopology(topology), m_Layout{}
    {
        DefaultHash hash{};

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

            hash.Append(input);
        }

        hash.Append(GetPrimitiveTopologyType()); // PSO 里用的是 D3D12_PRIMITIVE_TOPOLOGY_TYPE
        m_Hash = *hash;
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE GfxInputDesc::GetPrimitiveTopologyType() const
    {
        return GfxUtils::GetTopologyType(m_PrimitiveTopology);
    }

    GfxOutputDesc::GfxOutputDesc()
        : NumRTV(0)
        , RTVFormats{}
        , DSVFormat(DXGI_FORMAT_UNKNOWN)
        , SampleCount(1)
        , SampleQuality(0)
        , DepthBias(D3D12_DEFAULT_DEPTH_BIAS)
        , DepthBiasClamp(D3D12_DEFAULT_DEPTH_BIAS_CLAMP)
        , SlopeScaledDepthBias(D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS)
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
            DefaultHash hash{};
            hash.Append(RTVFormats, sizeof(DXGI_FORMAT) * static_cast<size_t>(NumRTV));
            hash.Append(DSVFormat);
            hash.Append(SampleCount);
            hash.Append(SampleQuality);
            hash.Append(DepthBias);
            hash.Append(DepthBiasClamp);
            hash.Append(SlopeScaledDepthBias);
            hash.Append(Wireframe);

            m_Hash = *hash;
            m_IsDirty = false;
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

        DefaultHash hash{};
        hash << ResolveShaderPassVar(state.Cull, resolveInt);

        for (ShaderPassBlendState& blend : state.Blends)
        {
            hash
                << blend.Enable
                << ResolveShaderPassVar(blend.WriteMask, resolveInt)
                << ResolveShaderPassVar(blend.Rgb.Src, resolveInt)
                << ResolveShaderPassVar(blend.Rgb.Dest, resolveInt)
                << ResolveShaderPassVar(blend.Rgb.Op, resolveInt)
                << ResolveShaderPassVar(blend.Alpha.Src, resolveInt)
                << ResolveShaderPassVar(blend.Alpha.Dest, resolveInt)
                << ResolveShaderPassVar(blend.Alpha.Op, resolveInt);
        }

        hash
            << state.DepthState.Enable
            << ResolveShaderPassVar(state.DepthState.Write, resolveBool)
            << ResolveShaderPassVar(state.DepthState.Compare, resolveInt)
            << state.StencilState.Enable
            << ResolveShaderPassVar(state.StencilState.Ref, resolveInt)
            << ResolveShaderPassVar(state.StencilState.ReadMask, resolveInt)
            << ResolveShaderPassVar(state.StencilState.WriteMask, resolveInt)
            << ResolveShaderPassVar(state.StencilState.FrontFace.Compare, resolveInt)
            << ResolveShaderPassVar(state.StencilState.FrontFace.PassOp, resolveInt)
            << ResolveShaderPassVar(state.StencilState.FrontFace.FailOp, resolveInt)
            << ResolveShaderPassVar(state.StencilState.FrontFace.DepthFailOp, resolveInt)
            << ResolveShaderPassVar(state.StencilState.BackFace.Compare, resolveInt)
            << ResolveShaderPassVar(state.StencilState.BackFace.PassOp, resolveInt)
            << ResolveShaderPassVar(state.StencilState.BackFace.FailOp, resolveInt)
            << ResolveShaderPassVar(state.StencilState.BackFace.DepthFailOp, resolveInt);

        return *hash;
    }

    template <size_t NumProgramTypes, typename T>
    static void SetShaderProgramIfExists(
        D3D12_SHADER_BYTECODE& s,
        ShaderProgramGroup<NumProgramTypes>* programGroup,
        T type,
        const ShaderKeywordSet& keywords)
    {
        ShaderProgram* program = programGroup->GetProgram(type, keywords);

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

    static __forceinline void ApplyReversedZBuffer(D3D12_RASTERIZER_DESC& raster)
    {
        if constexpr (!GfxSettings::UseReversedZBuffer)
        {
            return;
        }

        raster.DepthBias = -raster.DepthBias;
        raster.DepthBiasClamp = -raster.DepthBiasClamp;
        raster.SlopeScaledDepthBias = -raster.SlopeScaledDepthBias;
    }

    static __forceinline void ApplyReversedZBuffer(D3D12_DEPTH_STENCIL_DESC& depthStencil)
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

    ID3D12PipelineState* GfxPipelineState::GetGraphicsPSO(Material* material, size_t passIndex, const GfxInputDesc& inputDesc, const GfxOutputDesc& outputDesc)
    {
        Shader* shader = material->GetShader();
        if (shader == nullptr)
        {
            return nullptr;
        }

        ShaderPass* pass = shader->GetPass(passIndex);
        const ShaderKeywordSet& keywords = material->GetKeywords();

        size_t renderStateHash = 0;
        const ShaderPassRenderState& rs = material->GetResolvedRenderState(passIndex, &renderStateHash);

        DefaultHash hash{};
        hash.Append(renderStateHash);
        hash.Append(pass->GetProgramMatch(keywords).Hash);
        hash.Append(inputDesc.GetHash());
        hash.Append(outputDesc.GetHash());

        ComPtr<ID3D12PipelineState>& result = pass->m_PipelineStates[*hash];

        if (result == nullptr)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

            psoDesc.pRootSignature = pass->GetRootSignature(keywords)->GetD3DRootSignature();

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
            psoDesc.RasterizerState.DepthBias = static_cast<INT>(outputDesc.DepthBias);
            psoDesc.RasterizerState.DepthBiasClamp = outputDesc.DepthBiasClamp;
            psoDesc.RasterizerState.SlopeScaledDepthBias = outputDesc.SlopeScaledDepthBias;
            ApplyReversedZBuffer(psoDesc.RasterizerState);

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

            psoDesc.NumRenderTargets = static_cast<UINT>(outputDesc.NumRTV);
            std::copy_n(outputDesc.RTVFormats, static_cast<size_t>(outputDesc.NumRTV), psoDesc.RTVFormats);
            psoDesc.DSVFormat = outputDesc.DSVFormat;

            psoDesc.SampleDesc.Count = static_cast<UINT>(outputDesc.SampleCount);
            psoDesc.SampleDesc.Quality = static_cast<UINT>(outputDesc.SampleQuality);

            ID3D12Device4* device = GetGfxDevice()->GetD3DDevice4();
            GFX_HR(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(result.GetAddressOf())));
            GfxUtils::SetName(result.Get(), shader->GetName() + " - " + pass->GetName());

            LOG_TRACE("Create Graphics PSO for '{}' Pass of '{}' Shader", pass->GetName(), shader->GetName());
        }

        return result.Get();
    }

    ID3D12PipelineState* GfxPipelineState::GetComputePSO(ComputeShader* shader, ComputeShaderKernel* kernel, const ShaderKeywordSet& keywords)
    {
        size_t hash = kernel->GetProgramMatch(keywords).Hash;
        ComPtr<ID3D12PipelineState>& result = kernel->m_PipelineStates[hash];

        if (result == nullptr)
        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = kernel->GetRootSignature(keywords)->GetD3DRootSignature();
            SetShaderProgramIfExists(psoDesc.CS, kernel, 0, keywords);

            ID3D12Device4* device = GetGfxDevice()->GetD3DDevice4();
            GFX_HR(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(result.GetAddressOf())));
            GfxUtils::SetName(result.Get(), shader->GetName() + " - " + kernel->GetName());

            LOG_TRACE("Create Compute PSO for '{}' Kernel of '{}' Shader", kernel->GetName(), shader->GetName());
        }

        return result.Get();
    }
}
