#include "pch.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Graphics/GfxBuffer.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxTexture.h"
#include "Engine/Graphics/GfxUtils.h"
#include "Engine/Graphics/GfxPipelineState.h"
#include "Engine/Debug.h"
#include <vector>

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

        m_PerPassData.clear();
        m_ConstantBufferVersion = 0;
        m_ResolvedRenderStateVersion = 0;
    }

    void Material::SetInt(int32_t id, int32_t value)
    {
        if (auto it = m_Ints.find(id); it != m_Ints.end() && it->second == value)
        {
            return;
        }

        m_Ints[id] = value;
        ++m_ConstantBufferVersion;
        ++m_ResolvedRenderStateVersion; // 解析时用到了 Int 和 Float，强制重新解析
    }

    void Material::SetFloat(int32_t id, float value)
    {
        if (auto it = m_Floats.find(id); it != m_Floats.end() && it->second == value)
        {
            return;
        }

        m_Floats[id] = value;
        ++m_ConstantBufferVersion;
        ++m_ResolvedRenderStateVersion; // 解析时用到了 Int 和 Float，强制重新解析
    }

    void Material::SetVector(int32_t id, const XMFLOAT4& value)
    {
        if (auto it = m_Vectors.find(id); it != m_Vectors.end() && XMVector4Equal(XMLoadFloat4(&it->second), XMLoadFloat4(&value)))
        {
            return;
        }

        m_Vectors[id] = value;
        ++m_ConstantBufferVersion;
    }

    void Material::SetColor(int32_t id, const XMFLOAT4& value)
    {
        if (auto it = m_Colors.find(id); it != m_Colors.end() && XMVector4Equal(XMLoadFloat4(&it->second), XMLoadFloat4(&value)))
        {
            return;
        }

        m_Colors[id] = value;
        ++m_ConstantBufferVersion;
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
        m_PerPassData.clear();
        m_ConstantBufferVersion = 0;
        m_ResolvedRenderStateVersion = 0;

        if (shader != nullptr)
        {
            m_PerPassData.resize(shader->GetPassCount());
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
    static void SetConstantBufferProperty(uint8_t* p, ShaderPass* pass, int32_t id, const T& value)
    {
        const auto& locations = pass->GetPropertyLocations();
        if (const auto it = locations.find(id); it != locations.end())
        {
            assert(sizeof(T) >= it->second.Size); // 有时候会把 Vector4 绑定到 Vector3 上，所以用 >=
            memcpy(p + it->second.Offset, &value, it->second.Size);
        }
    }

    GfxBuffer* Material::GetConstantBuffer(size_t passIndex)
    {
        CheckShaderVersion();
        PerPassData& passData = m_PerPassData[passIndex];
        bool isNewBuffer = false;

        if (passData.ConstantBuffer == nullptr)
        {
            ShaderPass* pass = m_Shader->GetPass(passIndex);
            std::optional<uint32_t> size = pass->GetMaterialConstantBufferSize();

            if (!size)
            {
                return nullptr;
            }

            passData.ConstantBuffer = std::make_unique<GfxBuffer>(GetGfxDevice(), "MaterialConstantBuffer");
            isNewBuffer = true;
        }

        if (isNewBuffer || passData.ConstantBufferVersion != m_ConstantBufferVersion)
        {
            ShaderPass* pass = m_Shader->GetPass(passIndex);
            uint32_t size = *pass->GetMaterialConstantBufferSize();
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
                        SetConstantBufferProperty(data.data(), pass, id, value);
                    }
                    break;
                }

                case ShaderPropertyType::Int:
                {
                    int32_t value;
                    if (GetInt(id, &value))
                    {
                        SetConstantBufferProperty(data.data(), pass, id, value);
                    }
                    break;
                }

                case ShaderPropertyType::Color:
                {
                    XMFLOAT4 value;
                    if (GetColor(id, &value))
                    {
                        SetConstantBufferProperty(data.data(), pass, id, GfxUtils::GetShaderColor(value));
                    }
                    break;
                }

                case ShaderPropertyType::Vector:
                {
                    XMFLOAT4 value;
                    if (GetVector(id, &value))
                    {
                        SetConstantBufferProperty(data.data(), pass, id, value);
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

            passData.ConstantBuffer->SetData(desc, data.data());
            passData.ConstantBufferVersion = m_ConstantBufferVersion;
        }

        return passData.ConstantBuffer.get();
    }

    const ShaderPassRenderState& Material::GetResolvedRenderState(size_t passIndex, size_t* outHash)
    {
        CheckShaderVersion();
        PerPassData& passData = m_PerPassData[passIndex];

        if (!passData.ResolvedRenderState)
        {
            ShaderPassRenderState rs = m_Shader->GetPass(passIndex)->GetRenderState(); // 拷贝一份
            size_t hash = GfxPipelineState::ResolveShaderPassRenderState(rs,
                [this](int32_t id, int32_t* outInt) { return GetInt(id, outInt); },
                [this](int32_t id, float* outFloat) { return GetFloat(id, outFloat); });
            it = m_ResolvedRenderStates.emplace(passIndex, std::make_pair(rs, hash)).first;
        }

        if (outHash != nullptr)
        {
            *outHash = it->second.second;
        }

        return it->second.first;
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

    const std::vector<std::string>& MaterialInternalUtility::GetRawEnabledKeywords(Material* m)
    {
        return m->m_Keywords.GetEnabledKeywordStrings();
    }
}
