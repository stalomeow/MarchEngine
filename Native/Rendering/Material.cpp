#include "Rendering/Material.h"
#include "Core/StringUtility.h"

namespace dx12demo
{
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

    void Material::SetVector(const std::string& name, DirectX::XMFLOAT4 value)
    {
        m_Vectors[name] = value;
        SetConstantBufferValue(name, value);
    }

    void Material::SetTexture(const std::string& name, Texture* texture)
    {
        m_Textures[name] = texture;
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

    bool Material::GetVector(const std::string& name, DirectX::XMFLOAT4* outValue) const
    {
        auto matProp = m_Vectors.find(name);
        if (matProp != m_Vectors.end())
        {
            *outValue = matProp->second;
            return true;
        }

        auto shaderProp = m_Shader->Properties.find(name);
        if (shaderProp != m_Shader->Properties.end())
        {
            if (shaderProp->second.Type == ShaderPropertyType::Vector)
            {
                *outValue = shaderProp->second.DefaultVector;
                return true;
            }

            if (shaderProp->second.Type == ShaderPropertyType::Color)
            {
                *outValue = shaderProp->second.DefaultColor;
                return true;
            }
        }

        return false;
    }

    bool Material::GetTexture(const std::string& name, Texture** outValue) const
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

    void Material::SetShader(Shader* pShader)
    {
        if (m_Shader == pShader)
        {
            return;
        }

        m_Shader = pShader;

        // 创建 cbuffer
        m_ConstantBuffers.clear();
        for (auto& pass : pShader->Passes)
        {
            auto matCb = pass.ConstantBuffers.find(ShaderPass::MaterialCbName);
            if (matCb == pass.ConstantBuffers.end())
            {
                continue;
            }

            std::wstring cbName = StringUtility::Utf8ToUtf16(pass.Name + "ConstantBuffer");
            m_ConstantBuffers[&pass] = std::make_unique<ConstantBuffer>(cbName, matCb->second.Size, 1, false);
        }

        // 初始化 cbuffer
        for (const auto& kv : pShader->Properties)
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
            case ShaderPropertyType::Vector:
            {
                DirectX::XMFLOAT4 value;
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
}
