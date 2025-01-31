#include "pch.h"
#include "Engine/Rendering/D3D12Impl/Material.h"
#include "Engine/Rendering/D3D12Impl/GfxBuffer.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxTexture.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Rendering/D3D12Impl/GfxSettings.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Misc/HashUtils.h"
#include "Engine/Debug.h"
#include <vector>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace march
{
    void Material::Reset()
    {
        m_Shader = nullptr;
        m_ShaderVersion = 0;

        m_Keywords.Clear();
        m_IsKeywordDirty = true;

        m_Ints.clear();
        m_Floats.clear();
        m_Vectors.clear();
        m_Colors.clear();
        m_Textures.clear();

        m_ConstantBuffer = nullptr;
        m_IsConstantBufferDirty = true;

        m_ResolvedRenderStates.clear();
        m_ResolvedRenderStateVersion = 0;
    }

    void Material::SetInt(int32_t id, int32_t value)
    {
        if (auto it = m_Ints.find(id); it != m_Ints.end() && it->second == value)
        {
            return;
        }

        m_Ints[id] = value;
        m_IsConstantBufferDirty = true;
        ++m_ResolvedRenderStateVersion; // 解析时用到了 Int 和 Float，强制重新解析
    }

    void Material::SetFloat(int32_t id, float value)
    {
        if (auto it = m_Floats.find(id); it != m_Floats.end() && it->second == value)
        {
            return;
        }

        m_Floats[id] = value;
        m_IsConstantBufferDirty = true;
        ++m_ResolvedRenderStateVersion; // 解析时用到了 Int 和 Float，强制重新解析
    }

    void Material::SetVector(int32_t id, const XMFLOAT4& value)
    {
        if (auto it = m_Vectors.find(id); it != m_Vectors.end() && XMVector4Equal(XMLoadFloat4(&it->second), XMLoadFloat4(&value)))
        {
            return;
        }

        m_Vectors[id] = value;
        m_IsConstantBufferDirty = true;
    }

    void Material::SetColor(int32_t id, const XMFLOAT4& value)
    {
        if (auto it = m_Colors.find(id); it != m_Colors.end() && XMVector4Equal(XMLoadFloat4(&it->second), XMLoadFloat4(&value)))
        {
            return;
        }

        m_Colors[id] = value;
        m_IsConstantBufferDirty = true;
    }

    void Material::SetTexture(int32_t id, GfxTexture* texture)
    {
        if (texture == nullptr)
        {
            m_Textures.erase(id);
        }
        else
        {
            m_Textures[id] = texture;
        }
    }

    void Material::SetInt(const std::string& name, int32_t value)
    {
        SetInt(ShaderUtils::GetIdFromString(name), value);
    }

    void Material::SetFloat(const std::string& name, float value)
    {
        SetFloat(ShaderUtils::GetIdFromString(name), value);
    }

    void Material::SetVector(const std::string& name, const XMFLOAT4& value)
    {
        SetVector(ShaderUtils::GetIdFromString(name), value);
    }

    void Material::SetColor(const std::string& name, const XMFLOAT4& value)
    {
        SetColor(ShaderUtils::GetIdFromString(name), value);
    }

    void Material::SetTexture(const std::string& name, GfxTexture* texture)
    {
        SetTexture(ShaderUtils::GetIdFromString(name), texture);
    }

    bool Material::GetInt(int32_t id, int32_t* outValue) const
    {
        if (auto prop = m_Ints.find(id); prop != m_Ints.end())
        {
            *outValue = prop->second;
            return true;
        }

        if (m_Shader != nullptr)
        {
            const std::unordered_map<int32_t, ShaderProperty>& shaderProps = m_Shader->GetProperties();

            if (auto prop = shaderProps.find(id); prop != shaderProps.end() && prop->second.Type == ShaderPropertyType::Int)
            {
                *outValue = prop->second.DefaultInt;
                return true;
            }
        }

        return false;
    }

    bool Material::GetFloat(int32_t id, float* outValue) const
    {
        if (auto prop = m_Floats.find(id); prop != m_Floats.end())
        {
            *outValue = prop->second;
            return true;
        }

        if (m_Shader != nullptr)
        {
            const std::unordered_map<int32_t, ShaderProperty>& shaderProps = m_Shader->GetProperties();

            if (auto prop = shaderProps.find(id); prop != shaderProps.end() && prop->second.Type == ShaderPropertyType::Float)
            {
                *outValue = prop->second.DefaultFloat;
                return true;
            }
        }

        return false;
    }

    bool Material::GetVector(int32_t id, XMFLOAT4* outValue) const
    {
        if (auto prop = m_Vectors.find(id); prop != m_Vectors.end())
        {
            *outValue = prop->second;
            return true;
        }

        if (m_Shader != nullptr)
        {
            const std::unordered_map<int32_t, ShaderProperty>& shaderProps = m_Shader->GetProperties();

            if (auto prop = shaderProps.find(id); prop != shaderProps.end() && prop->second.Type == ShaderPropertyType::Vector)
            {
                *outValue = prop->second.DefaultVector;
                return true;
            }
        }

        return false;
    }

    bool Material::GetColor(int32_t id, XMFLOAT4* outValue) const
    {
        if (auto prop = m_Colors.find(id); prop != m_Colors.end())
        {
            *outValue = prop->second;
            return true;
        }

        if (m_Shader != nullptr)
        {
            const std::unordered_map<int32_t, ShaderProperty>& shaderProps = m_Shader->GetProperties();

            if (auto prop = shaderProps.find(id); prop != shaderProps.end() && prop->second.Type == ShaderPropertyType::Color)
            {
                *outValue = prop->second.DefaultColor;
                return true;
            }
        }

        return false;
    }

    bool Material::GetTexture(int32_t id, GfxTexture** outValue) const
    {
        if (auto prop = m_Textures.find(id); prop != m_Textures.end())
        {
            *outValue = prop->second;
            return true;
        }

        if (m_Shader != nullptr)
        {
            const std::unordered_map<int32_t, ShaderProperty>& shaderProps = m_Shader->GetProperties();

            if (auto prop = shaderProps.find(id); prop != shaderProps.end() && prop->second.Type == ShaderPropertyType::Texture)
            {
                *outValue = prop->second.GetDefaultTexture();
                return true;
            }
        }

        return false;
    }

    bool Material::GetInt(const std::string& name, int32_t* outValue) const
    {
        return GetInt(ShaderUtils::GetIdFromString(name), outValue);
    }

    bool Material::GetFloat(const std::string& name, float* outValue) const
    {
        return GetFloat(ShaderUtils::GetIdFromString(name), outValue);
    }

    bool Material::GetVector(const std::string& name, XMFLOAT4* outValue) const
    {
        return GetVector(ShaderUtils::GetIdFromString(name), outValue);
    }

    bool Material::GetColor(const std::string& name, XMFLOAT4* outValue) const
    {
        return GetColor(ShaderUtils::GetIdFromString(name), outValue);
    }

    bool Material::GetTexture(const std::string& name, GfxTexture** outValue) const
    {
        return GetTexture(ShaderUtils::GetIdFromString(name), outValue);
    }

    Shader* Material::GetShader() const
    {
        return m_Shader;
    }

    void Material::SetShader(Shader* shader)
    {
        if (m_Shader == shader && (shader == nullptr || m_ShaderVersion == shader->GetVersion()))
        {
            return;
        }

        m_Shader = shader;
        m_ShaderVersion = shader == nullptr ? 0 : shader->GetVersion();
        m_IsKeywordDirty = true;
        m_IsConstantBufferDirty = true;
        m_ResolvedRenderStates.clear();
        m_ResolvedRenderStateVersion = 0;

        if (shader != nullptr)
        {
            m_ResolvedRenderStates.resize(shader->GetPassCount());
        }
    }

    void Material::CheckShaderVersion()
    {
        SetShader(m_Shader);
    }

    void Material::UpdateKeywords()
    {
        CheckShaderVersion();

        if (m_IsKeywordDirty)
        {
            m_Keywords.TransformToSpace(m_Shader ? m_Shader->GetKeywordSpace() : nullptr);
            m_IsKeywordDirty = false;
        }
    }

    const ShaderKeywordSet& Material::GetKeywords()
    {
        UpdateKeywords();
        return m_Keywords.GetKeywords();
    }

    void Material::SetKeyword(int32_t id, bool value)
    {
        UpdateKeywords();
        m_Keywords.SetKeyword(id, value);
    }

    void Material::EnableKeyword(int32_t id)
    {
        SetKeyword(id, true);
    }

    void Material::DisableKeyword(int32_t id)
    {
        SetKeyword(id, false);
    }

    void Material::SetKeyword(const std::string& keyword, bool value)
    {
        SetKeyword(ShaderUtils::GetIdFromString(keyword), value);
    }

    void Material::EnableKeyword(const std::string& keyword)
    {
        EnableKeyword(ShaderUtils::GetIdFromString(keyword));
    }

    void Material::DisableKeyword(const std::string& keyword)
    {
        DisableKeyword(ShaderUtils::GetIdFromString(keyword));
    }

    template<typename T>
    static void SetConstantBufferProperty(uint8_t* p, Shader* shader, int32_t id, const T& value)
    {
        const auto& locations = shader->GetPropertyLocations();
        if (const auto it = locations.find(id); it != locations.end())
        {
            assert(sizeof(T) >= it->second.Size); // 有时候会把 Vector4 绑定到 Vector3 上，所以用 >=
            memcpy(p + it->second.Offset, &value, it->second.Size);
        }
    }

    GfxBuffer* Material::GetConstantBuffer(size_t passIndex)
    {
        CheckShaderVersion();

        if (m_Shader == nullptr || m_Shader->GetMaterialConstantBufferSize() == 0)
        {
            return nullptr;
        }

        if (m_ConstantBuffer == nullptr)
        {
            m_ConstantBuffer = std::make_unique<GfxBuffer>(GetGfxDevice(), "MaterialConstantBuffer");
            m_IsConstantBufferDirty = true;
        }

        if (m_IsConstantBufferDirty)
        {
            uint32_t size = m_Shader->GetMaterialConstantBufferSize();
            std::vector<uint8_t> data(size);

            // 初始化 cbuffer
            for (const auto& [id, prop] : m_Shader->GetProperties())
            {
                switch (prop.Type)
                {
                case ShaderPropertyType::Float:
                {
                    float value;
                    if (GetFloat(id, &value))
                    {
                        SetConstantBufferProperty(data.data(), m_Shader, id, value);
                    }
                    break;
                }

                case ShaderPropertyType::Int:
                {
                    int32_t value;
                    if (GetInt(id, &value))
                    {
                        SetConstantBufferProperty(data.data(), m_Shader, id, value);
                    }
                    break;
                }

                case ShaderPropertyType::Color:
                {
                    XMFLOAT4 value;
                    if (GetColor(id, &value))
                    {
                        SetConstantBufferProperty(data.data(), m_Shader, id, GfxUtils::GetShaderColor(value));
                    }
                    break;
                }

                case ShaderPropertyType::Vector:
                {
                    XMFLOAT4 value;
                    if (GetVector(id, &value))
                    {
                        SetConstantBufferProperty(data.data(), m_Shader, id, value);
                    }
                    break;
                }

                case ShaderPropertyType::Texture:
                    // Ignore
                    break;

                default:
                    LOG_ERROR("Unknown shader property type");
                    break;
                }
            }

            GfxBufferDesc desc{};
            desc.Stride = size;
            desc.Count = 1;
            desc.Usages = GfxBufferUsages::Constant;
            desc.Flags = GfxBufferFlags::Dynamic;

            m_ConstantBuffer->SetData(desc, data.data());
            m_IsConstantBufferDirty = false;
        }

        return m_ConstantBuffer.get();
    }

    template<typename T, typename ResolveFunc>
    static T& ResolveShaderPassVar(ShaderPassVar<T>& v, ResolveFunc resolveFn)
    {
        if (v.IsDynamic)
        {
            v.Value = static_cast<T>(resolveFn(v.PropertyId));
            v.IsDynamic = false;
        }

        return v.Value;
    }

    const ShaderPassRenderState& Material::GetResolvedRenderState(size_t passIndex, size_t* outHash)
    {
        CheckShaderVersion();
        ResolvedRenderState& rrs = m_ResolvedRenderStates[passIndex];

        if (!rrs.State || rrs.Version != m_ResolvedRenderStateVersion)
        {
            auto resolveInt = [this](int32_t id)
            {
                if (int32_t i = 0; GetInt(id, &i)) return i;
                if (float f = 0.0f; GetFloat(id, &f)) return static_cast<int32_t>(f);
                return 0;
            };

            auto resolveBool = [this](int32_t id)
            {
                if (int32_t i = 0; GetInt(id, &i)) return i != 0;
                if (float f = 0.0f; GetFloat(id, &f)) return f != 0.0f;
                return false;
            };

            auto resolveFloat = [this](int32_t id)
            {
                if (float f = 0.0f; GetFloat(id, &f)) return f;
                if (int32_t i = 0; GetInt(id, &i)) return static_cast<float>(i);
                return 0.0f;
            };

            DefaultHash hash{};
            ShaderPassRenderState rs = m_Shader->GetPass(passIndex)->GetRenderState(); // 拷贝一份

            hash << ResolveShaderPassVar(rs.Cull, resolveInt);

            for (ShaderPassBlendState& blend : rs.Blends)
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
                << rs.DepthState.Enable
                << ResolveShaderPassVar(rs.DepthState.Write, resolveBool)
                << ResolveShaderPassVar(rs.DepthState.Compare, resolveInt)
                << rs.StencilState.Enable
                << ResolveShaderPassVar(rs.StencilState.Ref, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.ReadMask, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.WriteMask, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.FrontFace.Compare, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.FrontFace.PassOp, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.FrontFace.FailOp, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.FrontFace.DepthFailOp, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.BackFace.Compare, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.BackFace.PassOp, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.BackFace.FailOp, resolveInt)
                << ResolveShaderPassVar(rs.StencilState.BackFace.DepthFailOp, resolveInt);

            rrs.State = rs;
            rrs.Hash = *hash;
            rrs.Version = m_ResolvedRenderStateVersion;
        }

        if (outHash != nullptr) *outHash = rrs.Hash;
        return rrs.State.value();
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

    ID3D12PipelineState* Material::GetPSO(size_t passIndex, const GfxInputDesc& inputDesc, const GfxOutputDesc& outputDesc)
    {
        if (m_Shader == nullptr)
        {
            return nullptr;
        }

        ShaderPass* pass = m_Shader->GetPass(passIndex);
        const ShaderKeywordSet& keywords = GetKeywords();

        size_t renderStateHash = 0;
        const ShaderPassRenderState& rs = GetResolvedRenderState(passIndex, &renderStateHash);

        DefaultHash hash{};
        hash.Append(renderStateHash);
        hash.Append(pass->GetProgramMatch(keywords).Hash);
        hash.Append(inputDesc.GetHash());
        hash.Append(outputDesc.GetHash());

        ComPtr<ID3D12PipelineState>& result = pass->m_PipelineStates[*hash];

        if (result == nullptr)
        {
            auto setProgramIfExists = [pass, keywords](D3D12_SHADER_BYTECODE& s, ShaderProgramType type)
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
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = pass->GetRootSignature(keywords)->GetD3DRootSignature();
            setProgramIfExists(psoDesc.VS, ShaderProgramType::Vertex);
            setProgramIfExists(psoDesc.PS, ShaderProgramType::Pixel);
            setProgramIfExists(psoDesc.DS, ShaderProgramType::Domain);
            setProgramIfExists(psoDesc.HS, ShaderProgramType::Hull);
            setProgramIfExists(psoDesc.GS, ShaderProgramType::Geometry);

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
            GfxUtils::SetName(result.Get(), m_Shader->GetName() + " - " + pass->GetName());

            LOG_TRACE("Create Graphics PSO for '{}' Pass of '{}' Shader", pass->GetName(), m_Shader->GetName());
        }

        return result.Get();
    }

    const std::unordered_map<int32_t, int32_t>& MaterialInternalUtility::GetRawInts(Material* m)
    {
        return m->m_Ints;
    }

    const std::unordered_map<int32_t, float>& MaterialInternalUtility::GetRawFloats(Material* m)
    {
        return m->m_Floats;
    }

    const std::unordered_map<int32_t, XMFLOAT4>& MaterialInternalUtility::GetRawVectors(Material* m)
    {
        return m->m_Vectors;
    }

    const std::unordered_map<int32_t, XMFLOAT4>& MaterialInternalUtility::GetRawColors(Material* m)
    {
        return m->m_Colors;
    }

    const std::unordered_map<int32_t, GfxTexture*>& MaterialInternalUtility::GetRawTextures(Material* m)
    {
        return m->m_Textures;
    }

    std::vector<std::string> MaterialInternalUtility::GetRawEnabledKeywords(Material* m)
    {
        return m->m_Keywords.GetEnabledKeywordStrings();
    }
}
