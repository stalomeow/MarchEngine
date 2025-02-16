#include "pch.h"
#include "Engine/Rendering/D3D12Impl/ShaderGraphics.h"
#include "Engine/Rendering/D3D12Impl/ShaderUtils.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxTexture.h"
#include <algorithm>

namespace march
{
    static constexpr auto MaterialConstantBufferName = "cbMaterial";

    GfxTexture* ShaderProperty::GetDefaultTexture() const
    {
        if (Type != ShaderPropertyType::Texture)
        {
            throw GfxException("Property is not a texture type");
        }

        return GfxTexture::GetDefault(DefaultTexture, TextureDimension);
    }

    D3D12_SHADER_VISIBILITY ShaderPass::GetShaderVisibility(size_t programType)
    {
        switch (static_cast<ShaderProgramType>(programType))
        {
        case ShaderProgramType::Vertex:   return D3D12_SHADER_VISIBILITY_VERTEX;
        case ShaderProgramType::Pixel:    return D3D12_SHADER_VISIBILITY_PIXEL;
        case ShaderProgramType::Domain:   return D3D12_SHADER_VISIBILITY_DOMAIN;
        case ShaderProgramType::Hull:     return D3D12_SHADER_VISIBILITY_HULL;
        case ShaderProgramType::Geometry: return D3D12_SHADER_VISIBILITY_GEOMETRY;
        default:                          throw GfxException("Unknown shader program type");
        }
    }

    bool ShaderPass::GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType)
    {
        if (key == "vs") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Vertex); return true; }
        if (key == "ps") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Pixel); return true; }
        if (key == "ds") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Domain); return true; }
        if (key == "hs") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Hull); return true; }
        if (key == "gs") { *pOutProgramType = static_cast<size_t>(ShaderProgramType::Geometry); return true; }
        return false;
    }

    std::string ShaderPass::GetTargetProfile(const std::string& shaderModel, size_t programType)
    {
        std::string model = shaderModel;
        std::replace(model.begin(), model.end(), '.', '_');

        std::string program;
        switch (static_cast<ShaderProgramType>(programType))
        {
        case ShaderProgramType::Vertex:   program = "vs"; break;
        case ShaderProgramType::Pixel:    program = "ps"; break;
        case ShaderProgramType::Domain:   program = "ds"; break;
        case ShaderProgramType::Hull:     program = "hs"; break;
        case ShaderProgramType::Geometry: program = "gs"; break;
        default:                          program = "unknown"; break;
        }

        return program + "_" + model;
    }

    bool Shader::CompilePass(size_t passIndex, const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error)
    {
        auto recordConstantBufferCallback = [this](ID3D12ShaderReflectionConstantBuffer* cbuffer) -> void
        {
            D3D12_SHADER_BUFFER_DESC bufferDesc{};
            CHECK_HR(cbuffer->GetDesc(&bufferDesc));

            // 记录 material 的 shader property location
            if (strcmp(bufferDesc.Name, MaterialConstantBufferName) == 0 && bufferDesc.Size > 0)
            {
                m_MaterialConstantBufferSize = std::max(m_MaterialConstantBufferSize, static_cast<uint32_t>(bufferDesc.Size));

                for (UINT i = 0; i < bufferDesc.Variables; i++)
                {
                    ID3D12ShaderReflectionVariable* var = cbuffer->GetVariableByIndex(i);
                    D3D12_SHADER_VARIABLE_DESC varDesc{};
                    CHECK_HR(var->GetDesc(&varDesc));

                    ShaderPropertyLocation& loc = m_PropertyLocations[ShaderUtils::GetIdFromString(varDesc.Name)];
                    loc.Offset = static_cast<uint32_t>(varDesc.StartOffset);
                    loc.Size = static_cast<uint32_t>(varDesc.Size);
                }
            }
        };

        m_Version++;
        ShaderPass* pass = GetPass(passIndex);
        return pass->Compile(m_KeywordSpace.get(), filename, source, warnings, error, recordConstantBufferCallback);
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

    int32 Shader::GetMaterialConstantBufferId()
    {
        static int32 id = ShaderUtils::GetIdFromString(MaterialConstantBufferName);
        return id;
    }
}
