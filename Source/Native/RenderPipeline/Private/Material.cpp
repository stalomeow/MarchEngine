#include "Material.h"
#include "Shader.h"
#include "Debug.h"
#include "GfxDevice.h"
#include "GfxTexture.h"

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

    void Material::SetInt(const std::string& name, int32_t value)
    {
        m_Ints[name] = value;
        SetConstantBufferValue(name, value);
    }

    void Material::SetFloat(const std::string& name, float value)
    {
        m_Floats[name] = value;
        SetConstantBufferValue(name, value);
    }

    void Material::SetVector(const std::string& name, const XMFLOAT4& value)
    {
        m_Vectors[name] = value;
        SetConstantBufferValue(name, value);
    }

    void Material::SetColor(const std::string& name, const XMFLOAT4& value)
    {
        m_Colors[name] = value;
        SetConstantBufferValue(name, value);
    }

    void Material::SetTexture(const std::string& name, GfxTexture* texture)
    {
        if (texture == nullptr)
        {
            m_Textures.erase(name);
        }
        else
        {
            m_Textures[name] = texture;
        }
    }

    bool Material::GetInt(const std::string& name, int32_t* outValue) const
    {
        auto matProp = m_Ints.find(name);
        if (matProp != m_Ints.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->Properties.find(name);
        if (shaderProp != m_Shader->Properties.end() && shaderProp->second.Type == ShaderPropertyType::Int)
        {
            *outValue = shaderProp->second.DefaultInt;
            return true;
        }

        return false;
    }

    bool Material::GetFloat(const std::string& name, float* outValue) const
    {
        auto matProp = m_Floats.find(name);
        if (matProp != m_Floats.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->Properties.find(name);
        if (shaderProp != m_Shader->Properties.end() && shaderProp->second.Type == ShaderPropertyType::Float)
        {
            *outValue = shaderProp->second.DefaultFloat;
            return true;
        }

        return false;
    }

    bool Material::GetVector(const std::string& name, XMFLOAT4* outValue) const
    {
        auto matProp = m_Vectors.find(name);
        if (matProp != m_Vectors.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->Properties.find(name);
        if (shaderProp != m_Shader->Properties.end() && shaderProp->second.Type == ShaderPropertyType::Vector)
        {
            *outValue = shaderProp->second.DefaultVector;
            return true;
        }

        return false;
    }

    bool Material::GetColor(const std::string& name, XMFLOAT4* outValue) const
    {
        auto matProp = m_Colors.find(name);
        if (matProp != m_Colors.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->Properties.find(name);
        if (shaderProp != m_Shader->Properties.end() && shaderProp->second.Type == ShaderPropertyType::Color)
        {
            *outValue = shaderProp->second.DefaultColor;
            return true;
        }

        return false;
    }

    bool Material::GetTexture(const std::string& name, GfxTexture** outValue) const
    {
        auto matProp = m_Textures.find(name);
        if (matProp != m_Textures.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->Properties.find(name);
        if (shaderProp != m_Shader->Properties.end() && shaderProp->second.Type == ShaderPropertyType::Texture)
        {
            *outValue = shaderProp->second.GetDefaultTexture();
            return true;
        }

        return false;
    }

    void Material::CheckShaderVersion()
    {
        if (m_Shader == nullptr)
        {
            return;
        }

        if (m_ShaderVersion == m_Shader->Version)
        {
            return;
        }

        m_ShaderVersion = m_Shader->Version;
        RecreateConstantBuffers();
    }

    void Material::RecreateConstantBuffers()
    {
        DEBUG_LOG_INFO("Recreate material cbuffer");

        // 创建 cbuffer
        m_ConstantBuffers.clear();
        for (auto& pass : m_Shader->Passes)
        {
            auto matCb = pass->ConstantBuffers.find(ShaderPass::MaterialCbName);
            if (matCb == pass->ConstantBuffers.end())
            {
                continue;
            }

            std::string cbName = pass->Name + "ConstantBuffer";
            m_ConstantBuffers[pass.get()] = std::make_unique<GfxConstantBuffer>(GetGfxDevice(), cbName, matCb->second.Size, 1, false);
        }

        // 初始化 cbuffer
        for (const auto& kv : m_Shader->Properties)
        {
            const std::string& name = kv.first;
            const ShaderProperty& prop = kv.second;

            switch (prop.Type)
            {
            case ShaderPropertyType::Float:
            {
                float value;
                if (GetFloat(name, &value))
                {
                    SetConstantBufferValue(name, value);
                }
                break;
            }

            case ShaderPropertyType::Int:
            {
                int32_t value;
                if (GetInt(name, &value))
                {
                    SetConstantBufferValue(name, value);
                }
                break;
            }

            case ShaderPropertyType::Color:
            {
                XMFLOAT4 value;
                if (GetColor(name, &value))
                {
                    SetConstantBufferValue(name, value);
                }
                break;
            }

            case ShaderPropertyType::Vector:
            {
                XMFLOAT4 value;
                if (GetVector(name, &value))
                {
                    SetConstantBufferValue(name, value);
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
    void Material::SetConstantBufferValue(const std::string& name, const T& value)
    {
        CheckShaderVersion();

        for (const auto& kv : m_ConstantBuffers)
        {
            const ShaderPass* pass = kv.first;
            const GfxConstantBuffer* cb = kv.second.get();
            const auto prop = pass->MaterialProperties.find(name);

            if (prop != pass->MaterialProperties.end())
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
        if (m_Shader == pShader && m_ShaderVersion == pShader->Version)
        {
            return;
        }

        m_Shader = pShader;

        if (pShader != nullptr)
        {
            m_ShaderVersion = pShader->Version;
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

    const std::unordered_map<std::string, int32_t>& MaterialInternalUtility::GetRawInts(Material* m)
    {
        return m->m_Ints;
    }

    const std::unordered_map<std::string, float>& MaterialInternalUtility::GetRawFloats(Material* m)
    {
        return m->m_Floats;
    }

    const std::unordered_map<std::string, XMFLOAT4>& MaterialInternalUtility::GetRawVectors(Material* m)
    {
        return m->m_Vectors;
    }

    const std::unordered_map<std::string, XMFLOAT4>& MaterialInternalUtility::GetRawColors(Material* m)
    {
        return m->m_Colors;
    }

    const std::unordered_map<std::string, GfxTexture*>& MaterialInternalUtility::GetRawTextures(Material* m)
    {
        return m->m_Textures;
    }
}
