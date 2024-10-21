#pragma once

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
    class GfxTexture;
    class Shader;
    class ShaderPass;
    class ShaderBinding;

    struct ShaderConstantBuffer
    {
        uint32_t ShaderRegister;
        uint32_t RegisterSpace;
        uint32_t UnalignedSize;
        uint32_t RootParameterIndex; // 始终用 RootConstantBufferView
    };

    struct ShaderStaticSampler
    {
        uint32_t ShaderRegister;
        uint32_t RegisterSpace;
    };

    struct ShaderTexture
    {
        uint32_t ShaderRegisterTexture;
        uint32_t RegisterSpaceTexture;

        bool HasSampler;
        uint32_t ShaderRegisterSampler;
        uint32_t RegisterSpaceSampler;

        uint32_t TextureDescriptorTableIndex;
        uint32_t SamplerDescriptorTableIndex;
    };

    enum class ShaderProgramType
    {
        Vertex,
        Pixel,
        NumTypes,
    };

    class ShaderProgram
    {
        friend Shader;
        friend ShaderPass;
        friend ShaderBinding;

    public:
        ShaderProgram() = default;
        ~ShaderProgram() = default;

        uint8_t* GetBinaryData() const;
        uint64_t GetBinarySize() const;

        const std::unordered_map<int32_t, ShaderConstantBuffer>& GetConstantBuffers() const;
        const std::unordered_map<int32_t, ShaderStaticSampler>& GetStaticSamplers() const;
        const std::unordered_map<int32_t, ShaderTexture>& GetTextures() const;

        uint32_t GetSrvUavRootParameterIndex() const;
        uint32_t GetSamplerRootParameterIndex() const;

    private:
        Microsoft::WRL::ComPtr<IDxcBlob> m_Binary;
        std::unordered_map<int32_t, ShaderConstantBuffer> m_ConstantBuffers;
        std::unordered_map<int32_t, ShaderStaticSampler> m_StaticSamplers;
        std::unordered_map<int32_t, ShaderTexture> m_Textures;

        uint32_t m_SrvUavRootParameterIndex = 0;
        uint32_t m_SamplerRootParameterIndex = 0;
    };

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

        union
        {
            float Float;
            int32_t Int;
            DirectX::XMFLOAT4 Color;
            DirectX::XMFLOAT4 Vector;
            ShaderDefaultTexture Texture;
        } DefaultValue;

        GfxTexture* GetDefaultTexture() const;
    };

    struct ShaderPropertyLocation
    {
        uint32_t Offset;
        uint32_t Size;
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
        uint8_t Ref;
        bool Enable;
        uint8_t ReadMask;
        uint8_t WriteMask;
        ShaderPassStencilAction FrontFace;
        ShaderPassStencilAction BackFace;
    };

    class ShaderPass
    {
        friend Shader;
        friend ShaderBinding;

    public:
        ShaderPass() = default;
        ~ShaderPass() = default;

        const std::string& GetName() const;
        const std::unordered_map<int32_t, ShaderPropertyLocation>& GetPropertyLocations() const;
        ShaderProgram* GetProgram(ShaderProgramType type) const;
        ShaderProgram* CreateProgram(ShaderProgramType type);

        const ShaderPassCullMode& GetCull() const;
        const std::vector<ShaderPassBlendState>& GetBlends() const;
        const ShaderPassDepthState& GetDepthState() const;
        const ShaderPassStencilState& GetStencilState() const;

        ID3D12RootSignature* GetRootSignature() const;
        void CreateRootSignature();

    private:
        static D3D12_SHADER_VISIBILITY ToShaderVisibility(ShaderProgramType type);
        void AddStaticSamplers(std::vector<CD3DX12_STATIC_SAMPLER_DESC>& samplers, ShaderProgram* program, D3D12_SHADER_VISIBILITY visibility);

        std::string m_Name;
        std::unordered_map<int32_t, ShaderPropertyLocation> m_PropertyLocations; // shader property 在 cbuffer 中的位置
        std::unique_ptr<ShaderProgram> m_Programs[static_cast<int32_t>(ShaderProgramType::NumTypes)];

        ShaderPassCullMode m_Cull{};
        std::vector<ShaderPassBlendState> m_Blends;
        ShaderPassDepthState m_DepthState{};
        ShaderPassStencilState m_StencilState{};

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
    };

    class Shader
    {
        friend ShaderBinding;

    public:
        const std::unordered_map<int32_t, ShaderProperty>& GetProperties() const;
        ShaderPass* GetPass(int32_t index) const;
        int32_t GetPassCount() const;
        int32_t GetVersion() const;

        static std::string GetEngineShaderPathUnixStyle();

        static IDxcUtils* GetDxcUtils();
        static IDxcCompiler3* GetDxcCompiler();

        static int32_t GetNameId(const std::string& name);
        static const std::string& GetIdName(int32_t id);

        static int32_t GetMaterialConstantBufferId();

        bool CompilePass(int32_t passIndex,
            const std::string& filename,
            const std::string& program,
            const std::string& entrypoint,
            const std::string& shaderModel,
            ShaderProgramType programType);

    private:
        std::unordered_map<int32_t, ShaderProperty> m_Properties;
        std::vector<std::unique_ptr<ShaderPass>> m_Passes;
        int32_t m_Version = 0;

        static Microsoft::WRL::ComPtr<IDxcUtils> s_Utils;
        static Microsoft::WRL::ComPtr<IDxcCompiler3> s_Compiler;

        static std::unordered_map<std::string, int32_t> s_NameIdMap;
        static int32_t s_NextNameId;

        static int32_t s_MaterialConstantBufferId;
    };
}
