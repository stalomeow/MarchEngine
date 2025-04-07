#pragma once

#include "Engine/Ints.h"
#include "Engine/Object.h"
#include "Engine/Rendering/D3D12Impl/ShaderKeyword.h"
#include "Engine/Rendering/D3D12Impl/ShaderProgram.h"
#include <directx/d3dx12.h>
#include <DirectXMath.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <optional>

namespace march
{
    enum class ShaderProgramType
    {
        Vertex,
        Pixel,
        Domain,
        Hull,
        Geometry,
    };

    enum class CullMode
    {
        Off = 0,
        Front = 1,
        Back = 2,
    };

    enum class BlendMode
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

    enum class BlendOp
    {
        Add = 0,
        Subtract = 1,
        RevSubtract = 2,
        Min = 3,
        Max = 4,
    };

    enum class ColorWriteMask
    {
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        All = Red | Green | Blue | Alpha
    };

    enum class CompareFunction
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

    enum class StencilOp
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

    enum class ShaderPropertyType
    {
        Float = 0,
        Int = 1,
        Color = 2,
        Vector = 3,
        Texture = 4,
    };

    enum class GfxTextureDimension;
    enum class GfxDefaultTexture;

    struct ShaderProperty
    {
        ShaderPropertyType Type;

        union
        {
            float DefaultFloat;
            int32_t DefaultInt;
            DirectX::XMFLOAT4 DefaultColor;
            DirectX::XMFLOAT4 DefaultVector;

            struct
            {
                GfxTextureDimension TextureDimension;
                GfxDefaultTexture DefaultTexture;
            };
        };

        class GfxTexture* GetDefaultTexture() const;
    };

    struct ShaderPropertyLocation
    {
        uint32_t Offset;
        uint32_t Size;
    };

    template <typename T>
    struct ShaderPassVar
    {
        bool IsDynamic;

        union
        {
            int32_t PropertyId;
            T Value;
        };
    };

    struct ShaderPassBlendFormula
    {
        ShaderPassVar<BlendMode> Src;
        ShaderPassVar<BlendMode> Dest;
        ShaderPassVar<BlendOp> Op;
    };

    struct ShaderPassBlendState
    {
        bool Enable;
        ShaderPassVar<ColorWriteMask> WriteMask;
        ShaderPassBlendFormula Rgb;
        ShaderPassBlendFormula Alpha;
    };

    struct ShaderPassDepthState
    {
        bool Enable;
        ShaderPassVar<bool> Write;
        ShaderPassVar<CompareFunction> Compare;
    };

    struct ShaderPassStencilAction
    {
        ShaderPassVar<CompareFunction> Compare;
        ShaderPassVar<StencilOp> PassOp;
        ShaderPassVar<StencilOp> FailOp;
        ShaderPassVar<StencilOp> DepthFailOp;
    };

    struct ShaderPassStencilState
    {
        bool Enable;
        ShaderPassVar<uint8_t> Ref;
        ShaderPassVar<uint8_t> ReadMask;
        ShaderPassVar<uint8_t> WriteMask;
        ShaderPassStencilAction FrontFace;
        ShaderPassStencilAction BackFace;
    };

    struct ShaderPassRenderState
    {
        ShaderPassVar<CullMode> Cull;
        std::vector<ShaderPassBlendState> Blends; // 如果长度大于 1 则使用 Independent Blend
        ShaderPassDepthState DepthState;
        ShaderPassStencilState StencilState;
    };

    class ShaderPass final : public ShaderProgramGroup<5>
    {
        friend class Shader;
        friend class Material;
        friend struct ShaderBinding;

        std::unordered_map<std::string, std::string> m_Tags{};
        ShaderPassRenderState m_RenderState{};

    public:
        const std::unordered_map<std::string, std::string>& GetTags() const { return m_Tags; }
        const ShaderPassRenderState& GetRenderState() const { return m_RenderState; }

    protected:
        D3D12_SHADER_VISIBILITY GetShaderVisibility(size_t programType) override;
        bool GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType) override;
        std::string GetTargetProfile(const std::string& shaderModel, size_t programType) override;
        std::string GetProgramTypePreprocessorMacro(size_t programType) override;
        void RecordEntrypointCallback(size_t programType, std::string& entrypoint) override {}
    };

    class Shader : public MarchObject
    {
        friend struct ShaderBinding;

    public:
        static constexpr size_t NumProgramTypes = ShaderPass::NumProgramTypes;
        using RootSignatureType = ShaderPass::RootSignatureType;

    private:
        uint32_t m_Version = 0;

        std::string m_Name{};
        std::unique_ptr<ShaderKeywordSpace> m_KeywordSpace = std::make_unique<ShaderKeywordSpace>();

        std::unordered_map<int32_t, ShaderProperty> m_Properties{};
        std::unordered_map<int32_t, ShaderPropertyLocation> m_PropertyLocations{}; // shader property 在 cbuffer 中的位置
        uint32_t m_MaterialConstantBufferSize = 0;

        std::vector<std::unique_ptr<ShaderPass>> m_Passes{};

        bool CompilePass(size_t passIndex, const std::string& filename, const std::string& source, const std::vector<std::string>& pragmas, std::vector<std::string>& warnings, std::string& error);

    public:
        uint32_t GetVersion() const { return m_Version; }

        const std::string& GetName() const { return m_Name; }
        const ShaderKeywordSpace* GetKeywordSpace() const { return m_KeywordSpace.get(); }

        const std::unordered_map<int32_t, ShaderProperty>& GetProperties() const { return m_Properties; }
        const std::unordered_map<int32_t, ShaderPropertyLocation>& GetPropertyLocations() const { return m_PropertyLocations; }
        uint32_t GetMaterialConstantBufferSize() const { return m_MaterialConstantBufferSize; }

        ShaderPass* GetPass(size_t index) const { return m_Passes[index].get(); }
        size_t GetPassCount() const { return m_Passes.size(); }

        std::optional<size_t> GetFirstPassIndexWithTagValue(const std::string& tag, const std::string& value) const;
        ShaderPass* GetFirstPassWithTagValue(const std::string& tag, const std::string& value) const;

        static int32 GetMaterialConstantBufferId();
    };
}
