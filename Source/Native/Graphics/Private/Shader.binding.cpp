#include "Shader.h"
#include "InteropServices.h"

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

struct CSharpShaderPassConstantBuffer
{
    cs_string Name;
    cs_uint ShaderRegister;
    cs_uint RegisterSpace;
    cs_uint Size;
};

struct CSharpShaderPassSampler
{
    cs_string Name;
    cs_uint ShaderRegister;
    cs_uint RegisterSpace;
};

struct CSharpShaderPassMaterialProperty
{
    cs_string Name;
    cs_uint Offset;
    cs_uint Size;
};

struct CSharpShaderPassTextureProperty
{
    cs_string Name;
    cs_uint ShaderRegisterTexture;
    cs_uint RegisterSpaceTexture;
    cs_bool HasSampler;
    cs_uint ShaderRegisterSampler;
    cs_uint RegisterSpaceSampler;
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
    cs_bool Enable;
    march::cs_byte ReadMask;
    march::cs_byte WriteMask;
    CSharpShaderPassStencilAction FrontFace;
    CSharpShaderPassStencilAction BackFace;
};

struct CSharpShaderPass
{
    cs_string Name;

    cs<march::cs_byte[]> VertexShader;
    cs<march::cs_byte[]> PixelShader;

    cs<CSharpShaderPassConstantBuffer[]> ConstantBuffers;
    cs<CSharpShaderPassSampler[]> Samplers;
    cs<CSharpShaderPassMaterialProperty[]> MaterialProperties;
    cs<CSharpShaderPassTextureProperty[]> TextureProperties;

    cs<ShaderPassCullMode> Cull;
    cs<CSharpShaderPassBlendState[]> Blends;
    CSharpShaderPassDepthState DepthState;
    CSharpShaderPassStencilState StencilState;
};

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
    pShader->Version++;
    pShader->Properties.clear();
}

NATIVE_EXPORT_AUTO Shader_SetProperty(cs<Shader*> pShader, cs<CSharpShaderProperty*> prop)
{
    pShader->Version++;
    pShader->Properties[prop->Name] =
    {
        prop->Type,
        prop->DefaultFloat,
        prop->DefaultInt,
        prop->DefaultColor,
        prop->DefaultVector,
        prop->DefaultTexture,
    };
}

NATIVE_EXPORT_AUTO Shader_GetPassCount(cs<Shader*> pShader)
{
    retcs static_cast<int32_t>(pShader->Passes.size());
}

NATIVE_EXPORT_AUTO Shader_GetPasses(cs<Shader*> pShader, cs<CSharpShaderPass[]> passes)
{
    for (int32_t i = 0; i < pShader->Passes.size(); i++)
    {
        ShaderPass* pass = pShader->Passes[i].get();
        CSharpShaderPass& dest = passes[i];

        dest.Name.assign(pass->Name);

        dest.VertexShader.assign(static_cast<int32_t>(pass->VertexShader->GetBufferSize()),
            reinterpret_cast<march::cs_byte*>(pass->VertexShader->GetBufferPointer()));
        dest.PixelShader.assign(static_cast<int32_t>(pass->PixelShader->GetBufferSize()),
            reinterpret_cast<march::cs_byte*>(pass->PixelShader->GetBufferPointer()));

        dest.ConstantBuffers.assign(static_cast<int32_t>(pass->ConstantBuffers.size()));
        int32_t cbIndex = 0;
        for (auto& kvp : pass->ConstantBuffers)
        {
            auto& cb = dest.ConstantBuffers[cbIndex++];
            cb.Name.assign(kvp.first);
            cb.ShaderRegister.assign(kvp.second.ShaderRegister);
            cb.RegisterSpace.assign(kvp.second.RegisterSpace);
            cb.Size.assign(kvp.second.Size);
        }

        dest.Samplers.assign(static_cast<int32_t>(pass->Samplers.size()));
        int32_t samplerIndex = 0;
        for (auto& kvp : pass->Samplers)
        {
            auto& sampler = dest.Samplers[samplerIndex++];
            sampler.Name.assign(kvp.first);
            sampler.ShaderRegister.assign(kvp.second.ShaderRegister);
            sampler.RegisterSpace.assign(kvp.second.RegisterSpace);
        }

        dest.MaterialProperties.assign(static_cast<int32_t>(pass->MaterialProperties.size()));
        int32_t mpIndex = 0;
        for (auto& kvp : pass->MaterialProperties)
        {
            auto& mp = dest.MaterialProperties[mpIndex++];
            mp.Name.assign(kvp.first);
            mp.Offset.assign(kvp.second.Offset);
            mp.Size.assign(kvp.second.Size);
        }

        dest.TextureProperties.assign(static_cast<int32_t>(pass->TextureProperties.size()));
        int32_t tpIndex = 0;
        for (auto& kvp : pass->TextureProperties)
        {
            auto& tp = dest.TextureProperties[tpIndex++];
            tp.Name.assign(kvp.first);
            tp.ShaderRegisterTexture.assign(kvp.second.ShaderRegisterTexture);
            tp.RegisterSpaceTexture.assign(kvp.second.RegisterSpaceTexture);
            tp.HasSampler.assign(kvp.second.HasSampler);
            tp.ShaderRegisterSampler.assign(kvp.second.ShaderRegisterSampler);
            tp.RegisterSpaceSampler.assign(kvp.second.RegisterSpaceSampler);
        }

        dest.Cull.assign(pass->Cull);
        dest.Blends.assign(static_cast<int32_t>(pass->Blends.size()));
        for (int32_t j = 0; j < pass->Blends.size(); j++)
        {
            auto& blend = dest.Blends[j];
            blend.Enable.assign(pass->Blends[j].Enable);
            blend.WriteMask.assign(pass->Blends[j].WriteMask);
            blend.Rgb.Src.assign(pass->Blends[j].Rgb.Src);
            blend.Rgb.Dest.assign(pass->Blends[j].Rgb.Dest);
            blend.Rgb.Op.assign(pass->Blends[j].Rgb.Op);
            blend.Alpha.Src.assign(pass->Blends[j].Alpha.Src);
            blend.Alpha.Dest.assign(pass->Blends[j].Alpha.Dest);
            blend.Alpha.Op.assign(pass->Blends[j].Alpha.Op);
        }
        dest.DepthState.Enable.assign(pass->DepthState.Enable);
        dest.DepthState.Write.assign(pass->DepthState.Write);
        dest.DepthState.Compare.assign(pass->DepthState.Compare);
        dest.StencilState.Enable.assign(pass->StencilState.Enable);
        dest.StencilState.ReadMask.assign(pass->StencilState.ReadMask);
        dest.StencilState.WriteMask.assign(pass->StencilState.WriteMask);
        dest.StencilState.FrontFace.Compare.assign(pass->StencilState.FrontFace.Compare);
        dest.StencilState.FrontFace.PassOp.assign(pass->StencilState.FrontFace.PassOp);
        dest.StencilState.FrontFace.FailOp.assign(pass->StencilState.FrontFace.FailOp);
        dest.StencilState.FrontFace.DepthFailOp.assign(pass->StencilState.FrontFace.DepthFailOp);
        dest.StencilState.BackFace.Compare.assign(pass->StencilState.BackFace.Compare);
        dest.StencilState.BackFace.PassOp.assign(pass->StencilState.BackFace.PassOp);
        dest.StencilState.BackFace.FailOp.assign(pass->StencilState.BackFace.FailOp);
        dest.StencilState.BackFace.DepthFailOp.assign(pass->StencilState.BackFace.DepthFailOp);
    }
}

NATIVE_EXPORT_AUTO Shader_SetPasses(cs<Shader*> pShader, cs<CSharpShaderPass[]> passes)
{
    pShader->Version++;
    pShader->Passes.resize(static_cast<size_t>(passes.size()));

    for (int32_t i = 0; i < pShader->Passes.size(); i++)
    {
        const auto& src = passes[i];
        pShader->Passes[i] = std::make_unique<ShaderPass>();
        ShaderPass* pass = pShader->Passes[i].get();

        pass->Name = src.Name;

        GFX_HR(Shader::GetDxcUtils()->CreateBlob(
            src.VertexShader.begin(), src.VertexShader.size(), DXC_CP_ACP,
            reinterpret_cast<IDxcBlobEncoding**>(pass->VertexShader.ReleaseAndGetAddressOf())));
        GFX_HR(Shader::GetDxcUtils()->CreateBlob(
            src.PixelShader.begin(), src.PixelShader.size(), DXC_CP_ACP,
            reinterpret_cast<IDxcBlobEncoding**>(pass->PixelShader.ReleaseAndGetAddressOf())));

        pass->ConstantBuffers.clear();
        for (int32_t j = 0; j < src.ConstantBuffers.size(); j++)
        {
            const auto& cb = src.ConstantBuffers[j];
            pass->ConstantBuffers[cb.Name] = { cb.ShaderRegister, cb.RegisterSpace, cb.Size };
        }

        pass->Samplers.clear();
        for (int32_t j = 0; j < src.Samplers.size(); j++)
        {
            const auto& sampler = src.Samplers[j];
            pass->Samplers[sampler.Name] = { sampler.ShaderRegister, sampler.RegisterSpace };
        }

        pass->MaterialProperties.clear();
        for (int32_t j = 0; j < src.MaterialProperties.size(); j++)
        {
            const auto& mp = src.MaterialProperties[j];
            pass->MaterialProperties[mp.Name] = { mp.Offset, mp.Size };
        }

        pass->TextureProperties.clear();
        for (int32_t j = 0; j < src.TextureProperties.size(); j++)
        {
            const auto& tp = src.TextureProperties[j];
            pass->TextureProperties[tp.Name] =
            {
                tp.ShaderRegisterTexture,
                tp.RegisterSpaceTexture,
                tp.HasSampler,
                tp.ShaderRegisterSampler,
                tp.RegisterSpaceSampler
            };
        }

        pass->Cull = src.Cull;

        pass->Blends.resize(src.Blends.size());
        for (int32_t j = 0; j < pass->Blends.size(); j++)
        {
            const auto& blend = src.Blends[j];
            pass->Blends[j].Enable = blend.Enable;
            pass->Blends[j].WriteMask = blend.WriteMask;
            pass->Blends[j].Rgb = { blend.Rgb.Src, blend.Rgb.Dest, blend.Rgb.Op };
            pass->Blends[j].Alpha = { blend.Alpha.Src, blend.Alpha.Dest, blend.Alpha.Op };
        }

        pass->DepthState =
        {
            src.DepthState.Enable,
            src.DepthState.Write,
            src.DepthState.Compare
        };

        pass->StencilState =
        {
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

NATIVE_EXPORT_AUTO Shader_CompilePass(cs<Shader*> pShader, cs_int passIndex, cs_string filename, cs_string program, cs_string entrypoint, cs_string shaderModel, cs<ShaderProgramType> programType)
{
    retcs pShader->CompilePass(passIndex, filename, program, entrypoint, shaderModel, programType);
}

NATIVE_EXPORT_AUTO Shader_CreatePassRootSignature(cs<Shader*> pShader, cs_int passIndex)
{
    pShader->Passes[passIndex]->CreateRootSignature();
}
