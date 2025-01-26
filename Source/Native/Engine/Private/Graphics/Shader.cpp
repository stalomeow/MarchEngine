#include "pch.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxTexture.h"
#include "Engine/Debug.h"
#include "Engine/PathUtils.h"

using namespace DirectX;

namespace march
{
    GfxTexture* ShaderProperty::GetDefaultTexture() const
    {
        if (Type != ShaderPropertyType::Texture)
        {
            throw GfxException("Property is not a texture type");
        }

        return GfxTexture::GetDefault(DefaultTexture, TextureDimension);
    }

    ShaderPass* Shader::GetPass(size_t index) const
    {
        if (index >= m_Passes.size())
        {
            throw GfxException("Invalid pass index");
        }

        return m_Passes[index].get();
    }

    std::optional<size_t> Shader::GetFirstPassIndexWithTagValue(const std::string& tag, const std::string& value) const
    {
        for (size_t i = 0; i < m_Passes.size(); i++)
        {
            const std::unordered_map<std::string, std::string>& tags = m_Passes[i]->GetTags();

            if (auto it = tags.find(tag); it != tags.end() && it->second == value)
            {
                return i;
            }
        }

        return std::nullopt;
    }

    ShaderPass* Shader::GetFirstPassWithTagValue(const std::string& tag, const std::string& value) const
    {
        std::optional<size_t> i = GetFirstPassIndexWithTagValue(tag, value);
        return i ? m_Passes[*i].get() : nullptr;
    }

    std::string Shader::GetEngineShaderPathUnixStyle()
    {
#ifdef ENGINE_SHADER_UNIX_PATH
        return ENGINE_SHADER_UNIX_PATH;
#endif

        return PathUtils::GetWorkingDirectoryUtf8(PathStyle::Unix) + "/Shaders";
    }

    static std::unordered_map<std::string, int32_t> s_NameIdMap{};
    static int32_t s_NextNameId = 0;

    int32_t Shader::GetNameId(const std::string& name)
    {
        if (auto it = s_NameIdMap.find(name); it != s_NameIdMap.end())
        {
            return it->second;
        }

        int32_t result = s_NextNameId++;
        s_NameIdMap[name] = result;
        return result;
    }

    const std::string& Shader::GetIdName(int32_t id)
    {
        if (id < s_NextNameId)
        {
            for (const auto& it : s_NameIdMap)
            {
                if (it.second == id)
                {
                    return it.first;
                }
            }
        }

        throw std::runtime_error("Invalid shader property id");
    }

    int32_t Shader::GetMaterialConstantBufferId()
    {
        static int32_t id = Shader::GetNameId("cbMaterial");
        return id;
    }

    ComputeShaderKernel* ComputeShader::GetKernel(size_t index) const
    {
        if (index >= m_Kernels.size())
        {
            throw GfxException("Invalid kernel index");
        }

        return m_Kernels[index].get();
    }

    ComputeShaderKernel* ComputeShader::GetKernel(const std::string& name) const
    {
        for (const std::unique_ptr<ComputeShaderKernel>& k : m_Kernels)
        {
            if (k->GetName() == name)
            {
                return k.get();
            }
        }

        return nullptr;
    }
}
