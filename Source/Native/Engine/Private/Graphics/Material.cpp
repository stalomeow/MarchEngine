#include "pch.h"
#include "Engine/Graphics/Material.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxTexture.h"
#include "Engine/Graphics/GfxUtils.h"
#include "Engine/Graphics/GfxPipelineState.h"
#include "Engine/Debug.h"
#include <vector>

using namespace DirectX;

namespace march
{
    Material::Material()
        : m_Shader(nullptr)
        , m_ShaderVersion(0)
        , m_KeywordCache{}
        , m_EnabledKeywords{}
        , m_ConstantBuffers{}
        , m_ResolvedRenderStates{}
        , m_Ints{}
        , m_Floats{}
        , m_Vectors{}
        , m_Colors{}
        , m_Textures{}
    {
    }

    Material::Material(Shader* shader) : Material()
    {
        SetShader(shader);
    }

    void Material::Reset()
    {
        m_Shader = nullptr;
        m_ShaderVersion = 0;
        m_KeywordCache.Clear();
        m_EnabledKeywords.clear();
        m_ConstantBuffers.clear();
        m_ResolvedRenderStates.clear();

        m_Ints.clear();
        m_Floats.clear();
        m_Vectors.clear();
        m_Colors.clear();
        m_Textures.clear();
    }

    void Material::SetInt(int32_t id, int32_t value)
    {
        if (auto it = m_Ints.find(id); it != m_Ints.end() && it->second == value)
        {
            return;
        }

        m_Ints[id] = value;
        ClearResolvedRenderStates(); // 解析时用到了 Int 和 Float，强制重新解析
        ClearConstantBuffers(); // 强制重新创建 ConstantBuffer；当前的 ConstantBuffer 可能正被 GPU 使用，不能直接写入新值
    }

    void Material::SetFloat(int32_t id, float value)
    {
        if (auto it = m_Floats.find(id); it != m_Floats.end() && it->second == value)
        {
            return;
        }

        m_Floats[id] = value;
        ClearResolvedRenderStates(); // 解析时用到了 Int 和 Float，强制重新解析
        ClearConstantBuffers(); // 强制重新创建 ConstantBuffer；当前的 ConstantBuffer 可能正被 GPU 使用，不能直接写入新值
    }

    void Material::SetVector(int32_t id, const XMFLOAT4& value)
    {
        if (auto it = m_Vectors.find(id); it != m_Vectors.end() && XMVector4Equal(XMLoadFloat4(&it->second), XMLoadFloat4(&value)))
        {
            return;
        }

        m_Vectors[id] = value;
        ClearConstantBuffers(); // 强制重新创建 ConstantBuffer；当前的 ConstantBuffer 可能正被 GPU 使用，不能直接写入新值
    }

    void Material::SetColor(int32_t id, const XMFLOAT4& value)
    {
        if (auto it = m_Colors.find(id); it != m_Colors.end() && XMVector4Equal(XMLoadFloat4(&it->second), XMLoadFloat4(&value)))
        {
            return;
        }

        m_Colors[id] = value;
        ClearConstantBuffers(); // 强制重新创建 ConstantBuffer；当前的 ConstantBuffer 可能正被 GPU 使用，不能直接写入新值
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

        if (m_Shader != nullptr)
        {
            auto shaderProp = m_Shader->GetProperties().find(id);
            if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Int)
            {
                *outValue = shaderProp->second.DefaultInt;
                return true;
            }
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

        if (m_Shader != nullptr)
        {
            auto shaderProp = m_Shader->GetProperties().find(id);
            if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Float)
            {
                *outValue = shaderProp->second.DefaultFloat;
                return true;
            }
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

        if (m_Shader != nullptr)
        {
            auto shaderProp = m_Shader->GetProperties().find(id);
            if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Vector)
            {
                *outValue = shaderProp->second.DefaultVector;
                return true;
            }
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

        if (m_Shader != nullptr)
        {
            auto shaderProp = m_Shader->GetProperties().find(id);
            if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Color)
            {
                *outValue = shaderProp->second.DefaultColor;
                return true;
            }
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

        if (m_Shader != nullptr)
        {
            auto shaderProp = m_Shader->GetProperties().find(id);
            if (shaderProp != m_Shader->GetProperties().end() && shaderProp->second.Type == ShaderPropertyType::Texture)
            {
                *outValue = shaderProp->second.GetDefaultTexture();
                return true;
            }
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
        RebuildKeywordCache();
        ClearResolvedRenderStates();
        ClearConstantBuffers();
    }

    Shader* Material::GetShader() const
    {
        return m_Shader;
    }

    void Material::SetShader(Shader* pShader)
    {
        if (m_Shader == pShader && (pShader == nullptr || m_ShaderVersion == pShader->GetVersion()))
        {
            return;
        }

        m_Shader = pShader;
        m_ShaderVersion = pShader == nullptr ? 0 : pShader->GetVersion();

        RebuildKeywordCache();
        ClearResolvedRenderStates();
        ClearConstantBuffers();
    }

    const ShaderKeywordSet& Material::GetKeywords()
    {
        CheckShaderVersion();
        return m_KeywordCache;
    }

    void Material::EnableKeyword(const std::string& keyword)
    {
        CheckShaderVersion();

        if (m_EnabledKeywords.insert(keyword).second && m_Shader != nullptr)
        {
            m_KeywordCache.EnableKeyword(m_Shader->GetKeywordSpace(), keyword);
        }
    }

    void Material::DisableKeyword(const std::string& keyword)
    {
        CheckShaderVersion();

        if (m_EnabledKeywords.erase(keyword) == 1 && m_Shader != nullptr)
        {
            m_KeywordCache.DisableKeyword(m_Shader->GetKeywordSpace(), keyword);
        }
    }

    void Material::SetKeyword(const std::string& keyword, bool value)
    {
        if (value) EnableKeyword(keyword);
        else       DisableKeyword(keyword);
    }

    void Material::RebuildKeywordCache()
    {
        m_KeywordCache.Clear();

        if (m_Shader == nullptr)
        {
            return;
        }

        for (const std::string& keyword : m_EnabledKeywords)
        {
            m_KeywordCache.EnableKeyword(m_Shader->GetKeywordSpace(), keyword);
        }
    }

    template<typename T>
    static void SetConstantBufferProperty(GfxRawConstantBuffer& buffer, ShaderPass* pass, int32_t id, const T& value)
    {
        const auto& locations = pass->GetPropertyLocations();
        if (const auto it = locations.find(id); it != locations.end())
        {
            assert(sizeof(T) >= it->second.Size); // 有时候会把 Vector4 绑定到 Vector3 上，所以用 >=
            buffer.SetData(it->second.Offset, &value, it->second.Size);
        }
    }

    GfxRawConstantBuffer* Material::GetConstantBuffer(int32_t passIndex)
    {
        CheckShaderVersion();

        auto it = m_ConstantBuffers.find(passIndex);

        if (it == m_ConstantBuffers.end())
        {
            ShaderPass* pass = m_Shader->GetPass(passIndex);
            uint32_t bufferSizeInBytes = 0;

            for (int32_t i = 0; i < ShaderProgram::NumTypes; i++)
            {
                ShaderProgramType type = static_cast<ShaderProgramType>(i);

                for (int32_t j = 0; j < pass->GetProgramCount(type); j++)
                {
                    ShaderProgram* program = pass->GetProgram(type, j);

                    if (program == nullptr)
                    {
                        continue;
                    }

                    const std::vector<ShaderBuffer>& buffers = program->GetSrvCbvBuffers();
                    auto cbIterator = std::find_if(buffers.begin(), buffers.end(), [](const ShaderBuffer& buf)
                    {
                        return buf.Id == Shader::GetMaterialConstantBufferId();
                    });

                    if (cbIterator != buffers.end() && cbIterator->ConstantBufferSize > 0)
                    {
                        if (bufferSizeInBytes == 0)
                        {
                            bufferSizeInBytes = bufferSizeInBytes = cbIterator->ConstantBufferSize;
                        }
                        else if (bufferSizeInBytes != cbIterator->ConstantBufferSize)
                        {
                            // 同一个 pass 下的所有 program 的 material cbuffer 大小必须一致
                            throw std::runtime_error("Material cbuffer size mismatch");
                        }
                    }
                }
            }

            if (bufferSizeInBytes == 0)
            {
                return nullptr;
            }

            it = m_ConstantBuffers.emplace(passIndex, GfxRawConstantBuffer{ GetGfxDevice() , bufferSizeInBytes, GfxSubAllocator::PersistentUpload }).first;

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
                        SetConstantBufferProperty(it->second, pass, id, value);
                    }
                    break;
                }

                case ShaderPropertyType::Int:
                {
                    int32_t value;
                    if (GetInt(id, &value))
                    {
                        SetConstantBufferProperty(it->second, pass, id, value);
                    }
                    break;
                }

                case ShaderPropertyType::Color:
                {
                    XMFLOAT4 value;
                    if (GetColor(id, &value))
                    {
                        SetConstantBufferProperty(it->second, pass, id, GfxUtils::GetShaderColor(value));
                    }
                    break;
                }

                case ShaderPropertyType::Vector:
                {
                    XMFLOAT4 value;
                    if (GetVector(id, &value))
                    {
                        SetConstantBufferProperty(it->second, pass, id, value);
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
        }

        return &it->second;
    }

    void Material::ClearConstantBuffers()
    {
        m_ConstantBuffers.clear();
    }

    const ShaderPassRenderState& Material::GetResolvedRenderState(int32_t passIndex, size_t* outHash)
    {
        CheckShaderVersion();
        auto it = m_ResolvedRenderStates.find(passIndex);

        if (it == m_ResolvedRenderStates.end())
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

    void Material::ClearResolvedRenderStates()
    {
        m_ResolvedRenderStates.clear();
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

    const std::unordered_set<std::string>& MaterialInternalUtility::GetRawEnabledKeywords(Material* m)
    {
        return m->m_EnabledKeywords;
    }
}
