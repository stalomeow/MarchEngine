#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxPipeline.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Misc/HashUtils.h"

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
        : m_IsDirty(true)
        , m_Hash(0)
        , NumRTV(0)
        , RTVFormats{}
        , DSVFormat(DXGI_FORMAT_UNKNOWN)
        , SampleCount(1)
        , SampleQuality(0)
        , DepthBias(D3D12_DEFAULT_DEPTH_BIAS)
        , DepthBiasClamp(D3D12_DEFAULT_DEPTH_BIAS_CLAMP)
        , SlopeScaledDepthBias(D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS)
        , Wireframe(false)
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
}
