#pragma once

#include "Scripting/ScriptTypes.h"
#include "Rendering/DxException.h"
#include "Rendering/Resource/Texture.h"
#include <directx/d3dx12.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <wrl.h>
#include <dxcapi.h>
#include <stdint.h>

namespace dx12demo
{
    enum class ShaderPropertyType
    {
        Float = 0,
        Int = 1,
        Color = 2,
        Vector = 3,
        Texture = 4,
    };

    enum class ShaderDefaultTexture
    {
        Black = 0,
        White = 1
    };

    struct ShaderProperty
    {
        ShaderPropertyType Type;

        float DefaultFloat;
        int32_t DefaultInt;
        DirectX::XMFLOAT4 DefaultColor;
        DirectX::XMFLOAT4 DefaultVector;
        ShaderDefaultTexture DefaultTexture;

        Texture* GetDefaultTexture() const
        {
            switch (DefaultTexture)
            {
            case dx12demo::ShaderDefaultTexture::Black:
                return Texture::GetDefaultBlack();

            case dx12demo::ShaderDefaultTexture::White:
                return Texture::GetDefaultWhite();

            default:
                return nullptr;
            }
        }
    };

    struct ShaderPassConstantBuffer
    {
        UINT ShaderRegister;
        UINT RegisterSpace;
        UINT Size;
    };

    struct ShaderPassSampler
    {
        UINT ShaderRegister;
        UINT RegisterSpace;
    };

    struct ShaderPassMaterialProperty
    {
        UINT Offset;
        UINT Size;
    };

    struct ShaderPassTextureProperty
    {
        std::string Name;

        UINT ShaderRegisterTexture;
        UINT RegisterSpaceTexture;

        bool HasSampler;
        UINT ShaderRegisterSampler;
        UINT RegisterSpaceSampler;
    };

    enum class ShaderPassCullMode
    {
        Off = 0,
        Front = 1,
        Back = 2,
    };

    enum class ShaderPassBlend
    {
        Zero = 0,
        One = 1,
        SrcColor = 2,
        InvSrcColor = 3,
        SrcAlpha = 4,
        InvSrcAlpha = 5,
        DestAlpha = 6,
        InvDestAlpha = 7,
        DestColor = 8,
        InvDestColor = 9,
        SrcAlphaSat = 10,
    };

    enum class ShaderPassBlendOp
    {
        Add = 0,
        Subtract = 1,
        RevSubtract = 2,
        Min = 3,
        Max = 4,
    };

    enum class ShaderPassColorWriteMask
    {
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        All = Red | Green | Blue | Alpha
    };

    struct ShaderPassBlendFormula
    {
        ShaderPassBlend Src;
        ShaderPassBlend Dest;
        ShaderPassBlendOp Op;
    };

    struct ShaderPassBlendState
    {
        bool Enable;
        ShaderPassColorWriteMask WriteMask;
        ShaderPassBlendFormula Rgb;
        ShaderPassBlendFormula Alpha;
    };

    enum class ShaderPassCompareFunc
    {
        Never = 0,
        Less = 1,
        Equal = 2,
        LessEqual = 3,
        Greater = 4,
        NotEqual = 5,
        GreaterEqual = 6,
        Always = 7,
    };

    struct ShaderPassDepthState
    {
        bool Enable;
        bool Write;
        ShaderPassCompareFunc Compare;
    };

    enum class ShaderPassStencilOp
    {
        Keep = 0,
        Zero = 1,
        Replace = 2,
        IncrSat = 3,
        DecrSat = 4,
        Invert = 5,
        Incr = 6,
        Decr = 7,
    };

    struct ShaderPassStencilAction
    {
        ShaderPassCompareFunc Compare;
        ShaderPassStencilOp PassOp;
        ShaderPassStencilOp FailOp;
        ShaderPassStencilOp DepthFailOp;
    };

    struct ShaderPassStencilState
    {
        bool Enable;
        uint8_t ReadMask;
        uint8_t WriteMask;
        ShaderPassStencilAction FrontFace;
        ShaderPassStencilAction BackFace;
    };

    enum class ShaderProgramType
    {
        Vertex = 0,
        Pixel = 1,
    };

    class ShaderPass
    {
    public:
        std::string Name;

        Microsoft::WRL::ComPtr<IDxcBlob> VertexShader;
        Microsoft::WRL::ComPtr<IDxcBlob> PixelShader;

        std::unordered_map<std::string, ShaderPassConstantBuffer> ConstantBuffers;
        std::unordered_map<std::string, ShaderPassSampler> Samplers;
        std::unordered_map<std::string, ShaderPassMaterialProperty> MaterialProperties;
        std::vector<ShaderPassTextureProperty> TextureProperties; // 保证顺序

        ShaderPassCullMode Cull;
        std::vector<ShaderPassBlendState> Blends;
        ShaderPassDepthState DepthState;
        ShaderPassStencilState StencilState;

        UINT GetRootSrvDescriptorTableIndex() const { return m_RootSrvDescriptorTableIndex; }
        UINT GetRootSamplerDescriptorTableIndex() const { return m_RootSamplerDescriptorTableIndex; }

        bool TryGetRootCbvIndex(const std::string& name, UINT* outIndex) const
        {
            auto it = m_CbRootParamIndexMap.find(name);

            if (it == m_CbRootParamIndexMap.end())
            {
                return false;
            }

            *outIndex = it->second;
            return true;
        }

        ID3D12RootSignature* GetRootSignature() const { return m_RootSignature.Get(); }

        void CreateRootSignature();

    private:
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> CreateStaticSamplers();

        std::unordered_map<std::string, UINT> m_CbRootParamIndexMap;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

        const UINT m_RootSrvDescriptorTableIndex = 0;
        const UINT m_RootSamplerDescriptorTableIndex = 1;

    public:
        static constexpr LPCSTR MaterialCbName = "cbMaterial";
    };

    class Shader
    {
    public:
        void CompilePass(int passIndex,
            const std::string& filename,
            const std::string& entrypoint,
            const std::string& shaderModel,
            ShaderProgramType programType);

        std::unordered_map<std::string, ShaderProperty> Properties;
        std::vector<ShaderPass> Passes;

        inline static IDxcUtils* GetDxcUtils()
        {
            if (s_Utils == nullptr)
            {
                THROW_IF_FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_Utils)));
            }

            return s_Utils.Get();
        }

        inline static IDxcCompiler3* GetDxcCompiler()
        {
            if (s_Compiler == nullptr)
            {
                THROW_IF_FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_Compiler)));
            }

            return s_Compiler.Get();
        }

    private:
        inline static Microsoft::WRL::ComPtr<IDxcUtils> s_Utils = nullptr;
        inline static Microsoft::WRL::ComPtr<IDxcCompiler3> s_Compiler = nullptr;
    };

    namespace binding
    {
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

        inline CSHARP_API(Shader*) Shader_New()
        {
            return new Shader();
        }

        inline CSHARP_API(void) Shader_Delete(Shader* pShader)
        {
            delete pShader;
        }

        inline CSHARP_API(void) Shader_ClearProperties(Shader* pShader)
        {
            pShader->Properties.clear();
        }

        inline CSHARP_API(void) Shader_SetProperty(Shader* pShader, CSharpShaderProperty* prop)
        {
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

        inline CSHARP_API(CSharpInt) Shader_GetPassCount(Shader* pShader)
        {
            return static_cast<CSharpInt>(pShader->Passes.size());
        }

        inline CSHARP_API(void) Shader_GetPasses(Shader* pShader, CSharpArray passes)
        {
            for (int i = 0; i < pShader->Passes.size(); i++)
            {
                auto& pass = pShader->Passes[i];
                auto& cs = CSharpArray_Get<CSharpShaderPass>(passes, i);

                cs.Name = CSharpString_FromUtf8(pass.Name);

                cs.VertexShader = CSharpArray_New<CSharpByte>(pass.VertexShader->GetBufferSize());
                CSharpArray_CopyFrom(cs.VertexShader, pass.VertexShader->GetBufferPointer());

                cs.PixelShader = CSharpArray_New<CSharpByte>(pass.PixelShader->GetBufferSize());
                CSharpArray_CopyFrom(cs.PixelShader, pass.PixelShader->GetBufferPointer());

                cs.ConstantBuffers = CSharpArray_New<CSharpShaderPassConstantBuffer>(pass.ConstantBuffers.size());
                int cbIndex = 0;
                for (auto& kvp : pass.ConstantBuffers)
                {
                    auto& cb = CSharpArray_Get<CSharpShaderPassConstantBuffer>(cs.ConstantBuffers, cbIndex++);
                    cb.Name = CSharpString_FromUtf8(kvp.first);
                    cb.ShaderRegister = kvp.second.ShaderRegister;
                    cb.RegisterSpace = kvp.second.RegisterSpace;
                    cb.Size = kvp.second.Size;
                }

                cs.Samplers = CSharpArray_New<CSharpShaderPassSampler>(pass.Samplers.size());
                int samplerIndex = 0;
                for (auto& kvp : pass.Samplers)
                {
                    auto& sampler = CSharpArray_Get<CSharpShaderPassSampler>(cs.Samplers, samplerIndex++);
                    sampler.Name = CSharpString_FromUtf8(kvp.first);
                    sampler.ShaderRegister = kvp.second.ShaderRegister;
                    sampler.RegisterSpace = kvp.second.RegisterSpace;
                }

                cs.MaterialProperties = CSharpArray_New<CSharpShaderPassMaterialProperty>(pass.MaterialProperties.size());
                int mpIndex = 0;
                for (auto& kvp : pass.MaterialProperties)
                {
                    auto& mp = CSharpArray_Get<CSharpShaderPassMaterialProperty>(cs.MaterialProperties, mpIndex++);
                    mp.Name = CSharpString_FromUtf8(kvp.first);
                    mp.Offset = kvp.second.Offset;
                    mp.Size = kvp.second.Size;
                }

                cs.TextureProperties = CSharpArray_New<CSharpShaderPassTextureProperty>(pass.TextureProperties.size());
                for (int i = 0; i < pass.TextureProperties.size(); i++)
                {
                    auto& tp = CSharpArray_Get<CSharpShaderPassTextureProperty>(cs.TextureProperties, i);
                    tp.Name = CSharpString_FromUtf8(pass.TextureProperties[i].Name);
                    tp.ShaderRegisterTexture = pass.TextureProperties[i].ShaderRegisterTexture;
                    tp.RegisterSpaceTexture = pass.TextureProperties[i].RegisterSpaceTexture;
                    tp.HasSampler = CSHARP_MARSHAL_BOOL(pass.TextureProperties[i].HasSampler);
                    tp.ShaderRegisterSampler = pass.TextureProperties[i].ShaderRegisterSampler;
                    tp.RegisterSpaceSampler = pass.TextureProperties[i].RegisterSpaceSampler;
                }

                cs.Cull = static_cast<CSharpInt>(pass.Cull);
                cs.Blends = CSharpArray_New<CSharpShaderPassBlendState>(pass.Blends.size());
                for (int i = 0; i < pass.Blends.size(); i++)
                {
                    auto& blend = CSharpArray_Get<CSharpShaderPassBlendState>(cs.Blends, i);
                    blend.Enable = CSHARP_MARSHAL_BOOL(pass.Blends[i].Enable);
                    blend.WriteMask = static_cast<CSharpInt>(pass.Blends[i].WriteMask);
                    blend.Rgb.Src = static_cast<CSharpInt>(pass.Blends[i].Rgb.Src);
                    blend.Rgb.Dest = static_cast<CSharpInt>(pass.Blends[i].Rgb.Dest);
                    blend.Rgb.Op = static_cast<CSharpInt>(pass.Blends[i].Rgb.Op);
                    blend.Alpha.Src = static_cast<CSharpInt>(pass.Blends[i].Alpha.Src);
                    blend.Alpha.Dest = static_cast<CSharpInt>(pass.Blends[i].Alpha.Dest);
                    blend.Alpha.Op = static_cast<CSharpInt>(pass.Blends[i].Alpha.Op);
                }
                cs.DepthState.Enable = CSHARP_MARSHAL_BOOL(pass.DepthState.Enable);
                cs.DepthState.Write = CSHARP_MARSHAL_BOOL(pass.DepthState.Write);
                cs.DepthState.Compare = static_cast<CSharpInt>(pass.DepthState.Compare);
                cs.StencilState.Enable = CSHARP_MARSHAL_BOOL(pass.StencilState.Enable);
                cs.StencilState.ReadMask = pass.StencilState.ReadMask;
                cs.StencilState.WriteMask = pass.StencilState.WriteMask;
                cs.StencilState.FrontFace.Compare = static_cast<CSharpInt>(pass.StencilState.FrontFace.Compare);
                cs.StencilState.FrontFace.PassOp = static_cast<CSharpInt>(pass.StencilState.FrontFace.PassOp);
                cs.StencilState.FrontFace.FailOp = static_cast<CSharpInt>(pass.StencilState.FrontFace.FailOp);
                cs.StencilState.FrontFace.DepthFailOp = static_cast<CSharpInt>(pass.StencilState.FrontFace.DepthFailOp);
                cs.StencilState.BackFace.Compare = static_cast<CSharpInt>(pass.StencilState.BackFace.Compare);
                cs.StencilState.BackFace.PassOp = static_cast<CSharpInt>(pass.StencilState.BackFace.PassOp);
                cs.StencilState.BackFace.FailOp = static_cast<CSharpInt>(pass.StencilState.BackFace.FailOp);
                cs.StencilState.BackFace.DepthFailOp = static_cast<CSharpInt>(pass.StencilState.BackFace.DepthFailOp);
            }
        }

        inline CSHARP_API(void) Shader_SetPasses(Shader* pShader, CSharpArray passes)
        {
            pShader->Passes.resize(CSharpArray_GetLength<CSharpShaderPass>(passes));

            for (int i = 0; i < pShader->Passes.size(); i++)
            {
                const auto& cs = CSharpArray_Get<CSharpShaderPass>(passes, i);
                auto& pass = pShader->Passes[i];

                pass.Name = CSharpString_ToUtf8(cs.Name);

                THROW_IF_FAILED(Shader::GetDxcUtils()->CreateBlob(
                    &cs.VertexShader->FirstByte, cs.VertexShader->Length, DXC_CP_ACP,
                    reinterpret_cast<IDxcBlobEncoding**>(pass.VertexShader.ReleaseAndGetAddressOf())));
                THROW_IF_FAILED(Shader::GetDxcUtils()->CreateBlob(
                    &cs.PixelShader->FirstByte, cs.PixelShader->Length, DXC_CP_ACP,
                    reinterpret_cast<IDxcBlobEncoding**>(pass.PixelShader.ReleaseAndGetAddressOf())));

                pass.ConstantBuffers.clear();
                for (int j = 0; j < CSharpArray_GetLength<CSharpShaderPassConstantBuffer>(cs.ConstantBuffers); j++)
                {
                    const auto& cb = CSharpArray_Get<CSharpShaderPassConstantBuffer>(cs.ConstantBuffers, j);
                    pass.ConstantBuffers[CSharpString_ToUtf8(cb.Name)] = { cb.ShaderRegister, cb.RegisterSpace, cb.Size };
                }

                pass.Samplers.clear();
                for (int j = 0; j < CSharpArray_GetLength<CSharpShaderPassSampler>(cs.Samplers); j++)
                {
                    const auto& sampler = CSharpArray_Get<CSharpShaderPassSampler>(cs.Samplers, j);
                    pass.Samplers[CSharpString_ToUtf8(sampler.Name)] = { sampler.ShaderRegister, sampler.RegisterSpace };
                }

                pass.MaterialProperties.clear();
                for (int j = 0; j < CSharpArray_GetLength<CSharpShaderPassMaterialProperty>(cs.MaterialProperties); j++)
                {
                    const auto& mp = CSharpArray_Get<CSharpShaderPassMaterialProperty>(cs.MaterialProperties, j);
                    pass.MaterialProperties[CSharpString_ToUtf8(mp.Name)] = { mp.Offset, mp.Size };
                }

                pass.TextureProperties.resize(CSharpArray_GetLength<CSharpShaderPassTextureProperty>(cs.TextureProperties));
                for (int j = 0; j < pass.TextureProperties.size(); j++)
                {
                    const auto& tp = CSharpArray_Get<CSharpShaderPassTextureProperty>(cs.TextureProperties, j);
                    pass.TextureProperties[j].Name = CSharpString_ToUtf8(tp.Name);
                    pass.TextureProperties[j].ShaderRegisterTexture = tp.ShaderRegisterTexture;
                    pass.TextureProperties[j].RegisterSpaceTexture = tp.RegisterSpaceTexture;
                    pass.TextureProperties[j].HasSampler = CSHARP_UNMARSHAL_BOOL(tp.HasSampler);
                    pass.TextureProperties[j].ShaderRegisterSampler = tp.ShaderRegisterSampler;
                    pass.TextureProperties[j].RegisterSpaceSampler = tp.RegisterSpaceSampler;
                }

                pass.Cull = static_cast<ShaderPassCullMode>(cs.Cull);

                pass.Blends.resize(CSharpArray_GetLength<CSharpShaderPassBlendState>(cs.Blends));
                for (int j = 0; j < pass.Blends.size(); j++)
                {
                    const auto& blend = CSharpArray_Get<CSharpShaderPassBlendState>(cs.Blends, j);
                    pass.Blends[j].Enable = CSHARP_UNMARSHAL_BOOL(blend.Enable);
                    pass.Blends[j].WriteMask = static_cast<ShaderPassColorWriteMask>(blend.WriteMask);
                    pass.Blends[j].Rgb = { static_cast<ShaderPassBlend>(blend.Rgb.Src), static_cast<ShaderPassBlend>(blend.Rgb.Dest), static_cast<ShaderPassBlendOp>(blend.Rgb.Op) };
                    pass.Blends[j].Alpha = { static_cast<ShaderPassBlend>(blend.Alpha.Src), static_cast<ShaderPassBlend>(blend.Alpha.Dest), static_cast<ShaderPassBlendOp>(blend.Alpha.Op) };
                }

                pass.DepthState =
                {
                    CSHARP_UNMARSHAL_BOOL(cs.DepthState.Enable),
                    CSHARP_UNMARSHAL_BOOL(cs.DepthState.Write),
                    static_cast<ShaderPassCompareFunc>(cs.DepthState.Compare)
                };

                pass.StencilState =
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

        inline CSHARP_API(void) Shader_CompilePass(Shader* pShader, CSharpInt passIndex, CSharpString filename, CSharpString entrypoint, CSharpString shaderModel, CSharpInt programType)
        {
            pShader->CompilePass(
                passIndex,
                CSharpString_ToUtf8(filename),
                CSharpString_ToUtf8(entrypoint),
                CSharpString_ToUtf8(shaderModel),
                static_cast<ShaderProgramType>(programType)
            );
        }

        inline CSHARP_API(void) Shader_CreatePassRootSignature(Shader* pShader, CSharpInt passIndex)
        {
            pShader->Passes[passIndex].CreateRootSignature();
        }
    }
}
