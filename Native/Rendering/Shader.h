#pragma once

#include <d3d12.h>
#include <vector>
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

    struct ShaderPassMaterialProperty
    {
        std::string Name;
        ShaderPropertyType Type;
        int Offset;
    };

    struct ShaderPassTextureProperty
    {
        std::string Name;
        int ShaderRegisterTexture;

        bool HasSampler;
        int ShaderRegisterSampler;
    };

    struct ShaderPassConstantBuffer
    {
        std::string Name;
        int ShaderRegister;
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

    struct ShaderPass
    {
        std::string Name;

        Microsoft::WRL::ComPtr<IDxcBlob> VertexShader;
        Microsoft::WRL::ComPtr<IDxcBlob> PixelShader;

        std::vector<ShaderPassConstantBuffer> ConstantBuffers;
        std::vector<ShaderPassMaterialProperty> MaterialProperties;
        std::vector<ShaderPassTextureProperty> TextureProperties;

        ShaderPassCullMode Cull;
        std::vector<ShaderPassBlendState> Blends;
        ShaderPassDepthState DepthState;
        ShaderPassStencilState StencilState;
    };

    struct ShaderCompileResult
    {

    };

    class Shader
    {
    public:
        static void Compile(const std::string& filename, const std::string& entrypoint, const std::string& targetProfile);

    private:
        std::vector<ShaderPass> m_Passes;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PsoNormal;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PsoWireframe;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PsoNormalMSAA;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PsoWireframeMSAA;
    };
}
