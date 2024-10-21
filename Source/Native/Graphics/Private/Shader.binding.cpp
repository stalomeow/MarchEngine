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

struct CSharpShaderPassBlendFormula
{
    cs<ShaderPassBlend> Src;
    cs<ShaderPassBlend> Dest;
    cs<ShaderPassBlendOp> Op;
};

struct CSharpShaderPassBlendState
{
    cs_bool Enable;
    cs<ShaderPassColorWriteMask> WriteMask;
    CSharpShaderPassBlendFormula Rgb;
    CSharpShaderPassBlendFormula Alpha;
};

struct CSharpShaderPassDepthState
{
    cs_bool Enable;
    cs_bool Write;
    cs<ShaderPassCompareFunc> Compare;
};

struct CSharpShaderPassStencilAction
{
    cs<ShaderPassCompareFunc> Compare;
    cs<ShaderPassStencilOp> PassOp;
    cs<ShaderPassStencilOp> FailOp;
    cs<ShaderPassStencilOp> DepthFailOp;
};

struct CSharpShaderPassStencilState
{
    march::cs_byte Ref;
    cs_bool Enable;
    march::cs_byte ReadMask;
    march::cs_byte WriteMask;
    CSharpShaderPassStencilAction FrontFace;
    CSharpShaderPassStencilAction BackFace;
};

struct CSharpShaderPass
{
    cs_string Name;
    cs<CSharpShaderPropertyLocation[]> PropertyLocations;
    cs<CSharpShaderProgram[]> Programs;

    cs<ShaderPassCullMode> Cull;
    cs<CSharpShaderPassBlendState[]> Blends;
    CSharpShaderPassDepthState DepthState;
    CSharpShaderPassStencilState StencilState;
};

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
                DEBUG_LOG_ERROR("Unknown shader property type: %d", static_cast<int32_t>(prop->Type.data));
                break;
            }
        }

        inline static void SetPasses(Shader* pShader, cs<CSharpShaderPass[]> passes)
        {
            pShader->m_Version++;
            pShader->m_Passes.clear();
            pShader->m_Passes.resize(static_cast<size_t>(passes.size()));

            for (int32_t i = 0; i < pShader->m_Passes.size(); i++)
            {
                const auto& src = passes[i];
                pShader->m_Passes[i] = std::make_unique<ShaderPass>();
                ShaderPass* pass = pShader->m_Passes[i].get();

                pass->m_Name = src.Name;

                pass->m_PropertyLocations.clear();
                for (int32_t j = 0; j < src.PropertyLocations.size(); j++)
                {
                    const auto& mp = src.PropertyLocations[j];
                    pass->m_PropertyLocations[Shader::GetNameId(mp.Name)] = { mp.Offset, mp.Size };
                }

                for (int32_t j = 0; j < std::size(pass->m_Programs); j++)
                {
                    pass->m_Programs[j] = nullptr;
                }
                for (int32_t j = 0; j < src.Programs.size(); j++)
                {
                    const auto& p = src.Programs[j];
                    std::unique_ptr<ShaderProgram> program = std::make_unique<ShaderProgram>();

                    try
                    {
                        GFX_HR(Shader::GetDxcUtils()->CreateBlob(p.Binary.begin(), p.Binary.size(), DXC_CP_ACP,
                            reinterpret_cast<IDxcBlobEncoding**>(program->m_Binary.ReleaseAndGetAddressOf())));
                    }
                    catch (const std::exception& e)
                    {
                        DEBUG_LOG_ERROR("Failed to create shader blob: %s", e.what());
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

                    pass->m_Programs[static_cast<int32_t>(p.Type.data)] = std::move(program);
                }

                pass->m_Cull = src.Cull;

                pass->m_Blends.resize(src.Blends.size());
                for (int32_t j = 0; j < pass->m_Blends.size(); j++)
                {
                    const auto& blend = src.Blends[j];
                    pass->m_Blends[j].Enable = blend.Enable;
                    pass->m_Blends[j].WriteMask = blend.WriteMask;
                    pass->m_Blends[j].Rgb = { blend.Rgb.Src, blend.Rgb.Dest, blend.Rgb.Op };
                    pass->m_Blends[j].Alpha = { blend.Alpha.Src, blend.Alpha.Dest, blend.Alpha.Op };
                }

                pass->m_DepthState =
                {
                    src.DepthState.Enable,
                    src.DepthState.Write,
                    src.DepthState.Compare
                };

                pass->m_StencilState =
                {
                    src.StencilState.Ref,
                    src.StencilState.Enable,
                    src.StencilState.ReadMask,
                    src.StencilState.WriteMask,
                    {
                        src.StencilState.FrontFace.Compare,
                        src.StencilState.FrontFace.PassOp,
                        src.StencilState.FrontFace.FailOp,
                        src.StencilState.FrontFace.DepthFailOp
                    },
                    {
                        src.StencilState.BackFace.Compare,
                        src.StencilState.BackFace.PassOp,
                        src.StencilState.BackFace.FailOp,
                        src.StencilState.BackFace.DepthFailOp
                    }
                };
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

NATIVE_EXPORT_AUTO Shader_ClearProperties(cs<Shader*> pShader)
{
    ShaderBinding::ClearProperties(pShader);
}

NATIVE_EXPORT_AUTO Shader_SetProperty(cs<Shader*> pShader, cs<CSharpShaderProperty*> prop)
{
    ShaderBinding::SetProperty(pShader, prop);
}

NATIVE_EXPORT_AUTO Shader_GetPassCount(cs<Shader*> pShader)
{
    retcs pShader->GetPassCount();
}

NATIVE_EXPORT_AUTO Shader_GetPasses(cs<Shader*> pShader, cs<CSharpShaderPass[]> passes)
{
    for (int32_t i = 0; i < pShader->GetPassCount(); i++)
    {
        ShaderPass* pass = pShader->GetPass(i);
        CSharpShaderPass& dest = passes[i];

        dest.Name.assign(pass->GetName());

        dest.PropertyLocations.assign(static_cast<int32_t>(pass->GetPropertyLocations().size()));
        int32_t propLocIndex = 0;
        for (auto& kvp : pass->GetPropertyLocations())
        {
            auto& loc = dest.PropertyLocations[propLocIndex++];
            loc.Name.assign(Shader::GetIdName(kvp.first));
            loc.Offset.assign(kvp.second.Offset);
            loc.Size.assign(kvp.second.Size);
        }

        int32_t nonNullProgramCount = 0;
        for (int32_t j = 0; j < static_cast<int32_t>(ShaderProgramType::NumTypes); j++)
        {
            if (pass->GetProgram(static_cast<ShaderProgramType>(j)) != nullptr)
            {
                nonNullProgramCount++;
            }
        }

        dest.Programs.assign(nonNullProgramCount);
        int32_t programIndex = 0;
        for (int32_t j = 0; j < static_cast<int32_t>(ShaderProgramType::NumTypes); j++)
        {
            ShaderProgram* program = pass->GetProgram(static_cast<ShaderProgramType>(j));

            if (program == nullptr)
            {
                continue;
            }

            auto& p = dest.Programs[j];
            p.Type.assign(static_cast<ShaderProgramType>(j));
            p.Binary.assign(static_cast<int32_t>(program->GetBinarySize()), reinterpret_cast<march::cs_byte*>(program->GetBinaryData()));

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

        dest.Cull.assign(pass->GetCull());
        dest.Blends.assign(static_cast<int32_t>(pass->GetBlends().size()));
        for (int32_t j = 0; j < pass->GetBlends().size(); j++)
        {
            auto& blend = dest.Blends[j];
            blend.Enable.assign(pass->GetBlends()[j].Enable);
            blend.WriteMask.assign(pass->GetBlends()[j].WriteMask);
            blend.Rgb.Src.assign(pass->GetBlends()[j].Rgb.Src);
            blend.Rgb.Dest.assign(pass->GetBlends()[j].Rgb.Dest);
            blend.Rgb.Op.assign(pass->GetBlends()[j].Rgb.Op);
            blend.Alpha.Src.assign(pass->GetBlends()[j].Alpha.Src);
            blend.Alpha.Dest.assign(pass->GetBlends()[j].Alpha.Dest);
            blend.Alpha.Op.assign(pass->GetBlends()[j].Alpha.Op);
        }

        dest.DepthState.Enable.assign(pass->GetDepthState().Enable);
        dest.DepthState.Write.assign(pass->GetDepthState().Write);
        dest.DepthState.Compare.assign(pass->GetDepthState().Compare);
        dest.StencilState.Ref.assign(pass->GetStencilState().Ref);
        dest.StencilState.Enable.assign(pass->GetStencilState().Enable);
        dest.StencilState.ReadMask.assign(pass->GetStencilState().ReadMask);
        dest.StencilState.WriteMask.assign(pass->GetStencilState().WriteMask);
        dest.StencilState.FrontFace.Compare.assign(pass->GetStencilState().FrontFace.Compare);
        dest.StencilState.FrontFace.PassOp.assign(pass->GetStencilState().FrontFace.PassOp);
        dest.StencilState.FrontFace.FailOp.assign(pass->GetStencilState().FrontFace.FailOp);
        dest.StencilState.FrontFace.DepthFailOp.assign(pass->GetStencilState().FrontFace.DepthFailOp);
        dest.StencilState.BackFace.Compare.assign(pass->GetStencilState().BackFace.Compare);
        dest.StencilState.BackFace.PassOp.assign(pass->GetStencilState().BackFace.PassOp);
        dest.StencilState.BackFace.FailOp.assign(pass->GetStencilState().BackFace.FailOp);
        dest.StencilState.BackFace.DepthFailOp.assign(pass->GetStencilState().BackFace.DepthFailOp);
    }
}

NATIVE_EXPORT_AUTO Shader_SetPasses(cs<Shader*> pShader, cs<CSharpShaderPass[]> passes)
{
    ShaderBinding::SetPasses(pShader, passes);
}

NATIVE_EXPORT_AUTO Shader_CompilePass(cs<Shader*> pShader, cs_int passIndex, cs_string filename, cs_string program, cs_string entrypoint, cs_string shaderModel, cs<ShaderProgramType> programType)
{
    retcs pShader->CompilePass(passIndex, filename, program, entrypoint, shaderModel, programType);
}

NATIVE_EXPORT_AUTO Shader_CreatePassRootSignature(cs<Shader*> pShader, cs_int passIndex)
{
    pShader->GetPass(passIndex)->CreateRootSignature();
}

NATIVE_EXPORT_AUTO Shader_GetEngineShaderPathUnixStyle()
{
    retcs Shader::GetEngineShaderPathUnixStyle();
}
