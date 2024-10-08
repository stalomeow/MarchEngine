#pragma once

#include "GfxExcept.h"
#include "GfxTexture.h"
#include <directx/d3dx12.h>
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <wrl.h>
#include <dxcapi.h>
#include <stdint.h>

namespace march
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

        GfxTexture* GetDefaultTexture() const
        {
            switch (DefaultTexture)
            {
            case march::ShaderDefaultTexture::Black:
                return GfxTexture::GetDefaultBlack();

            case march::ShaderDefaultTexture::White:
                return GfxTexture::GetDefaultWhite();

            default:
                return nullptr;
            }
        }
    };

    struct ShaderPassConstantBuffer
    {
        UINT ShaderRegister;
        UINT RegisterSpace;
        UINT Size; // unaligned

        UINT DescriptorTableIndex;
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
        UINT ShaderRegisterTexture;
        UINT RegisterSpaceTexture;

        bool HasSampler;
        UINT ShaderRegisterSampler;
        UINT RegisterSpaceSampler;

        UINT TextureDescriptorTableIndex;
        UINT SamplerDescriptorTableIndex;
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
        std::unordered_map<std::string, ShaderPassTextureProperty> TextureProperties;

        ShaderPassCullMode Cull;
        std::vector<ShaderPassBlendState> Blends;
        ShaderPassDepthState DepthState;
        ShaderPassStencilState StencilState;

        UINT GetCbvSrvUavCount() const { return m_CbvSrvUavCount; }
        UINT GetCbvSrvUavRootParamIndex() const { return m_CbvSrvUavRootParamIndex; }
        UINT GetSamplerCount() const { return m_SamplerCount; }
        UINT GetSamplerRootParamIndex() const { return m_SamplerRootParamIndex; }

        ID3D12RootSignature* GetRootSignature() const { return m_RootSignature.Get(); }

        void CreateRootSignature();

    private:
        std::vector<CD3DX12_STATIC_SAMPLER_DESC> CreateStaticSamplers();

        UINT m_CbvSrvUavCount = 0;
        UINT m_CbvSrvUavRootParamIndex = 0;

        UINT m_SamplerCount = 0;
        UINT m_SamplerRootParamIndex = 0;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

    public:
        static constexpr LPCSTR MaterialCbName = "cbMaterial";
    };

    class Shader
    {
    public:
        bool CompilePass(int passIndex,
            const std::string& filename,
            const std::string& program,
            const std::string& entrypoint,
            const std::string& shaderModel,
            ShaderProgramType programType);

        std::unordered_map<std::string, ShaderProperty> Properties;
        std::vector<std::unique_ptr<ShaderPass>> Passes;
        int32_t Version = 0;

        static std::string GetEngineShaderPathUnixStyle();

        inline static IDxcUtils* GetDxcUtils()
        {
            if (s_Utils == nullptr)
            {
                GFX_HR(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_Utils)));
            }

            return s_Utils.Get();
        }

        inline static IDxcCompiler3* GetDxcCompiler()
        {
            if (s_Compiler == nullptr)
            {
                GFX_HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_Compiler)));
            }

            return s_Compiler.Get();
        }

    private:
        inline static Microsoft::WRL::ComPtr<IDxcUtils> s_Utils = nullptr;
        inline static Microsoft::WRL::ComPtr<IDxcCompiler3> s_Compiler = nullptr;
    };
}
