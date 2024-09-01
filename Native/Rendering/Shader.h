#pragma once

#include "Scripting/ScriptTypes.h"
#include <directx/d3dx12.h>
#include <d3d12.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <wrl.h>
#include <dxcapi.h>
#include <stdint.h>

namespace dx12demo
{
    struct ShaderPassConstantBuffer
    {
        UINT ShaderRegister;
        UINT RegisterSpace;
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

        void CreatePso();

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

    protected:
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> CreateStaticSamplers();
        void CreateRootSignature();

    private:
        std::unordered_map<std::string, UINT> m_CbRootParamIndexMap;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PsoNormal;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PsoWireframe;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PsoNormalMSAA;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PsoWireframeMSAA;

    private:
        const UINT m_RootSrvDescriptorTableIndex = 0;
        const UINT m_RootSamplerDescriptorTableIndex = 1;

    public:
        static constexpr LPCSTR MaterialCbName = "cbMat";
    };

    class Shader
    {
    public:
        static void Compile(
            const std::string& filename,
            const std::string& entrypoint,
            const std::string& shaderModel,
            ShaderProgramType programType,
            ShaderPass& targetPass);

        std::vector<ShaderPass> Passes;
    };

    namespace binding
    {
        struct CSharpShaderPassConstantBuffer
        {
            CSharpString Name;
            CSharpUInt ShaderRegister;
            CSharpUInt RegisterSpace;
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
            ShaderPassCompareFunc Compare;
            ShaderPassStencilOp PassOp;
            ShaderPassStencilOp FailOp;
            ShaderPassStencilOp DepthFailOp;
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

            CSharpByte* VertexShader;
            CSharpUInt VertexShaderSize;
            CSharpByte* PixelShader;
            CSharpUInt PixelShaderSize;

            CSharpShaderPassConstantBuffer* ConstantBuffers;
            CSharpUInt ConstantBufferSize;
            CSharpShaderPassSampler* Samplers;
            CSharpUInt SamplerSize;
            CSharpShaderPassMaterialProperty* MaterialProperties;
            CSharpUInt MaterialPropertySize;
            CSharpShaderPassTextureProperty* TextureProperties;
            CSharpUInt TexturePropertySize;

            CSharpInt Cull;
            CSharpShaderPassBlendState* Blends;
            CSharpUInt BlendSize;
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

        inline CSHARP_API(CSharpInt) Shader_GetPassCount(Shader* pShader)
        {
            return static_cast<CSharpInt>(pShader->Passes.size());
        }

        inline CSHARP_API(void) Shader_GetPasses(Shader* pShader, CSharpShaderPass* pPasses)
        {
            for (int i = 0; i < pShader->Passes.size(); i++)
            {
                auto& pass = pShader->Passes[i];

                pPasses[i].Name = CSharpString_FromUtf8(pass.Name);
                pPasses[i].VertexShader = reinterpret_cast<CSharpByte*>(pass.VertexShader->GetBufferPointer());
                pPasses[i].VertexShaderSize = static_cast<CSharpUInt>(pass.VertexShader->GetBufferSize());
                pPasses[i].PixelShader = reinterpret_cast<CSharpByte*>(pass.PixelShader->GetBufferPointer());
                pPasses[i].PixelShaderSize = static_cast<CSharpUInt>(pass.PixelShader->GetBufferSize());

                // TODO: new 和 allocHGlobal 不能混用
                pPasses[i].ConstantBuffers = new CSharpShaderPassConstantBuffer[pass.ConstantBuffers.size()];
            }
        }

        inline CSHARP_API(void) Shader_SetPasses(Shader* pShader, CSharpShaderPass* pPasses, CSharpInt passCount)
        {
            pShader->Passes.resize(passCount);

        }
    }
}
