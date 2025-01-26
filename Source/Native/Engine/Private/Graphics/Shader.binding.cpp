#include "pch.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxTexture.h"
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
    cs_uint ConstantBufferSize;
};

struct CSharpShaderProgram
{
    cs<ShaderProgramType> Type;
    cs<cs_string[]> Keywords;
    cs<march::cs_byte[]> Hash;
    cs<march::cs_byte[]> Binary;
    cs<CSharpShaderBuffer[]> SrvCbvBuffers;
    cs<CSharpShaderTexture[]> SrvTextures;
    cs<CSharpShaderBuffer[]> UavBuffers;
    cs<CSharpShaderTexture[]> UavTextures;
    cs<CSharpShaderStaticSampler[]> StaticSamplers;
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
    cs<CSharpShaderPropertyLocation[]> PropertyLocations;
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
    class ShaderBinding
    {
    public:
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

            auto& p = pShader->m_Properties[Shader::GetNameId(prop->Name)];
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
            pShader->m_KeywordSpace.Clear();
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

                pass->m_PropertyLocations.clear();
                for (int32_t j = 0; j < src.PropertyLocations.size(); j++)
                {
                    const auto& mp = src.PropertyLocations[j];
                    pass->m_PropertyLocations[Shader::GetNameId(mp.Name)] = { mp.Offset, mp.Size };
                }

                for (int32_t j = 0; j < std::size(pass->m_Programs); j++)
                {
                    pass->m_Programs[j].clear();
                }
                for (int32_t j = 0; j < src.Programs.size(); j++)
                {
                    const auto& p = src.Programs[j];
                    std::unique_ptr<ShaderProgram> program = std::make_unique<ShaderProgram>();

                    for (int32_t k = 0; k < p.Keywords.size(); k++)
                    {
                        pShader->m_KeywordSpace.AddKeyword(p.Keywords[k]);
                        program->m_Keywords.EnableKeyword(pShader->m_KeywordSpace, p.Keywords[k]);
                    }

                    std::copy(p.Hash.begin(), p.Hash.end(), program->m_Hash.Data);

                    try
                    {
                        GFX_HR(Shader::GetDxcUtils()->CreateBlob(p.Binary.begin(), p.Binary.size(), DXC_CP_ACP,
                            reinterpret_cast<IDxcBlobEncoding**>(program->m_Binary.ReleaseAndGetAddressOf())));
                    }
                    catch (const std::exception& e)
                    {
                        LOG_ERROR("Failed to create shader blob: {}", e.what());
                        return;
                    }

                    for (int32_t k = 0; k < p.SrvCbvBuffers.size(); k++)
                    {
                        const auto& buf = p.SrvCbvBuffers[k];
                        program->m_SrvCbvBuffers.push_back({ Shader::GetNameId(buf.Name), buf.ShaderRegister, buf.RegisterSpace, buf.ConstantBufferSize });
                    }

                    for (int32_t k = 0; k < p.SrvTextures.size(); k++)
                    {
                        const auto& tp = p.SrvTextures[k];
                        program->m_SrvTextures.push_back({
                            Shader::GetNameId(tp.Name),
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
                        program->m_UavBuffers.push_back({ Shader::GetNameId(buf.Name), buf.ShaderRegister, buf.RegisterSpace, buf.ConstantBufferSize });
                    }

                    for (int32_t k = 0; k < p.UavTextures.size(); k++)
                    {
                        const auto& tp = p.UavTextures[k];
                        program->m_UavTextures.push_back({
                            Shader::GetNameId(tp.Name),
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
                        program->m_StaticSamplers[Shader::GetNameId(sampler.Name)] = { sampler.ShaderRegister, sampler.RegisterSpace };
                    }

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

        inline static bool CompilePass(cs<Shader*> pShader, cs_int passIndex, cs_string filename, cs_string source, cs<cs<cs_string[]>*> warnings, cs<cs_string*> error)
        {
            pShader->m_Version++;

            std::vector<std::string> warningBuffer{};
            std::string errorBuffer{};

            ShaderPass* pass = pShader->GetPass(passIndex);
            bool ret = pass->Compile(pShader->m_KeywordSpace, filename, source, warningBuffer, errorBuffer);

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

                dest.PropertyLocations.assign(static_cast<int32_t>(pass->GetPropertyLocations().size()));
                int32_t propLocIndex = 0;
                for (auto& kvp : pass->GetPropertyLocations())
                {
                    auto& loc = dest.PropertyLocations[propLocIndex++];
                    loc.Name.assign(Shader::GetIdName(kvp.first));
                    loc.Offset.assign(kvp.second.Offset);
                    loc.Size.assign(kvp.second.Size);
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
                        p.Type.assign(static_cast<ShaderProgramType>(j));

                        std::vector<std::string> keywords = program->m_Keywords.GetEnabledKeywords(pShader->m_KeywordSpace);
                        p.Keywords.assign(static_cast<int32_t>(keywords.size()));

                        for (int32_t k = 0; k < keywords.size(); k++)
                        {
                            p.Keywords[k].assign(std::move(keywords[k]));
                        }

                        p.Hash.assign(static_cast<int32_t>(std::size(program->GetHash().Data)), reinterpret_cast<const march::cs_byte*>(program->GetHash().Data));
                        p.Binary.assign(static_cast<int32_t>(program->GetBinarySize()), reinterpret_cast<const march::cs_byte*>(program->GetBinaryData()));

                        p.SrvCbvBuffers.assign(static_cast<int32_t>(program->GetSrvCbvBuffers().size()));
                        for (size_t bufIdx = 0; bufIdx < program->GetSrvCbvBuffers().size(); bufIdx++)
                        {
                            auto& buf = p.SrvCbvBuffers[static_cast<int32_t>(bufIdx)];
                            buf.Name.assign(Shader::GetIdName(program->GetSrvCbvBuffers()[bufIdx].Id));
                            buf.ShaderRegister.assign(program->GetSrvCbvBuffers()[bufIdx].ShaderRegister);
                            buf.RegisterSpace.assign(program->GetSrvCbvBuffers()[bufIdx].RegisterSpace);
                            buf.ConstantBufferSize.assign(program->GetSrvCbvBuffers()[bufIdx].ConstantBufferSize);
                        }

                        p.SrvTextures.assign(static_cast<int32_t>(program->GetSrvTextures().size()));
                        for (size_t texIdx = 0; texIdx < program->GetSrvTextures().size(); texIdx++)
                        {
                            auto& tp = p.SrvTextures[static_cast<int32_t>(texIdx)];
                            tp.Name.assign(Shader::GetIdName(program->GetSrvTextures()[texIdx].Id));
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
                            buf.Name.assign(Shader::GetIdName(program->GetUavBuffers()[bufIdx].Id));
                            buf.ShaderRegister.assign(program->GetUavBuffers()[bufIdx].ShaderRegister);
                            buf.RegisterSpace.assign(program->GetUavBuffers()[bufIdx].RegisterSpace);
                            buf.ConstantBufferSize.assign(program->GetUavBuffers()[bufIdx].ConstantBufferSize);
                        }

                        p.UavTextures.assign(static_cast<int32_t>(program->GetUavTextures().size()));
                        for (size_t texIdx = 0; texIdx < program->GetUavTextures().size(); texIdx++)
                        {
                            auto& tp = p.UavTextures[static_cast<int32_t>(texIdx)];
                            tp.Name.assign(Shader::GetIdName(program->GetUavTextures()[texIdx].Id));
                            tp.ShaderRegisterTexture.assign(program->GetUavTextures()[texIdx].ShaderRegisterTexture);
                            tp.RegisterSpaceTexture.assign(program->GetUavTextures()[texIdx].RegisterSpaceTexture);
                            tp.HasSampler.assign(program->GetUavTextures()[texIdx].HasSampler);
                            tp.ShaderRegisterSampler.assign(program->GetUavTextures()[texIdx].ShaderRegisterSampler);
                            tp.RegisterSpaceSampler.assign(program->GetUavTextures()[texIdx].RegisterSpaceSampler);
                        }

                        p.StaticSamplers.assign(static_cast<int32_t>(program->GetStaticSamplers().size()));
                        int32_t samplerIndex = 0;
                        for (auto& kvp : program->GetStaticSamplers())
                        {
                            auto& sampler = p.StaticSamplers[samplerIndex++];
                            sampler.Name.assign(Shader::GetIdName(kvp.first));
                            sampler.ShaderRegister.assign(kvp.second.ShaderRegister);
                            sampler.RegisterSpace.assign(kvp.second.RegisterSpace);
                        }
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

NATIVE_EXPORT_AUTO Shader_CompilePass(cs<Shader*> pShader, cs_int passIndex, cs_string filename, cs_string source, cs<cs<cs_string[]>*> warnings, cs<cs_string*> error)
{
    retcs ShaderBinding::CompilePass(pShader, passIndex, filename, source, warnings, error);
}

NATIVE_EXPORT_AUTO Shader_GetEngineShaderPathUnixStyle()
{
    retcs Shader::GetEngineShaderPathUnixStyle();
}

NATIVE_EXPORT_AUTO Shader_GetNameId(cs_string name)
{
    retcs Shader::GetNameId(name);
}

NATIVE_EXPORT_AUTO Shader_GetIdName(cs_int id)
{
    retcs Shader::GetIdName(id);
}
