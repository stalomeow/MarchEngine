#include "pch.h"
#include "Engine/Rendering/D3D12Impl/ShaderGraphics.h"
#include "Engine/Rendering/D3D12Impl/ShaderCompute.h"
#include "Engine/Rendering/D3D12Impl/GfxTexture.h"
#include "Engine/Scripting/InteropServices.h"
#include "Engine/Debug.h"
#include <exception>

struct CSharpShaderTexture
{
    cs_string Name;
    cs_uint ShaderRegisterTexture;
    cs_uint RegisterSpaceTexture;
    cs_bool HasSampler;
    cs_uint ShaderRegisterSampler;
    cs_uint RegisterSpaceSampler;
};

struct CSharpShaderStaticSampler
{
    cs_string Name;
    cs_uint ShaderRegister;
    cs_uint RegisterSpace;
};

struct CSharpShaderBuffer
{
    cs_string Name;
    cs_uint ShaderRegister;
    cs_uint RegisterSpace;
    cs_bool IsConstantBuffer;
};

struct CSharpShaderProgram
{
    cs_int Type;
    cs<cs_string[]> Keywords;
    cs<march::cs_byte[]> Hash;
    cs<CSharpShaderBuffer[]> SrvCbvBuffers;
    cs<CSharpShaderTexture[]> SrvTextures;
    cs<CSharpShaderBuffer[]> UavBuffers;
    cs<CSharpShaderTexture[]> UavTextures;
    cs<CSharpShaderStaticSampler[]> StaticSamplers;
    cs_uint ThreadGroupSizeX;
    cs_uint ThreadGroupSizeY;
    cs_uint ThreadGroupSizeZ;
};

struct CSharpShaderProperty
{
    cs_string Name;
    cs<ShaderPropertyType> Type;

    cs_float DefaultFloat;
    cs_int DefaultInt;
    cs_color DefaultColor;
    cs_vec4 DefaultVector;

    cs<GfxTextureDimension> TexDimension;
    cs<GfxDefaultTexture> DefaultTex;
};

struct CSharpShaderPropertyLocation
{
    cs_string Name;
    cs_uint Offset;
    cs_uint Size;
};

struct CSharpOptionalShaderPropertyId
{
    cs_bool HasValue;
    cs_int Value;
};

template<typename T>
struct CSharpShaderPassVar
{
    CSharpOptionalShaderPropertyId PropertyId;
    cs<T> Value;
};

struct CSharpShaderPassBlendFormula
{
    CSharpShaderPassVar<BlendMode> Src;
    CSharpShaderPassVar<BlendMode> Dest;
    CSharpShaderPassVar<BlendOp> Op;
};

struct CSharpShaderPassBlendState
{
    cs_bool Enable;
    CSharpShaderPassVar<ColorWriteMask> WriteMask;
    CSharpShaderPassBlendFormula Rgb;
    CSharpShaderPassBlendFormula Alpha;
};

struct CSharpShaderPassDepthState
{
    cs_bool Enable;
    CSharpShaderPassVar<bool> Write;
    CSharpShaderPassVar<CompareFunction> Compare;
};

struct CSharpShaderPassStencilAction
{
    CSharpShaderPassVar<CompareFunction> Compare;
    CSharpShaderPassVar<StencilOp> PassOp;
    CSharpShaderPassVar<StencilOp> FailOp;
    CSharpShaderPassVar<StencilOp> DepthFailOp;
};

struct CSharpShaderPassStencilState
{
    cs_bool Enable;
    CSharpShaderPassVar<uint8_t> Ref;
    CSharpShaderPassVar<uint8_t> ReadMask;
    CSharpShaderPassVar<uint8_t> WriteMask;
    CSharpShaderPassStencilAction FrontFace;
    CSharpShaderPassStencilAction BackFace;
};

struct CSharpShaderPassTag
{
    cs_string Key;
    cs_string Value;
};

struct CSharpShaderPass
{
    cs_string Name;
    cs<CSharpShaderPassTag[]> Tags;
    cs<CSharpShaderProgram[]> Programs;

    CSharpShaderPassVar<CullMode> Cull;
    cs<CSharpShaderPassBlendState[]> Blends;
    CSharpShaderPassDepthState DepthState;
    CSharpShaderPassStencilState StencilState;
};

template<typename T>
ShaderPassVar<T> UnpackShaderPassVar(const CSharpShaderPassVar<T>& v)
{
    ShaderPassVar<T> result{};

    if (v.PropertyId.HasValue)
    {
        result.IsDynamic = true;
        result.PropertyId = v.PropertyId.Value;
    }
    else
    {
        result.IsDynamic = false;
        result.Value = v.Value;
    }

    return result;
}

template<typename T>
CSharpShaderPassVar<T> PackShaderPassVar(const ShaderPassVar<T>& v)
{
    CSharpShaderPassVar<T> result{};

    if (v.IsDynamic)
    {
        result.PropertyId.HasValue.assign(true);
        result.PropertyId.Value.assign(v.PropertyId);
    }
    else
    {
        result.PropertyId.HasValue.assign(false);
        result.Value.assign(v.Value);
    }

    return result;
}

namespace march
{
    struct ShaderBinding
    {
        inline static void ClearProperties(Shader* pShader)
        {
            pShader->m_Version++;
            pShader->m_Properties.clear();
        }

        inline static void SetName(Shader* pShader, cs_string name)
        {
            pShader->m_Version++;
            pShader->m_Name = name;
        }

        inline static void SetProperty(Shader* pShader, CSharpShaderProperty* prop)
        {
            pShader->m_Version++;

            auto& p = pShader->m_Properties[ShaderUtils::GetIdFromString(prop->Name)];
            p.Type = prop->Type;

            switch (prop->Type)
            {
            case ShaderPropertyType::Float:
                p.DefaultFloat = prop->DefaultFloat;
                break;
            case ShaderPropertyType::Int:
                p.DefaultInt = prop->DefaultInt;
                break;
            case ShaderPropertyType::Color:
                p.DefaultColor = prop->DefaultColor;
                break;
            case ShaderPropertyType::Vector:
                p.DefaultVector = prop->DefaultVector;
                break;
            case ShaderPropertyType::Texture:
                p.TextureDimension = prop->TexDimension;
                p.DefaultTexture = prop->DefaultTex;
                break;
            default:
                LOG_ERROR("Unknown shader property type: {}", prop->Type.data);
                break;
            }
        }

        inline static void SetPasses(Shader* pShader, cs<CSharpShaderPass[]> passes)
        {
            pShader->m_Version++;
            pShader->m_KeywordSpace->Clear();
            pShader->m_Passes.clear();
            pShader->m_Passes.resize(static_cast<size_t>(passes.size()));

            for (int32_t i = 0; i < pShader->m_Passes.size(); i++)
            {
                const auto& src = passes[i];
                pShader->m_Passes[i] = std::make_unique<ShaderPass>();
                ShaderPass* pass = pShader->m_Passes[i].get();

                pass->m_Name = src.Name;

                pass->m_Tags.clear();
                for (int32_t j = 0; j < src.Tags.size(); j++)
                {
                    const auto& t = src.Tags[j];
                    pass->m_Tags[t.Key] = t.Value;
                }

                for (int32_t j = 0; j < std::size(pass->m_Programs); j++)
                {
                    pass->m_Programs[j].clear();
                }
                for (int32_t j = 0; j < src.Programs.size(); j++)
                {
                    const auto& p = src.Programs[j];
                    std::unique_ptr<ShaderProgram> program = std::make_unique<ShaderProgram>();
                    program->m_Keywords.Reset(pShader->m_KeywordSpace.get());

                    for (int32_t k = 0; k < p.Keywords.size(); k++)
                    {
                        bool result = pShader->m_KeywordSpace->RegisterKeyword(p.Keywords[k]);
                        assert(result);
                        program->m_Keywords.EnableKeyword(p.Keywords[k]);
                    }

                    std::copy(p.Hash.begin(), p.Hash.end(), program->m_Hash.Data);

                    try
                    {
                        ShaderCompilationInternalUtils::LoadShaderBinaryByHash(program->GetHash(), program->m_Binary.ReleaseAndGetAddressOf());
                    }
                    catch (const std::exception& e)
                    {
                        LOG_ERROR("Failed to create shader blob: {}", e.what());
                        return;
                    }

                    for (int32_t k = 0; k < p.SrvCbvBuffers.size(); k++)
                    {
                        const auto& buf = p.SrvCbvBuffers[k];
                        program->m_SrvCbvBuffers.push_back({ ShaderUtils::GetIdFromString(buf.Name), buf.ShaderRegister, buf.RegisterSpace, buf.IsConstantBuffer });
                    }

                    for (int32_t k = 0; k < p.SrvTextures.size(); k++)
                    {
                        const auto& tp = p.SrvTextures[k];
                        program->m_SrvTextures.push_back({
                            ShaderUtils::GetIdFromString(tp.Name),
                            tp.ShaderRegisterTexture,
                            tp.RegisterSpaceTexture,
                            tp.HasSampler,
                            tp.ShaderRegisterSampler,
                            tp.RegisterSpaceSampler
                            });
                    }

                    for (int32_t k = 0; k < p.UavBuffers.size(); k++)
                    {
                        const auto& buf = p.UavBuffers[k];
                        program->m_UavBuffers.push_back({ ShaderUtils::GetIdFromString(buf.Name), buf.ShaderRegister, buf.RegisterSpace, buf.IsConstantBuffer });
                    }

                    for (int32_t k = 0; k < p.UavTextures.size(); k++)
                    {
                        const auto& tp = p.UavTextures[k];
                        program->m_UavTextures.push_back({
                            ShaderUtils::GetIdFromString(tp.Name),
                            tp.ShaderRegisterTexture,
                            tp.RegisterSpaceTexture,
                            tp.HasSampler,
                            tp.ShaderRegisterSampler,
                            tp.RegisterSpaceSampler
                            });
                    }

                    for (int32_t k = 0; k < p.StaticSamplers.size(); k++)
                    {
                        const auto& sampler = p.StaticSamplers[k];
                        program->m_StaticSamplers.push_back({ ShaderUtils::GetIdFromString(sampler.Name), sampler.ShaderRegister, sampler.RegisterSpace });
                    }

                    program->m_ThreadGroupSizeX = p.ThreadGroupSizeX;
                    program->m_ThreadGroupSizeY = p.ThreadGroupSizeY;
                    program->m_ThreadGroupSizeZ = p.ThreadGroupSizeZ;

                    pass->m_Programs[static_cast<int32_t>(p.Type.data)].emplace_back(std::move(program));
                }

                pass->m_RenderState.Cull = UnpackShaderPassVar(src.Cull);

                pass->m_RenderState.Blends.resize(src.Blends.size());
                for (int32_t j = 0; j < pass->m_RenderState.Blends.size(); j++)
                {
                    const auto& blend = src.Blends[j];
                    pass->m_RenderState.Blends[j].Enable = blend.Enable;
                    pass->m_RenderState.Blends[j].WriteMask = UnpackShaderPassVar(blend.WriteMask);
                    pass->m_RenderState.Blends[j].Rgb = { UnpackShaderPassVar(blend.Rgb.Src), UnpackShaderPassVar(blend.Rgb.Dest), UnpackShaderPassVar(blend.Rgb.Op) };
                    pass->m_RenderState.Blends[j].Alpha = { UnpackShaderPassVar(blend.Alpha.Src), UnpackShaderPassVar(blend.Alpha.Dest), UnpackShaderPassVar(blend.Alpha.Op) };
                }

                pass->m_RenderState.DepthState =
                {
                    src.DepthState.Enable,
                    UnpackShaderPassVar(src.DepthState.Write),
                    UnpackShaderPassVar(src.DepthState.Compare)
                };

                pass->m_RenderState.StencilState =
                {
                    src.StencilState.Enable,
                    UnpackShaderPassVar(src.StencilState.Ref),
                    UnpackShaderPassVar(src.StencilState.ReadMask),
                    UnpackShaderPassVar(src.StencilState.WriteMask),
                    {
                        UnpackShaderPassVar(src.StencilState.FrontFace.Compare),
                        UnpackShaderPassVar(src.StencilState.FrontFace.PassOp),
                        UnpackShaderPassVar(src.StencilState.FrontFace.FailOp),
                        UnpackShaderPassVar(src.StencilState.FrontFace.DepthFailOp)
                    },
                    {
                        UnpackShaderPassVar(src.StencilState.BackFace.Compare),
                        UnpackShaderPassVar(src.StencilState.BackFace.PassOp),
                        UnpackShaderPassVar(src.StencilState.BackFace.FailOp),
                        UnpackShaderPassVar(src.StencilState.BackFace.DepthFailOp)
                    }
                };
            }
        }

        inline static bool CompilePass(cs<Shader*> pShader, cs_int passIndex, cs_string filename, cs_string source, cs<cs_string[]> pragmas, cs<cs<cs_string[]>*> warnings, cs<cs_string*> error)
        {
            std::vector<std::string> pragmasVec{};

            for (int32_t i = 0; i < pragmas.size(); i++)
            {
                pragmasVec.push_back(pragmas[i]);
            }

            std::vector<std::string> warningBuffer{};
            std::string errorBuffer{};

            bool ret = pShader->CompilePass(static_cast<size_t>(passIndex.data), filename, source, pragmasVec, warningBuffer, errorBuffer);

            if (!warningBuffer.empty())
            {
                warnings->assign(static_cast<int32_t>(warningBuffer.size()));

                for (int32_t i = 0; i < warningBuffer.size(); i++)
                {
                    (*warnings)[i].assign(std::move(warningBuffer[i]));
                }
            }

            if (!errorBuffer.empty())
            {
                error->assign(std::move(errorBuffer));
            }

            return ret;
        }

        inline static void GetPasses(cs<Shader*> pShader, cs<cs<CSharpShaderPass[]>*> passes)
        {
            passes->assign(static_cast<int32_t>(pShader->GetPassCount()));

            for (int32_t i = 0; i < pShader->GetPassCount(); i++)
            {
                ShaderPass* pass = pShader->GetPass(i);
                CSharpShaderPass& dest = (*passes)[i];

                dest.Name.assign(pass->GetName());

                dest.Tags.assign(static_cast<int32_t>(pass->GetTags().size()));
                int32_t tagIndex = 0;
                for (auto& kvp : pass->GetTags())
                {
                    auto& tag = dest.Tags[tagIndex++];
                    tag.Key.assign(kvp.first);
                    tag.Value.assign(kvp.second);
                }

                int32_t programCount = 0;
                for (int32_t j = 0; j < static_cast<int32_t>(Shader::NumProgramTypes); j++)
                {
                    programCount += static_cast<int32_t>(pass->m_Programs[j].size());
                }

                dest.Programs.assign(programCount);
                int32_t programIndex = 0;
                for (int32_t j = 0; j < static_cast<int32_t>(Shader::NumProgramTypes); j++)
                {
                    for (std::unique_ptr<ShaderProgram>& program : pass->m_Programs[j])
                    {
                        auto& p = dest.Programs[programIndex++];
                        p.Type.assign(j);

                        std::vector<std::string> keywords = program->m_Keywords.GetEnabledKeywordStringsInSpace();
                        p.Keywords.assign(static_cast<int32_t>(keywords.size()));

                        for (int32_t k = 0; k < keywords.size(); k++)
                        {
                            p.Keywords[k].assign(std::move(keywords[k]));
                        }

                        p.Hash.assign(static_cast<int32_t>(std::size(program->GetHash().Data)), reinterpret_cast<const march::cs_byte*>(program->GetHash().Data));

                        p.SrvCbvBuffers.assign(static_cast<int32_t>(program->GetSrvCbvBuffers().size()));
                        for (size_t bufIdx = 0; bufIdx < program->GetSrvCbvBuffers().size(); bufIdx++)
                        {
                            auto& buf = p.SrvCbvBuffers[static_cast<int32_t>(bufIdx)];
                            buf.Name.assign(ShaderUtils::GetStringFromId(program->GetSrvCbvBuffers()[bufIdx].Id));
                            buf.ShaderRegister.assign(program->GetSrvCbvBuffers()[bufIdx].ShaderRegister);
                            buf.RegisterSpace.assign(program->GetSrvCbvBuffers()[bufIdx].RegisterSpace);
                            buf.IsConstantBuffer.assign(program->GetSrvCbvBuffers()[bufIdx].IsConstantBuffer);
                        }

                        p.SrvTextures.assign(static_cast<int32_t>(program->GetSrvTextures().size()));
                        for (size_t texIdx = 0; texIdx < program->GetSrvTextures().size(); texIdx++)
                        {
                            auto& tp = p.SrvTextures[static_cast<int32_t>(texIdx)];
                            tp.Name.assign(ShaderUtils::GetStringFromId(program->GetSrvTextures()[texIdx].Id));
                            tp.ShaderRegisterTexture.assign(program->GetSrvTextures()[texIdx].ShaderRegisterTexture);
                            tp.RegisterSpaceTexture.assign(program->GetSrvTextures()[texIdx].RegisterSpaceTexture);
                            tp.HasSampler.assign(program->GetSrvTextures()[texIdx].HasSampler);
                            tp.ShaderRegisterSampler.assign(program->GetSrvTextures()[texIdx].ShaderRegisterSampler);
                            tp.RegisterSpaceSampler.assign(program->GetSrvTextures()[texIdx].RegisterSpaceSampler);
                        }

                        p.UavBuffers.assign(static_cast<int32_t>(program->GetUavBuffers().size()));
                        for (size_t bufIdx = 0; bufIdx < program->GetUavBuffers().size(); bufIdx++)
                        {
                            auto& buf = p.UavBuffers[static_cast<int32_t>(bufIdx)];
                            buf.Name.assign(ShaderUtils::GetStringFromId(program->GetUavBuffers()[bufIdx].Id));
                            buf.ShaderRegister.assign(program->GetUavBuffers()[bufIdx].ShaderRegister);
                            buf.RegisterSpace.assign(program->GetUavBuffers()[bufIdx].RegisterSpace);
                            buf.IsConstantBuffer.assign(program->GetUavBuffers()[bufIdx].IsConstantBuffer);
                        }

                        p.UavTextures.assign(static_cast<int32_t>(program->GetUavTextures().size()));
                        for (size_t texIdx = 0; texIdx < program->GetUavTextures().size(); texIdx++)
                        {
                            auto& tp = p.UavTextures[static_cast<int32_t>(texIdx)];
                            tp.Name.assign(ShaderUtils::GetStringFromId(program->GetUavTextures()[texIdx].Id));
                            tp.ShaderRegisterTexture.assign(program->GetUavTextures()[texIdx].ShaderRegisterTexture);
                            tp.RegisterSpaceTexture.assign(program->GetUavTextures()[texIdx].RegisterSpaceTexture);
                            tp.HasSampler.assign(program->GetUavTextures()[texIdx].HasSampler);
                            tp.ShaderRegisterSampler.assign(program->GetUavTextures()[texIdx].ShaderRegisterSampler);
                            tp.RegisterSpaceSampler.assign(program->GetUavTextures()[texIdx].RegisterSpaceSampler);
                        }

                        p.StaticSamplers.assign(static_cast<int32_t>(program->GetStaticSamplers().size()));
                        for (size_t samplerIdx = 0; samplerIdx < program->GetStaticSamplers().size(); samplerIdx++)
                        {
                            auto& sampler = p.StaticSamplers[static_cast<int32_t>(samplerIdx)];
                            sampler.Name.assign(ShaderUtils::GetStringFromId(program->GetStaticSamplers()[samplerIdx].Id));
                            sampler.ShaderRegister.assign(program->GetStaticSamplers()[samplerIdx].ShaderRegister);
                            sampler.RegisterSpace.assign(program->GetStaticSamplers()[samplerIdx].RegisterSpace);
                        }

                        p.ThreadGroupSizeX.assign(program->m_ThreadGroupSizeX);
                        p.ThreadGroupSizeY.assign(program->m_ThreadGroupSizeY);
                        p.ThreadGroupSizeZ.assign(program->m_ThreadGroupSizeZ);
                    }
                }

                dest.Cull = PackShaderPassVar(pass->m_RenderState.Cull);
                dest.Blends.assign(static_cast<int32_t>(pass->m_RenderState.Blends.size()));
                for (int32_t j = 0; j < pass->m_RenderState.Blends.size(); j++)
                {
                    auto& blend = dest.Blends[j];
                    blend.Enable.assign(pass->m_RenderState.Blends[j].Enable);
                    blend.WriteMask = PackShaderPassVar(pass->m_RenderState.Blends[j].WriteMask);
                    blend.Rgb.Src = PackShaderPassVar(pass->m_RenderState.Blends[j].Rgb.Src);
                    blend.Rgb.Dest = PackShaderPassVar(pass->m_RenderState.Blends[j].Rgb.Dest);
                    blend.Rgb.Op = PackShaderPassVar(pass->m_RenderState.Blends[j].Rgb.Op);
                    blend.Alpha.Src = PackShaderPassVar(pass->m_RenderState.Blends[j].Alpha.Src);
                    blend.Alpha.Dest = PackShaderPassVar(pass->m_RenderState.Blends[j].Alpha.Dest);
                    blend.Alpha.Op = PackShaderPassVar(pass->m_RenderState.Blends[j].Alpha.Op);
                }

                dest.DepthState.Enable.assign(pass->m_RenderState.DepthState.Enable);
                dest.DepthState.Write = PackShaderPassVar(pass->m_RenderState.DepthState.Write);
                dest.DepthState.Compare = PackShaderPassVar(pass->m_RenderState.DepthState.Compare);
                dest.StencilState.Enable.assign(pass->m_RenderState.StencilState.Enable);
                dest.StencilState.Ref = PackShaderPassVar(pass->m_RenderState.StencilState.Ref);
                dest.StencilState.ReadMask = PackShaderPassVar(pass->m_RenderState.StencilState.ReadMask);
                dest.StencilState.WriteMask = PackShaderPassVar(pass->m_RenderState.StencilState.WriteMask);
                dest.StencilState.FrontFace.Compare = PackShaderPassVar(pass->m_RenderState.StencilState.FrontFace.Compare);
                dest.StencilState.FrontFace.PassOp = PackShaderPassVar(pass->m_RenderState.StencilState.FrontFace.PassOp);
                dest.StencilState.FrontFace.FailOp = PackShaderPassVar(pass->m_RenderState.StencilState.FrontFace.FailOp);
                dest.StencilState.FrontFace.DepthFailOp = PackShaderPassVar(pass->m_RenderState.StencilState.FrontFace.DepthFailOp);
                dest.StencilState.BackFace.Compare = PackShaderPassVar(pass->m_RenderState.StencilState.BackFace.Compare);
                dest.StencilState.BackFace.PassOp = PackShaderPassVar(pass->m_RenderState.StencilState.BackFace.PassOp);
                dest.StencilState.BackFace.FailOp = PackShaderPassVar(pass->m_RenderState.StencilState.BackFace.FailOp);
                dest.StencilState.BackFace.DepthFailOp = PackShaderPassVar(pass->m_RenderState.StencilState.BackFace.DepthFailOp);
            }
        }

        inline static void SetMaterialConstantBufferSize(cs<Shader*> pShader, cs_uint value)
        {
            pShader->m_Version++;
            pShader->m_MaterialConstantBufferSize = value;
        }

        inline static void GetPropertyLocations(cs<Shader*> pShader, cs<cs<CSharpShaderPropertyLocation[]>*> locations)
        {
            locations->assign(static_cast<int32_t>(pShader->GetPropertyLocations().size()));
            int32_t propLocIndex = 0;
            for (auto& kvp : pShader->GetPropertyLocations())
            {
                auto& loc = (*locations)[propLocIndex++];
                loc.Name.assign(ShaderUtils::GetStringFromId(kvp.first));
                loc.Offset.assign(kvp.second.Offset);
                loc.Size.assign(kvp.second.Size);
            }
        }

        inline static void SetPropertyLocations(cs<Shader*> pShader, cs<CSharpShaderPropertyLocation[]> locations)
        {
            pShader->m_Version++;
            pShader->m_PropertyLocations.clear();

            for (int32_t i = 0; i < locations.size(); i++)
            {
                const auto& mp = locations[i];
                pShader->m_PropertyLocations[ShaderUtils::GetIdFromString(mp.Name)] = { mp.Offset, mp.Size };
            }
        }
    };
}

NATIVE_EXPORT_AUTO Shader_New()
{
    retcs MARCH_NEW Shader();
}

NATIVE_EXPORT_AUTO Shader_GetName(cs<Shader*> pShader)
{
    retcs pShader->GetName();
}

NATIVE_EXPORT_AUTO Shader_SetName(cs<Shader*> pShader, cs_string name)
{
    ShaderBinding::SetName(pShader, name);
}

NATIVE_EXPORT_AUTO Shader_ClearProperties(cs<Shader*> pShader)
{
    ShaderBinding::ClearProperties(pShader);
}

NATIVE_EXPORT_AUTO Shader_SetProperty(cs<Shader*> pShader, cs<CSharpShaderProperty*> prop)
{
    ShaderBinding::SetProperty(pShader, prop);
}

NATIVE_EXPORT_AUTO Shader_GetPasses(cs<Shader*> pShader, cs<cs<CSharpShaderPass[]>*> passes)
{
    ShaderBinding::GetPasses(pShader, passes);
}

NATIVE_EXPORT_AUTO Shader_SetPasses(cs<Shader*> pShader, cs<CSharpShaderPass[]> passes)
{
    ShaderBinding::SetPasses(pShader, passes);
}

NATIVE_EXPORT_AUTO Shader_GetMaterialConstantBufferSize(cs<Shader*> pShader)
{
    retcs pShader->GetMaterialConstantBufferSize();
}

NATIVE_EXPORT_AUTO Shader_SetMaterialConstantBufferSize(cs<Shader*> pShader, cs_uint value)
{
    ShaderBinding::SetMaterialConstantBufferSize(pShader, value);
}

NATIVE_EXPORT_AUTO Shader_GetPropertyLocations(cs<Shader*> pShader, cs<cs<CSharpShaderPropertyLocation[]>*> locations)
{
    ShaderBinding::GetPropertyLocations(pShader, locations);
}

NATIVE_EXPORT_AUTO Shader_SetPropertyLocations(cs<Shader*> pShader, cs<CSharpShaderPropertyLocation[]> locations)
{
    ShaderBinding::SetPropertyLocations(pShader, locations);
}

NATIVE_EXPORT_AUTO Shader_CompilePass(cs<Shader*> pShader, cs_int passIndex, cs_string filename, cs_string source, cs<cs_string[]> pragmas, cs<cs<cs_string[]>*> warnings, cs<cs_string*> error)
{
    retcs ShaderBinding::CompilePass(pShader, passIndex, filename, source, pragmas, warnings, error);
}

NATIVE_EXPORT_AUTO Shader_GetMaterialConstantBufferId()
{
    retcs Shader::GetMaterialConstantBufferId();
}

NATIVE_EXPORT_AUTO ShaderUtils_GetIdFromString(cs_string name)
{
    retcs ShaderUtils::GetIdFromString(name);
}

NATIVE_EXPORT_AUTO ShaderUtils_GetStringFromId(cs_int id)
{
    retcs ShaderUtils::GetStringFromId(id);
}

NATIVE_EXPORT_AUTO ShaderUtils_HasCachedShaderProgram(cs<march::cs_byte[]> hash)
{
    std::vector<uint8_t> hashVec;

    for (int32_t i = 0; i < hash.size(); i++)
    {
        hashVec.push_back(hash[i]);
    }

    retcs ShaderUtils::HasCachedShaderProgram(hashVec);
}

NATIVE_EXPORT_AUTO ShaderUtils_DeleteCachedShaderProgram(cs<march::cs_byte[]> hash)
{
    std::vector<uint8_t> hashVec;

    for (int32_t i = 0; i < hash.size(); i++)
    {
        hashVec.push_back(hash[i]);
    }

    ShaderUtils::DeleteCachedShaderProgram(hashVec);
}

struct CSharpComputeShaderKernel
{
    cs_string Name;
    cs<CSharpShaderProgram[]> Programs;
};

namespace march
{
    struct ComputeShaderBinding
    {
        static void SetName(cs<ComputeShader*> s, cs_string name)
        {
            s->m_Name = name;
        }

        static void GetKernels(cs<ComputeShader*> s, cs<cs<CSharpComputeShaderKernel[]>*> kernels)
        {
            kernels->assign(static_cast<int32_t>(s->GetKernelCount()));

            for (int32_t i = 0; i < s->GetKernelCount(); i++)
            {
                ComputeShaderKernel* kernel = s->m_Kernels[i].get();
                CSharpComputeShaderKernel& dest = (*kernels)[i];

                dest.Name.assign(kernel->GetName());

                int32_t programCount = 0;
                for (int32_t j = 0; j < static_cast<int32_t>(ComputeShader::NumProgramTypes); j++)
                {
                    programCount += static_cast<int32_t>(kernel->m_Programs[j].size());
                }

                dest.Programs.assign(programCount);
                int32_t programIndex = 0;
                for (int32_t j = 0; j < static_cast<int32_t>(ComputeShader::NumProgramTypes); j++)
                {
                    for (std::unique_ptr<ShaderProgram>& program : kernel->m_Programs[j])
                    {
                        auto& p = dest.Programs[programIndex++];
                        p.Type.assign(j);

                        std::vector<std::string> keywords = program->m_Keywords.GetEnabledKeywordStringsInSpace();
                        p.Keywords.assign(static_cast<int32_t>(keywords.size()));

                        for (int32_t k = 0; k < keywords.size(); k++)
                        {
                            p.Keywords[k].assign(std::move(keywords[k]));
                        }

                        p.Hash.assign(static_cast<int32_t>(std::size(program->GetHash().Data)), reinterpret_cast<const march::cs_byte*>(program->GetHash().Data));

                        p.SrvCbvBuffers.assign(static_cast<int32_t>(program->GetSrvCbvBuffers().size()));
                        for (size_t bufIdx = 0; bufIdx < program->GetSrvCbvBuffers().size(); bufIdx++)
                        {
                            auto& buf = p.SrvCbvBuffers[static_cast<int32_t>(bufIdx)];
                            buf.Name.assign(ShaderUtils::GetStringFromId(program->GetSrvCbvBuffers()[bufIdx].Id));
                            buf.ShaderRegister.assign(program->GetSrvCbvBuffers()[bufIdx].ShaderRegister);
                            buf.RegisterSpace.assign(program->GetSrvCbvBuffers()[bufIdx].RegisterSpace);
                            buf.IsConstantBuffer.assign(program->GetSrvCbvBuffers()[bufIdx].IsConstantBuffer);
                        }

                        p.SrvTextures.assign(static_cast<int32_t>(program->GetSrvTextures().size()));
                        for (size_t texIdx = 0; texIdx < program->GetSrvTextures().size(); texIdx++)
                        {
                            auto& tp = p.SrvTextures[static_cast<int32_t>(texIdx)];
                            tp.Name.assign(ShaderUtils::GetStringFromId(program->GetSrvTextures()[texIdx].Id));
                            tp.ShaderRegisterTexture.assign(program->GetSrvTextures()[texIdx].ShaderRegisterTexture);
                            tp.RegisterSpaceTexture.assign(program->GetSrvTextures()[texIdx].RegisterSpaceTexture);
                            tp.HasSampler.assign(program->GetSrvTextures()[texIdx].HasSampler);
                            tp.ShaderRegisterSampler.assign(program->GetSrvTextures()[texIdx].ShaderRegisterSampler);
                            tp.RegisterSpaceSampler.assign(program->GetSrvTextures()[texIdx].RegisterSpaceSampler);
                        }

                        p.UavBuffers.assign(static_cast<int32_t>(program->GetUavBuffers().size()));
                        for (size_t bufIdx = 0; bufIdx < program->GetUavBuffers().size(); bufIdx++)
                        {
                            auto& buf = p.UavBuffers[static_cast<int32_t>(bufIdx)];
                            buf.Name.assign(ShaderUtils::GetStringFromId(program->GetUavBuffers()[bufIdx].Id));
                            buf.ShaderRegister.assign(program->GetUavBuffers()[bufIdx].ShaderRegister);
                            buf.RegisterSpace.assign(program->GetUavBuffers()[bufIdx].RegisterSpace);
                            buf.IsConstantBuffer.assign(program->GetUavBuffers()[bufIdx].IsConstantBuffer);
                        }

                        p.UavTextures.assign(static_cast<int32_t>(program->GetUavTextures().size()));
                        for (size_t texIdx = 0; texIdx < program->GetUavTextures().size(); texIdx++)
                        {
                            auto& tp = p.UavTextures[static_cast<int32_t>(texIdx)];
                            tp.Name.assign(ShaderUtils::GetStringFromId(program->GetUavTextures()[texIdx].Id));
                            tp.ShaderRegisterTexture.assign(program->GetUavTextures()[texIdx].ShaderRegisterTexture);
                            tp.RegisterSpaceTexture.assign(program->GetUavTextures()[texIdx].RegisterSpaceTexture);
                            tp.HasSampler.assign(program->GetUavTextures()[texIdx].HasSampler);
                            tp.ShaderRegisterSampler.assign(program->GetUavTextures()[texIdx].ShaderRegisterSampler);
                            tp.RegisterSpaceSampler.assign(program->GetUavTextures()[texIdx].RegisterSpaceSampler);
                        }

                        p.StaticSamplers.assign(static_cast<int32_t>(program->GetStaticSamplers().size()));
                        for (size_t samplerIdx = 0; samplerIdx < program->GetStaticSamplers().size(); samplerIdx++)
                        {
                            auto& sampler = p.StaticSamplers[static_cast<int32_t>(samplerIdx)];
                            sampler.Name.assign(ShaderUtils::GetStringFromId(program->GetStaticSamplers()[samplerIdx].Id));
                            sampler.ShaderRegister.assign(program->GetStaticSamplers()[samplerIdx].ShaderRegister);
                            sampler.RegisterSpace.assign(program->GetStaticSamplers()[samplerIdx].RegisterSpace);
                        }

                        p.ThreadGroupSizeX.assign(program->m_ThreadGroupSizeX);
                        p.ThreadGroupSizeY.assign(program->m_ThreadGroupSizeY);
                        p.ThreadGroupSizeZ.assign(program->m_ThreadGroupSizeZ);
                    }
                }
            }
        }

        static void SetKernels(cs<ComputeShader*> s, cs<CSharpComputeShaderKernel[]> kernels)
        {
            s->m_KeywordSpace->Clear();
            s->m_Kernels.clear();
            s->m_Kernels.resize(static_cast<size_t>(kernels.size()));

            for (int32_t i = 0; i < s->m_Kernels.size(); i++)
            {
                const auto& src = kernels[i];
                s->m_Kernels[i] = std::make_unique<ComputeShaderKernel>();
                ComputeShaderKernel* kernel = s->m_Kernels[i].get();

                kernel->m_Name = src.Name;

                for (int32_t j = 0; j < std::size(kernel->m_Programs); j++)
                {
                    kernel->m_Programs[j].clear();
                }
                for (int32_t j = 0; j < src.Programs.size(); j++)
                {
                    const auto& p = src.Programs[j];
                    std::unique_ptr<ShaderProgram> program = std::make_unique<ShaderProgram>();
                    program->m_Keywords.Reset(s->m_KeywordSpace.get());

                    for (int32_t k = 0; k < p.Keywords.size(); k++)
                    {
                        bool result = s->m_KeywordSpace->RegisterKeyword(p.Keywords[k]);
                        assert(result);
                        program->m_Keywords.EnableKeyword(p.Keywords[k]);
                    }

                    std::copy(p.Hash.begin(), p.Hash.end(), program->m_Hash.Data);

                    try
                    {
                        ShaderCompilationInternalUtils::LoadShaderBinaryByHash(program->GetHash(), program->m_Binary.ReleaseAndGetAddressOf());
                    }
                    catch (const std::exception& e)
                    {
                        LOG_ERROR("Failed to create shader blob: {}", e.what());
                        return;
                    }

                    for (int32_t k = 0; k < p.SrvCbvBuffers.size(); k++)
                    {
                        const auto& buf = p.SrvCbvBuffers[k];
                        program->m_SrvCbvBuffers.push_back({ ShaderUtils::GetIdFromString(buf.Name), buf.ShaderRegister, buf.RegisterSpace, buf.IsConstantBuffer });
                    }

                    for (int32_t k = 0; k < p.SrvTextures.size(); k++)
                    {
                        const auto& tp = p.SrvTextures[k];
                        program->m_SrvTextures.push_back({
                            ShaderUtils::GetIdFromString(tp.Name),
                            tp.ShaderRegisterTexture,
                            tp.RegisterSpaceTexture,
                            tp.HasSampler,
                            tp.ShaderRegisterSampler,
                            tp.RegisterSpaceSampler
                            });
                    }

                    for (int32_t k = 0; k < p.UavBuffers.size(); k++)
                    {
                        const auto& buf = p.UavBuffers[k];
                        program->m_UavBuffers.push_back({ ShaderUtils::GetIdFromString(buf.Name), buf.ShaderRegister, buf.RegisterSpace, buf.IsConstantBuffer });
                    }

                    for (int32_t k = 0; k < p.UavTextures.size(); k++)
                    {
                        const auto& tp = p.UavTextures[k];
                        program->m_UavTextures.push_back({
                            ShaderUtils::GetIdFromString(tp.Name),
                            tp.ShaderRegisterTexture,
                            tp.RegisterSpaceTexture,
                            tp.HasSampler,
                            tp.ShaderRegisterSampler,
                            tp.RegisterSpaceSampler
                            });
                    }

                    for (int32_t k = 0; k < p.StaticSamplers.size(); k++)
                    {
                        const auto& sampler = p.StaticSamplers[k];
                        program->m_StaticSamplers.push_back({ ShaderUtils::GetIdFromString(sampler.Name), sampler.ShaderRegister, sampler.RegisterSpace });
                    }

                    program->m_ThreadGroupSizeX = p.ThreadGroupSizeX;
                    program->m_ThreadGroupSizeY = p.ThreadGroupSizeY;
                    program->m_ThreadGroupSizeZ = p.ThreadGroupSizeZ;

                    kernel->m_Programs[static_cast<size_t>(p.Type.data)].emplace_back(std::move(program));
                }
            }
        }

        static bool Compile(cs<ComputeShader*> s, cs_string filename, cs_string source, cs<cs_string[]> pragmas, cs<cs<cs_string[]>*> warnings, cs<cs_string*> error)
        {
            std::vector<std::string> pragmasVec{};

            for (int32_t i = 0; i < pragmas.size(); i++)
            {
                pragmasVec.push_back(pragmas[i]);
            }

            std::vector<std::string> warningBuffer{};
            std::string errorBuffer{};

            bool ret = s->Compile(filename, source, pragmasVec, warningBuffer, errorBuffer);

            if (!warningBuffer.empty())
            {
                warnings->assign(static_cast<int32_t>(warningBuffer.size()));

                for (int32_t i = 0; i < warningBuffer.size(); i++)
                {
                    (*warnings)[i].assign(std::move(warningBuffer[i]));
                }
            }

            if (!errorBuffer.empty())
            {
                error->assign(std::move(errorBuffer));
            }

            return ret;
        }
    };
}

NATIVE_EXPORT_AUTO ComputeShader_New()
{
    retcs MARCH_NEW ComputeShader();
}

NATIVE_EXPORT_AUTO ComputeShader_GetName(cs<ComputeShader*> s)
{
    retcs s->GetName();
}

NATIVE_EXPORT_AUTO ComputeShader_SetName(cs<ComputeShader*> s, cs_string name)
{
    ComputeShaderBinding::SetName(s, name);
}

NATIVE_EXPORT_AUTO ComputeShader_GetKernels(cs<ComputeShader*> s, cs<cs<CSharpComputeShaderKernel[]>*> kernels)
{
    ComputeShaderBinding::GetKernels(s, kernels);
}

NATIVE_EXPORT_AUTO ComputeShader_SetKernels(cs<ComputeShader*> s, cs<CSharpComputeShaderKernel[]> kernels)
{
    ComputeShaderBinding::SetKernels(s, kernels);
}

NATIVE_EXPORT_AUTO ComputeShader_Compile(cs<ComputeShader*> s, cs_string filename, cs_string source, cs<cs_string[]> pragmas, cs<cs<cs_string[]>*> warnings, cs<cs_string*> error)
{
    retcs ComputeShaderBinding::Compile(s, filename, source, pragmas, warnings, error);
}
