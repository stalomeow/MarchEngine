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

    std::string ShaderPass::GetProgramTypePreprocessorMacro(size_t programType)
    {
        switch (static_cast<ShaderProgramType>(programType))
        {
        case ShaderProgramType::Vertex:   return "SHADER_STAGE_VERTEX";
        case ShaderProgramType::Pixel:    return "SHADER_STAGE_PIXEL";
        case ShaderProgramType::Domain:   return "SHADER_STAGE_DOMAIN";
        case ShaderProgramType::Hull:     return "SHADER_STAGE_HULL";
        case ShaderProgramType::Geometry: return "SHADER_STAGE_GEOMETRY";
        default:                          return "SHADER_STAGE_UNKNOWN";
        }
    }

    D3D12_ROOT_SIGNATURE_FLAGS ShaderPass::GetRootSignatureFlags(const ProgramMatch& m)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_root_signature_flags
        // The value in denying access to shader stages is a minor optimization on some hardware.
        // If, for example, the D3D12_SHADER_VISIBILITY_ALL flag has been set to broadcast the root signature to all shader stages,
        // then denying access can overrule this and save the hardware some work.
        // Alternatively if the shader is so simple that no root signature resources are needed,
        // then denying access could be used here too.

        constexpr D3D12_ROOT_SIGNATURE_FLAGS denyAccessFlags[] =
        {
            D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS,
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS,
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS,
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS,
        };

        D3D12_ROOT_SIGNATURE_FLAGS flags
            = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

        for (size_t i = 0; i < NumProgramTypes; i++)
        {
            if (!m.Indices[i])
            {
                flags |= denyAccessFlags[i];
            }
        }

        return flags;
    }

    bool Shader::CompilePass(size_t passIndex, const std::string& filename, const std::string& source, const std::vector<std::string>& pragmas, std::vector<std::string>& warnings, std::string& error)
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
        return pass->Compile(m_KeywordSpace.get(), filename, source, pragmas, warnings, error, recordConstantBufferCallback);
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
