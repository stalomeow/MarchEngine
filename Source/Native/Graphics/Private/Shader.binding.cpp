#include "Shader.h"
#include "InteropServices.h"

using namespace march;

struct CSharpShaderProperty
{
    CSharpString Name;
    CSharpInt Type;

    CSharpFloat DefaultFloat;
    CSharpInt DefaultInt;
    CSharpColor DefaultColor;
    CSharpVector4 DefaultVector;
    CSharpInt DefaultTexture;
};

struct CSharpShaderPassConstantBuffer
{
    CSharpString Name;
    CSharpUInt ShaderRegister;
    CSharpUInt RegisterSpace;
    CSharpUInt Size;
};

struct CSharpShaderPassSampler
{
    CSharpString Name;
    CSharpUInt ShaderRegister;
    CSharpUInt RegisterSpace;
};

struct CSharpShaderPassMaterialProperty
{
    CSharpString Name;
    CSharpUInt Offset;
    CSharpUInt Size;
};

struct CSharpShaderPassTextureProperty
{
    CSharpString Name;
    CSharpUInt ShaderRegisterTexture;
    CSharpUInt RegisterSpaceTexture;
    CSharpBool HasSampler;
    CSharpUInt ShaderRegisterSampler;
    CSharpUInt RegisterSpaceSampler;
};

struct CSharpShaderPassBlendFormula
{
    CSharpInt Src;
    CSharpInt Dest;
    CSharpInt Op;
};

struct CSharpShaderPassBlendState
{
    CSharpBool Enable;
    CSharpInt WriteMask;
    CSharpShaderPassBlendFormula Rgb;
    CSharpShaderPassBlendFormula Alpha;
};

struct CSharpShaderPassDepthState
{
    CSharpBool Enable;
    CSharpBool Write;
    CSharpInt Compare;
};

struct CSharpShaderPassStencilAction
{
    CSharpInt Compare;
    CSharpInt PassOp;
    CSharpInt FailOp;
    CSharpInt DepthFailOp;
};

struct CSharpShaderPassStencilState
{
    CSharpBool Enable;
    CSharpByte ReadMask;
    CSharpByte WriteMask;
    CSharpShaderPassStencilAction FrontFace;
    CSharpShaderPassStencilAction BackFace;
};

struct CSharpShaderPass
{
    CSharpString Name;

    CSharpArray VertexShader;
    CSharpArray PixelShader;

    CSharpArray ConstantBuffers;
    CSharpArray Samplers;
    CSharpArray MaterialProperties;
    CSharpArray TextureProperties;

    CSharpInt Cull;
    CSharpArray Blends;
    CSharpShaderPassDepthState DepthState;
    CSharpShaderPassStencilState StencilState;
};

NATIVE_EXPORT(Shader*) Shader_New()
{
    return new Shader();
}

NATIVE_EXPORT(void) Shader_Delete(Shader* pShader)
{
    delete pShader;
}

NATIVE_EXPORT(void) Shader_ClearProperties(Shader* pShader)
{
    pShader->Version++;
    pShader->Properties.clear();
}

NATIVE_EXPORT(void) Shader_SetProperty(Shader* pShader, CSharpShaderProperty* prop)
{
    pShader->Version++;
    pShader->Properties[CSharpString_ToUtf8(prop->Name)] =
    {
        static_cast<ShaderPropertyType>(prop->Type),
        prop->DefaultFloat,
        prop->DefaultInt,
        ToXMFLOAT4(prop->DefaultColor),
        ToXMFLOAT4(prop->DefaultVector),
        static_cast<ShaderDefaultTexture>(prop->DefaultTexture)
    };
}

NATIVE_EXPORT(CSharpInt) Shader_GetPassCount(Shader* pShader)
{
    return static_cast<CSharpInt>(pShader->Passes.size());
}

NATIVE_EXPORT(void) Shader_GetPasses(Shader* pShader, CSharpArray passes)
{
    for (int i = 0; i < pShader->Passes.size(); i++)
    {
        ShaderPass* pass = pShader->Passes[i].get();
        auto& cs = CSharpArray_Get<CSharpShaderPass>(passes, i);

        cs.Name = CSharpString_FromUtf8(pass->Name);

        cs.VertexShader = CSharpArray_New<CSharpByte>(pass->VertexShader->GetBufferSize());
        CSharpArray_CopyFrom(cs.VertexShader, pass->VertexShader->GetBufferPointer());

        cs.PixelShader = CSharpArray_New<CSharpByte>(pass->PixelShader->GetBufferSize());
        CSharpArray_CopyFrom(cs.PixelShader, pass->PixelShader->GetBufferPointer());

        cs.ConstantBuffers = CSharpArray_New<CSharpShaderPassConstantBuffer>(pass->ConstantBuffers.size());
        int cbIndex = 0;
        for (auto& kvp : pass->ConstantBuffers)
        {
            auto& cb = CSharpArray_Get<CSharpShaderPassConstantBuffer>(cs.ConstantBuffers, cbIndex++);
            cb.Name = CSharpString_FromUtf8(kvp.first);
            cb.ShaderRegister = kvp.second.ShaderRegister;
            cb.RegisterSpace = kvp.second.RegisterSpace;
            cb.Size = kvp.second.Size;
        }

        cs.Samplers = CSharpArray_New<CSharpShaderPassSampler>(pass->Samplers.size());
        int samplerIndex = 0;
        for (auto& kvp : pass->Samplers)
        {
            auto& sampler = CSharpArray_Get<CSharpShaderPassSampler>(cs.Samplers, samplerIndex++);
            sampler.Name = CSharpString_FromUtf8(kvp.first);
            sampler.ShaderRegister = kvp.second.ShaderRegister;
            sampler.RegisterSpace = kvp.second.RegisterSpace;
        }

        cs.MaterialProperties = CSharpArray_New<CSharpShaderPassMaterialProperty>(pass->MaterialProperties.size());
        int mpIndex = 0;
        for (auto& kvp : pass->MaterialProperties)
        {
            auto& mp = CSharpArray_Get<CSharpShaderPassMaterialProperty>(cs.MaterialProperties, mpIndex++);
            mp.Name = CSharpString_FromUtf8(kvp.first);
            mp.Offset = kvp.second.Offset;
            mp.Size = kvp.second.Size;
        }

        cs.TextureProperties = CSharpArray_New<CSharpShaderPassTextureProperty>(pass->TextureProperties.size());
        int tpIndex = 0;
        for (auto& kvp : pass->TextureProperties)
        {
            auto& tp = CSharpArray_Get<CSharpShaderPassTextureProperty>(cs.TextureProperties, tpIndex++);
            tp.Name = CSharpString_FromUtf8(kvp.first);
            tp.ShaderRegisterTexture = kvp.second.ShaderRegisterTexture;
            tp.RegisterSpaceTexture = kvp.second.RegisterSpaceTexture;
            tp.HasSampler = CSHARP_MARSHAL_BOOL(kvp.second.HasSampler);
            tp.ShaderRegisterSampler = kvp.second.ShaderRegisterSampler;
            tp.RegisterSpaceSampler = kvp.second.RegisterSpaceSampler;
        }

        cs.Cull = static_cast<CSharpInt>(pass->Cull);
        cs.Blends = CSharpArray_New<CSharpShaderPassBlendState>(pass->Blends.size());
        for (int j = 0; j < pass->Blends.size(); j++)
        {
            auto& blend = CSharpArray_Get<CSharpShaderPassBlendState>(cs.Blends, j);
            blend.Enable = CSHARP_MARSHAL_BOOL(pass->Blends[j].Enable);
            blend.WriteMask = static_cast<CSharpInt>(pass->Blends[j].WriteMask);
            blend.Rgb.Src = static_cast<CSharpInt>(pass->Blends[j].Rgb.Src);
            blend.Rgb.Dest = static_cast<CSharpInt>(pass->Blends[j].Rgb.Dest);
            blend.Rgb.Op = static_cast<CSharpInt>(pass->Blends[j].Rgb.Op);
            blend.Alpha.Src = static_cast<CSharpInt>(pass->Blends[j].Alpha.Src);
            blend.Alpha.Dest = static_cast<CSharpInt>(pass->Blends[j].Alpha.Dest);
            blend.Alpha.Op = static_cast<CSharpInt>(pass->Blends[j].Alpha.Op);
        }
        cs.DepthState.Enable = CSHARP_MARSHAL_BOOL(pass->DepthState.Enable);
        cs.DepthState.Write = CSHARP_MARSHAL_BOOL(pass->DepthState.Write);
        cs.DepthState.Compare = static_cast<CSharpInt>(pass->DepthState.Compare);
        cs.StencilState.Enable = CSHARP_MARSHAL_BOOL(pass->StencilState.Enable);
        cs.StencilState.ReadMask = pass->StencilState.ReadMask;
        cs.StencilState.WriteMask = pass->StencilState.WriteMask;
        cs.StencilState.FrontFace.Compare = static_cast<CSharpInt>(pass->StencilState.FrontFace.Compare);
        cs.StencilState.FrontFace.PassOp = static_cast<CSharpInt>(pass->StencilState.FrontFace.PassOp);
        cs.StencilState.FrontFace.FailOp = static_cast<CSharpInt>(pass->StencilState.FrontFace.FailOp);
        cs.StencilState.FrontFace.DepthFailOp = static_cast<CSharpInt>(pass->StencilState.FrontFace.DepthFailOp);
        cs.StencilState.BackFace.Compare = static_cast<CSharpInt>(pass->StencilState.BackFace.Compare);
        cs.StencilState.BackFace.PassOp = static_cast<CSharpInt>(pass->StencilState.BackFace.PassOp);
        cs.StencilState.BackFace.FailOp = static_cast<CSharpInt>(pass->StencilState.BackFace.FailOp);
        cs.StencilState.BackFace.DepthFailOp = static_cast<CSharpInt>(pass->StencilState.BackFace.DepthFailOp);
    }
}

NATIVE_EXPORT(void) Shader_SetPasses(Shader* pShader, CSharpArray passes)
{
    pShader->Version++;
    pShader->Passes.resize(CSharpArray_GetLength<CSharpShaderPass>(passes));

    for (int i = 0; i < pShader->Passes.size(); i++)
    {
        const auto& cs = CSharpArray_Get<CSharpShaderPass>(passes, i);
        pShader->Passes[i] = std::make_unique<ShaderPass>();
        ShaderPass* pass = pShader->Passes[i].get();

        pass->Name = CSharpString_ToUtf8(cs.Name);

        THROW_IF_FAILED(Shader::GetDxcUtils()->CreateBlob(
            &cs.VertexShader->FirstByte, cs.VertexShader->Length, DXC_CP_ACP,
            reinterpret_cast<IDxcBlobEncoding**>(pass->VertexShader.ReleaseAndGetAddressOf())));
        THROW_IF_FAILED(Shader::GetDxcUtils()->CreateBlob(
            &cs.PixelShader->FirstByte, cs.PixelShader->Length, DXC_CP_ACP,
            reinterpret_cast<IDxcBlobEncoding**>(pass->PixelShader.ReleaseAndGetAddressOf())));

        pass->ConstantBuffers.clear();
        for (int j = 0; j < CSharpArray_GetLength<CSharpShaderPassConstantBuffer>(cs.ConstantBuffers); j++)
        {
            const auto& cb = CSharpArray_Get<CSharpShaderPassConstantBuffer>(cs.ConstantBuffers, j);
            pass->ConstantBuffers[CSharpString_ToUtf8(cb.Name)] = { cb.ShaderRegister, cb.RegisterSpace, cb.Size };
        }

        pass->Samplers.clear();
        for (int j = 0; j < CSharpArray_GetLength<CSharpShaderPassSampler>(cs.Samplers); j++)
        {
            const auto& sampler = CSharpArray_Get<CSharpShaderPassSampler>(cs.Samplers, j);
            pass->Samplers[CSharpString_ToUtf8(sampler.Name)] = { sampler.ShaderRegister, sampler.RegisterSpace };
        }

        pass->MaterialProperties.clear();
        for (int j = 0; j < CSharpArray_GetLength<CSharpShaderPassMaterialProperty>(cs.MaterialProperties); j++)
        {
            const auto& mp = CSharpArray_Get<CSharpShaderPassMaterialProperty>(cs.MaterialProperties, j);
            pass->MaterialProperties[CSharpString_ToUtf8(mp.Name)] = { mp.Offset, mp.Size };
        }

        pass->TextureProperties.clear();
        for (int j = 0; j < CSharpArray_GetLength<CSharpShaderPassTextureProperty>(cs.TextureProperties); j++)
        {
            const auto& tp = CSharpArray_Get<CSharpShaderPassTextureProperty>(cs.TextureProperties, j);
            pass->TextureProperties[CSharpString_ToUtf8(tp.Name)] =
            {
                tp.ShaderRegisterTexture,
                tp.RegisterSpaceTexture,
                CSHARP_UNMARSHAL_BOOL(tp.HasSampler),
                tp.ShaderRegisterSampler,
                tp.RegisterSpaceSampler
            };
        }

        pass->Cull = static_cast<ShaderPassCullMode>(cs.Cull);

        pass->Blends.resize(CSharpArray_GetLength<CSharpShaderPassBlendState>(cs.Blends));
        for (int j = 0; j < pass->Blends.size(); j++)
        {
            const auto& blend = CSharpArray_Get<CSharpShaderPassBlendState>(cs.Blends, j);
            pass->Blends[j].Enable = CSHARP_UNMARSHAL_BOOL(blend.Enable);
            pass->Blends[j].WriteMask = static_cast<ShaderPassColorWriteMask>(blend.WriteMask);
            pass->Blends[j].Rgb = { static_cast<ShaderPassBlend>(blend.Rgb.Src), static_cast<ShaderPassBlend>(blend.Rgb.Dest), static_cast<ShaderPassBlendOp>(blend.Rgb.Op) };
            pass->Blends[j].Alpha = { static_cast<ShaderPassBlend>(blend.Alpha.Src), static_cast<ShaderPassBlend>(blend.Alpha.Dest), static_cast<ShaderPassBlendOp>(blend.Alpha.Op) };
        }

        pass->DepthState =
        {
            CSHARP_UNMARSHAL_BOOL(cs.DepthState.Enable),
            CSHARP_UNMARSHAL_BOOL(cs.DepthState.Write),
            static_cast<ShaderPassCompareFunc>(cs.DepthState.Compare)
        };

        pass->StencilState =
        {
            CSHARP_UNMARSHAL_BOOL(cs.StencilState.Enable),
            static_cast<uint8_t>(cs.StencilState.ReadMask),
            static_cast<uint8_t>(cs.StencilState.WriteMask),
            {
                static_cast<ShaderPassCompareFunc>(cs.StencilState.FrontFace.Compare),
                static_cast<ShaderPassStencilOp>(cs.StencilState.FrontFace.PassOp),
                static_cast<ShaderPassStencilOp>(cs.StencilState.FrontFace.FailOp),
                static_cast<ShaderPassStencilOp>(cs.StencilState.FrontFace.DepthFailOp)
            },
            {
                static_cast<ShaderPassCompareFunc>(cs.StencilState.BackFace.Compare),
                static_cast<ShaderPassStencilOp>(cs.StencilState.BackFace.PassOp),
                static_cast<ShaderPassStencilOp>(cs.StencilState.BackFace.FailOp),
                static_cast<ShaderPassStencilOp>(cs.StencilState.BackFace.DepthFailOp)
            }
        };
    }
}

NATIVE_EXPORT(CSharpBool) Shader_CompilePass(Shader* pShader, CSharpInt passIndex, CSharpString filename, CSharpString program, CSharpString entrypoint, CSharpString shaderModel, CSharpInt programType)
{
    return CSHARP_MARSHAL_BOOL(pShader->CompilePass(
        passIndex,
        CSharpString_ToUtf8(filename),
        CSharpString_ToUtf8(program),
        CSharpString_ToUtf8(entrypoint),
        CSharpString_ToUtf8(shaderModel),
        static_cast<ShaderProgramType>(programType)
    ));
}

NATIVE_EXPORT(void) Shader_CreatePassRootSignature(Shader* pShader, CSharpInt passIndex)
{
    pShader->Passes[passIndex]->CreateRootSignature();
}
