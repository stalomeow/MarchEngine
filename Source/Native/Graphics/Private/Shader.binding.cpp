#include "Shader.h"
#include "InteropServices.h"
#include "Debug.h"
#include "GfxExcept.h"
#include <exception>

struct CSharpShaderConstantBuffer
{
    cs_string Name;
    cs_uint ShaderRegister;
    cs_uint RegisterSpace;
    cs_uint UnalignedSize;
};

struct CSharpShaderStaticSampler
{
    cs_string Name;
    cs_uint ShaderRegister;
    cs_uint RegisterSpace;
};

struct CSharpShaderTexture
{
    cs_string Name;
    cs_uint ShaderRegisterTexture;
    cs_uint RegisterSpaceTexture;
    cs_bool HasSampler;
    cs_uint ShaderRegisterSampler;
    cs_uint RegisterSpaceSampler;
};

struct CSharpShaderProgram
{
    cs<ShaderProgramType> Type;
    cs<cs_string[]> Keywords;
    cs<march::cs_byte[]> Hash;
    cs<march::cs_byte[]> Binary;
    cs<CSharpShaderConstantBuffer[]> ConstantBuffers;
    cs<CSharpShaderStaticSampler[]> StaticSamplers;
    cs<CSharpShaderTexture[]> Textures;
};

struct CSharpShaderProperty
{
    cs_string Name;
    cs<ShaderPropertyType> Type;

    cs_float DefaultFloat;
    cs_int DefaultInt;
    cs_color DefaultColor;
    cs_vec4 DefaultVector;
    cs<ShaderDefaultTexture> DefaultTexture;
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
                p.DefaultValue.Float = prop->DefaultFloat;
                break;
            case ShaderPropertyType::Int:
                p.DefaultValue.Int = prop->DefaultInt;
                break;
            case ShaderPropertyType::Color:
                p.DefaultValue.Color = prop->DefaultColor;
                break;
            case ShaderPropertyType::Vector:
                p.DefaultValue.Vector = prop->DefaultVector;
                break;
            case ShaderPropertyType::Texture:
                p.DefaultValue.Texture = prop->DefaultTexture;
                break;
            default:
                LOG_ERROR("Unknown shader property type: %d", static_cast<int32_t>(prop->Type.data));
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
                pShader->m_Passes[i] = std::make_unique<ShaderPass>(pShader);
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

                    std::copy(p.Hash.begin(), p.Hash.end(), program->m_Hash);

                    try
                    {
                        GFX_HR(Shader::GetDxcUtils()->CreateBlob(p.Binary.begin(), p.Binary.size(), DXC_CP_ACP,
                            reinterpret_cast<IDxcBlobEncoding**>(program->m_Binary.ReleaseAndGetAddressOf())));
                    }
                    catch (const std::exception& e)
                    {
                        LOG_ERROR("Failed to create shader blob: %s", e.what());
                        return;
                    }

                    for (int32_t k = 0; k < p.ConstantBuffers.size(); k++)
                    {
                        const auto& cb = p.ConstantBuffers[k];
                        program->m_ConstantBuffers[Shader::GetNameId(cb.Name)] = { cb.ShaderRegister, cb.RegisterSpace, cb.UnalignedSize };
                    }

                    for (int32_t k = 0; k < p.StaticSamplers.size(); k++)
                    {
                        const auto& sampler = p.StaticSamplers[k];
                        program->m_StaticSamplers[Shader::GetNameId(sampler.Name)] = { sampler.ShaderRegister, sampler.RegisterSpace };
                    }

                    for (int32_t k = 0; k < p.Textures.size(); k++)
                    {
                        const auto& tp = p.Textures[k];
                        program->m_Textures[Shader::GetNameId(tp.Name)] =
                        {
                            tp.ShaderRegisterTexture,
                            tp.RegisterSpaceTexture,
                            tp.HasSampler,
                            tp.ShaderRegisterSampler,
                            tp.RegisterSpaceSampler
                        };
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
            bool ret = pass->Compile(filename, source, warningBuffer, errorBuffer);

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
            passes->assign(pShader->GetPassCount());

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
                for (int32_t j = 0; j < static_cast<int32_t>(ShaderProgramType::NumTypes); j++)
                {
                    programCount += static_cast<int32_t>(pass->m_Programs[j].size());
                }

                dest.Programs.assign(programCount);
                int32_t programIndex = 0;
                for (int32_t j = 0; j < static_cast<int32_t>(ShaderProgramType::NumTypes); j++)
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

                        p.Hash.assign(static_cast<int32_t>(std::size(program->GetHash())), reinterpret_cast<const march::cs_byte*>(program->GetHash()));
                        p.Binary.assign(static_cast<int32_t>(program->GetBinarySize()), reinterpret_cast<const march::cs_byte*>(program->GetBinaryData()));

                        p.ConstantBuffers.assign(static_cast<int32_t>(program->GetConstantBuffers().size()));
                        int32_t cbIndex = 0;
                        for (auto& kvp : program->GetConstantBuffers())
                        {
                            auto& cb = p.ConstantBuffers[cbIndex++];
                            cb.Name.assign(Shader::GetIdName(kvp.first));
                            cb.ShaderRegister.assign(kvp.second.ShaderRegister);
                            cb.RegisterSpace.assign(kvp.second.RegisterSpace);
                            cb.UnalignedSize.assign(kvp.second.UnalignedSize);
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

                        p.Textures.assign(static_cast<int32_t>(program->GetTextures().size()));
                        int32_t tpIndex = 0;
                        for (auto& kvp : program->GetTextures())
                        {
                            auto& tp = p.Textures[tpIndex++];
                            tp.Name.assign(Shader::GetIdName(kvp.first));
                            tp.ShaderRegisterTexture.assign(kvp.second.ShaderRegisterTexture);
                            tp.RegisterSpaceTexture.assign(kvp.second.RegisterSpaceTexture);
                            tp.HasSampler.assign(kvp.second.HasSampler);
                            tp.ShaderRegisterSampler.assign(kvp.second.ShaderRegisterSampler);
                            tp.RegisterSpaceSampler.assign(kvp.second.RegisterSpaceSampler);
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
    retcs DBG_NEW Shader();
}

NATIVE_EXPORT_AUTO Shader_Delete(cs<Shader*> pShader)
{
    delete pShader;
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
