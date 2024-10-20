#include "Material.h"
#include "Shader.h"
#include "Debug.h"
#include "GfxDevice.h"
#include "GfxTexture.h"
#include "GfxHelpers.h"

using namespace DirectX;

namespace march
{
    Material::Material()
        : m_Shader(nullptr)
        , m_ShaderVersion(0)
        , m_ConstantBuffers{}
        , m_Ints{}
        , m_Floats{}
        , m_Vectors{}
        , m_Colors{}
        , m_Textures{}
    {

    }

    void Material::Reset()
    {
        m_Shader = nullptr;
        m_ShaderVersion = 0;
        m_ConstantBuffers.clear();

        m_Ints.clear();
        m_Floats.clear();
        m_Vectors.clear();
        m_Colors.clear();
        m_Textures.clear();
    }

    void Material::SetInt(int32_t id, int32_t value)
    {
        m_Ints[id] = value;
        SetConstantBufferValue(id, value);
    }

    void Material::SetFloat(int32_t id, float value)
    {
        m_Floats[id] = value;
        SetConstantBufferValue(id, value);
    }

    void Material::SetVector(int32_t id, const XMFLOAT4& value)
    {
        m_Vectors[id] = value;
        SetConstantBufferValue(id, value);
    }

    void Material::SetColor(int32_t id, const XMFLOAT4& value)
    {
        m_Colors[id] = value;
        SetConstantBufferValue(id, GfxHelpers::ToShaderColor(value));
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
        SetInt(Shader::GetNameId(name), value);
    }

    void Material::SetFloat(const std::string& name, float value)
    {
        SetFloat(Shader::GetNameId(name), value);
    }

    void Material::SetVector(const std::string& name, const XMFLOAT4& value)
    {
        SetVector(Shader::GetNameId(name), value);
    }

    void Material::SetColor(const std::string& name, const XMFLOAT4& value)
    {
        SetColor(Shader::GetNameId(name), value);
    }

    void Material::SetTexture(const std::string& name, GfxTexture* texture)
    {
        SetTexture(Shader::GetNameId(name), texture);
    }

    bool Material::GetInt(int32_t id, int32_t* outValue) const
    {
        auto matProp = m_Ints.find(id);
        if (matProp != m_Ints.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->GetProperties().find(id);
        if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Int)
        {
            *outValue = shaderProp->second.DefaultValue.Int;
            return true;
        }

        return false;
    }

    bool Material::GetFloat(int32_t id, float* outValue) const
    {
        auto matProp = m_Floats.find(id);
        if (matProp != m_Floats.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->GetProperties().find(id);
        if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Float)
        {
            *outValue = shaderProp->second.DefaultValue.Float;
            return true;
        }

        return false;
    }

    bool Material::GetVector(int32_t id, XMFLOAT4* outValue) const
    {
        auto matProp = m_Vectors.find(id);
        if (matProp != m_Vectors.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->GetProperties().find(id);
        if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Vector)
        {
            *outValue = shaderProp->second.DefaultValue.Vector;
            return true;
        }

        return false;
    }

    bool Material::GetColor(int32_t id, XMFLOAT4* outValue) const
    {
        auto matProp = m_Colors.find(id);
        if (matProp != m_Colors.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->GetProperties().find(id);
        if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Color)
        {
            *outValue = shaderProp->second.DefaultValue.Color;
            return true;
        }

        return false;
    }

    bool Material::GetTexture(int32_t id, GfxTexture** outValue) const
    {
        auto matProp = m_Textures.find(id);
        if (matProp != m_Textures.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->GetProperties().find(id);
        if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Texture)
        {
            *outValue = shaderProp->second.GetDefaultTexture();
            return true;
        }

        return false;
    }

    bool Material::GetInt(const std::string& name, int32_t* outValue) const
    {
        return GetInt(Shader::GetNameId(name), outValue);
    }

    bool Material::GetFloat(const std::string& name, float* outValue) const
    {
        return GetFloat(Shader::GetNameId(name), outValue);
    }

    bool Material::GetVector(const std::string& name, XMFLOAT4* outValue) const
    {
        return GetVector(Shader::GetNameId(name), outValue);
    }

    bool Material::GetColor(const std::string& name, XMFLOAT4* outValue) const
    {
        return GetColor(Shader::GetNameId(name), outValue);
    }

    bool Material::GetTexture(const std::string& name, GfxTexture** outValue) const
    {
        return GetTexture(Shader::GetNameId(name), outValue);
    }

    void Material::CheckShaderVersion()
    {
        if (m_Shader == nullptr)
        {
            return;
        }

        if (m_ShaderVersion == m_Shader->GetVersion())
        {
            return;
        }

        m_ShaderVersion = m_Shader->GetVersion();
        RecreateConstantBuffers();
    }

    void Material::RecreateConstantBuffers()
    {
        DEBUG_LOG_INFO("Recreate material cbuffer");

        m_ConstantBuffers.clear();

        // 创建 cbuffer
        for (int32_t i = 0; i < m_Shader->GetPassCount(); i++)
        {
            ShaderPass* pass = m_Shader->GetPass(i);
            uint32_t cbUnalignedSize = 0;

            for (int32_t j = 0; j < static_cast<int32_t>(ShaderProgramType::NumTypes); j++)
            {
                ShaderProgram* program = pass->GetProgram(static_cast<ShaderProgramType>(j));

                if (program == nullptr)
                {
                    continue;
                }

                const auto& cbMap = program->GetConstantBuffers();
                if (auto it = cbMap.find(Shader::GetMaterialConstantBufferId()); it != cbMap.end())
                {
                    if (cbUnalignedSize == 0)
                    {
                        cbUnalignedSize = it->second.UnalignedSize;
                    }
                    else if (cbUnalignedSize != it->second.UnalignedSize)
                    {
                        // 同一个 pass 下的所有 program 的 material cbuffer 大小必须一致
                        throw std::runtime_error("Material cbuffer size mismatch");
                    }
                }
            }

            if (cbUnalignedSize > 0)
            {
                std::string cbName = pass->GetName() + "ConstantBuffer";
                m_ConstantBuffers[pass] = std::make_unique<GfxConstantBuffer>(GetGfxDevice(), cbName, cbUnalignedSize, 1, false);
            }
        }

        // 初始化 cbuffer
        for (const auto& kv : m_Shader->GetProperties())
        {
            int32_t id = kv.first;
            const ShaderProperty& prop = kv.second;

            switch (prop.Type)
            {
            case ShaderPropertyType::Float:
            {
                float value;
                if (GetFloat(id, &value))
                {
                    SetConstantBufferValue(id, value);
                }
                break;
            }

            case ShaderPropertyType::Int:
            {
                int32_t value;
                if (GetInt(id, &value))
                {
                    SetConstantBufferValue(id, value);
                }
                break;
            }

            case ShaderPropertyType::Color:
            {
                XMFLOAT4 value;
                if (GetColor(id, &value))
                {
                    SetConstantBufferValue(id, GfxHelpers::ToShaderColor(value));
                }
                break;
            }

            case ShaderPropertyType::Vector:
            {
                XMFLOAT4 value;
                if (GetVector(id, &value))
                {
                    SetConstantBufferValue(id, value);
                }
                break;
            }

            case ShaderPropertyType::Texture:
                // Ignore
                break;

            default:
                DEBUG_LOG_ERROR("Unknown shader property type");
                break;
            }
        }
    }

    template<typename T>
    void Material::SetConstantBufferValue(int32_t id, const T& value)
    {
        CheckShaderVersion();

        for (const auto& kv : m_ConstantBuffers)
        {
            const ShaderPass* pass = kv.first;
            const GfxConstantBuffer* cb = kv.second.get();
            auto& locations = pass->GetPropertyLocations();

            if (const auto prop = locations.find(id); prop != locations.end())
            {
                BYTE* p = cb->GetMappedData(0);
                assert(sizeof(value) >= prop->second.Size); // 有时候会把 Vector4 绑定到 Vector3 上，所以用 >=
                memcpy(p + prop->second.Offset, &value, prop->second.Size);
            }
        }
    }

    Shader* Material::GetShader() const
    {
        return m_Shader;
    }

    void Material::SetShader(Shader* pShader)
    {
        if (m_Shader == pShader && m_ShaderVersion == pShader->GetVersion())
        {
            return;
        }

        m_Shader = pShader;

        if (pShader != nullptr)
        {
            m_ShaderVersion = pShader->GetVersion();
            RecreateConstantBuffers();
        }
        else
        {
            m_ShaderVersion = 0;
            m_ConstantBuffers.clear();
        }
    }

    GfxConstantBuffer* Material::GetConstantBuffer(ShaderPass* pass, GfxConstantBuffer* defaultValue)
    {
        CheckShaderVersion();
        auto it = m_ConstantBuffers.find(pass);
        return it == m_ConstantBuffers.end() ? defaultValue : it->second.get();
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
}
