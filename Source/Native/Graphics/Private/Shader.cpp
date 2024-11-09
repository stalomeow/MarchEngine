#include "Shader.h"
#include "Debug.h"
#include "GfxExcept.h"
#include "GfxTexture.h"
#include "PathHelper.h"

using namespace DirectX;

namespace march
{
    GfxTexture* ShaderProperty::GetDefaultTexture() const
    {
        if (Type != ShaderPropertyType::Texture)
        {
            throw GfxException("Property is not a texture type");
        }

        switch (DefaultValue.Texture)
        {
        case ShaderDefaultTexture::Black:
            return GfxTexture::GetDefaultBlack();

        case ShaderDefaultTexture::White:
            return GfxTexture::GetDefaultWhite();

        case ShaderDefaultTexture::Bump:
            return GfxTexture::GetDefaultBump();

        default:
            throw GfxException("Unknown default texture type");
        }
    }

    ShaderPass::ShaderPass(Shader* shader)
        : m_Shader(shader)
        , m_Name{}
        , m_Tags{}
        , m_PropertyLocations{}
        , m_Programs{}
        , m_Cull{}
        , m_Blends{}
        , m_DepthState{}
        , m_StencilState{}
        , m_RootSignature(nullptr)
        , m_PipelineStates{}
    {
    }

    Shader* ShaderPass::GetShader() const
    {
        return m_Shader;
    }

    const std::string& ShaderPass::GetName() const
    {
        return m_Name;
    }

    const std::unordered_map<std::string, std::string>& ShaderPass::GetTags() const
    {
        return m_Tags;
    }

    const std::unordered_map<int32_t, ShaderPropertyLocation>& ShaderPass::GetPropertyLocations() const
    {
        return m_PropertyLocations;
    }

    ShaderProgram* ShaderPass::GetProgram(ShaderProgramType type) const
    {
        return m_Programs[static_cast<int32_t>(type)].get();
    }

    const ShaderPassCullMode& ShaderPass::GetCull() const
    {
        return m_Cull;
    }

    const std::vector<ShaderPassBlendState>& ShaderPass::GetBlends() const
    {
        return m_Blends;
    }

    const ShaderPassDepthState& ShaderPass::GetDepthState() const
    {
        return m_DepthState;
    }

    const ShaderPassStencilState& ShaderPass::GetStencilState() const
    {
        return m_StencilState;
    }

    const std::string& Shader::GetName() const
    {
        return m_Name;
    }

    const std::unordered_map<int32_t, ShaderProperty>& Shader::GetProperties() const
    {
        return m_Properties;
    }

    ShaderPass* Shader::GetPass(int32_t index) const
    {
        if (index < 0 || index > m_Passes.size())
        {
            throw GfxException("Invalid pass index");
        }

        return m_Passes[index].get();
    }

    int32_t Shader::GetFirstPassIndexWithTagValue(const std::string& tag, const std::string& value) const
    {
        for (int32_t i = 0; i < m_Passes.size(); i++)
        {
            const std::unordered_map<std::string, std::string>& tags = m_Passes[i]->GetTags();

            if (auto it = tags.find(tag); it != tags.end() && it->second == value)
            {
                return i;
            }
        }

        return -1;
    }

    ShaderPass* Shader::GetFirstPassWithTagValue(const std::string& tag, const std::string& value) const
    {
        int32_t i = GetFirstPassIndexWithTagValue(tag, value);
        return i == -1 ? nullptr : m_Passes[i].get();
    }

    int32_t Shader::GetPassCount() const
    {
        return static_cast<int32_t>(m_Passes.size());
    }

    int32_t Shader::GetVersion() const
    {
        return m_Version;
    }

    std::string Shader::GetEngineShaderPathUnixStyle()
    {
#ifdef ENGINE_SHADER_UNIX_PATH
        return ENGINE_SHADER_UNIX_PATH;
#endif

        return PathHelper::GetWorkingDirectoryUtf8(PathStyle::Unix) + "/Shaders";
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

    static int32_t s_MaterialConstantBufferId = Shader::GetNameId("cbMaterial");

    int32_t Shader::GetMaterialConstantBufferId()
    {
        return s_MaterialConstantBufferId;
    }
}
