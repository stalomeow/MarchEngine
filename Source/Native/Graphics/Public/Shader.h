#pragma once

#include <directx/d3dx12.h>
#include <DirectXMath.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <wrl.h>
#include <dxcapi.h>
#include <stdint.h>
#include <memory>
#include <bitset>
#include <optional>

namespace march
{
    class GfxTexture;
    class Shader;
    class ShaderPass;
    class ShaderBinding;
    class ShaderKeywordSet;
    class ShaderKeywordSpace;

    enum class GfxDefaultTexture;
    enum class GfxTextureDimension;

    class ShaderKeywordSet
    {
    public:
        using data_t = std::bitset<128>;

        ShaderKeywordSet();

        size_t GetEnabledKeywordCount() const;
        size_t GetMatchingKeywordCount(const ShaderKeywordSet& other) const;
        std::vector<std::string> GetEnabledKeywords(const ShaderKeywordSpace& space) const;
        const data_t& GetData() const { return m_Keywords; }

        void SetKeyword(const ShaderKeywordSpace& space, const std::string& keyword, bool value);
        void EnableKeyword(const ShaderKeywordSpace& space, const std::string& keyword);
        void DisableKeyword(const ShaderKeywordSpace& space, const std::string& keyword);
        void Clear();

    private:
        data_t m_Keywords;
    };

    class ShaderKeywordSpace
    {
    public:
        ShaderKeywordSpace();

        ShaderKeywordSpace(const ShaderKeywordSpace&) = delete;
        ShaderKeywordSpace& operator =(const ShaderKeywordSpace&) = delete;

        enum class AddKeywordResult
        {
            Success = 0,
            AlreadyExists = 1,
            OutOfSpace = 2,
        };

        size_t GetKeywordCount() const;
        int8_t GetKeywordIndex(const std::string& keyword) const;
        const std::string& GetKeywordName(int8_t index) const;
        AddKeywordResult AddKeyword(const std::string& keyword);
        void Clear();

    private:
        std::unordered_map<std::string, uint8_t> m_KeywordIndexMap;
        uint8_t m_NextIndex; // 目前最多支持 128 个 Keyword
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

    struct ShaderTexture
    {
        int32_t Id;
        uint32_t ShaderRegisterTexture;
        uint32_t RegisterSpaceTexture;

        bool HasSampler;
        uint32_t ShaderRegisterSampler;
        uint32_t RegisterSpaceSampler;
    };

    struct ShaderStaticSampler
    {
        uint32_t ShaderRegister;
        uint32_t RegisterSpace;
    };

    struct ShaderBuffer
    {
        int32_t Id;
        uint32_t ShaderRegister;
        uint32_t RegisterSpace;
        uint32_t ConstantBufferSize; // 只有 Constant Buffer 有值，其他为 0
    };

    enum class ShaderProgramType
    {
        Vertex,
        Pixel,
        Domain,
        Hull,
        Geometry,
        NumTypes,
    };

    class ShaderProgram
    {
        friend Shader;
        friend ShaderPass;
        friend ShaderBinding;

    public:
        using hash_t = uint8_t[16];
        static constexpr int32_t NumTypes = static_cast<int32_t>(ShaderProgramType::NumTypes);

        ShaderProgram();

        const hash_t& GetHash() const { return m_Hash; }
        const ShaderKeywordSet& GetKeywords() const { return m_Keywords; }
        const uint8_t* GetBinaryData() const { return reinterpret_cast<const uint8_t*>(m_Binary->GetBufferPointer()); }
        uint64_t GetBinarySize() const { return static_cast<uint64_t>(m_Binary->GetBufferSize()); }

        const std::vector<ShaderBuffer>& GetSrvCbvBuffers() const { return m_SrvCbvBuffers; }
        const std::vector<ShaderTexture>& GetSrvTextures() const { return m_SrvTextures; }
        const std::vector<ShaderBuffer>& GetUavBuffers() const { return m_UavBuffers; }
        const std::vector<ShaderTexture>& GetUavTextures() const { return m_UavTextures; }
        const std::unordered_map<int32_t, ShaderStaticSampler>& GetStaticSamplers() const { return m_StaticSamplers; }

    private:
        hash_t m_Hash;
        ShaderKeywordSet m_Keywords;
        Microsoft::WRL::ComPtr<IDxcBlob> m_Binary;

        std::vector<ShaderBuffer> m_SrvCbvBuffers;
        std::vector<ShaderTexture> m_SrvTextures;
        std::vector<ShaderBuffer> m_UavBuffers;
        std::vector<ShaderTexture> m_UavTextures;
        std::unordered_map<int32_t, ShaderStaticSampler> m_StaticSamplers;
    };

    enum class ShaderPropertyType
    {
        Float = 0,
        Int = 1,
        Color = 2,
        Vector = 3,
        Texture = 4,
    };

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

        GfxTexture* GetDefaultTexture() const;
    };

    struct ShaderPropertyLocation
    {
        uint32_t Offset;
        uint32_t Size;
    };

    template<typename T>
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

    class GfxRootSignature
    {
        friend ShaderPass;

    public:
        struct BufferBinding
        {
            int32_t Id;
            uint32_t BindPoint;
            bool IsConstantBuffer;
        };

        struct TextureBinding
        {
            int32_t Id{};
            uint32_t BindPointTexture{};
            std::optional<uint32_t> BindPointSampler = ::std::nullopt;
        };

        struct UavBinding
        {
            int32_t Id;
            uint32_t BindPoint;
        };

        GfxRootSignature() : m_RootSignature(nullptr), m_Bindings{} {}

        ID3D12RootSignature* GetD3DRootSignature() const { return m_RootSignature.Get(); }

        std::optional<uint32_t> GetSrvUavTableRootParamIndex(ShaderProgramType type) const { return m_Bindings[static_cast<size_t>(type)].SrvUavTableRootParamIndex; }
        std::optional<uint32_t> GetSamplerTableRootParamIndex(ShaderProgramType type) const { return m_Bindings[static_cast<size_t>(type)].SamplerTableRootParamIndex; }

        const std::vector<BufferBinding>& GetSrvCbvBufferRootParamIndices(ShaderProgramType type) const { return m_Bindings[static_cast<size_t>(type)].SrvCbvBufferRootParamIndices; }
        const std::vector<TextureBinding>& GetSrvTextureTableSlots(ShaderProgramType type) const { return m_Bindings[static_cast<size_t>(type)].SrvTextureTableSlots; }
        const std::vector<UavBinding>& GetUavBufferTableSlots(ShaderProgramType type) const { return m_Bindings[static_cast<size_t>(type)].UavBufferTableSlots; }
        const std::vector<UavBinding>& GetUavTextureTableSlots(ShaderProgramType type) const { return m_Bindings[static_cast<size_t>(type)].UavTextureTableSlots; }

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

        struct
        {
            std::optional<uint32_t> SrvUavTableRootParamIndex{};
            std::optional<uint32_t> SamplerTableRootParamIndex{};

            std::vector<BufferBinding> SrvCbvBufferRootParamIndices{}; // srv/cbv buffer 都使用 root srv/cbv
            std::vector<TextureBinding> SrvTextureTableSlots{};        // srv texture 在 srv/uav table 中的位置和 sampler 在 sampler table 中的位置
            std::vector<UavBinding> UavBufferTableSlots{};             // uav buffer 在 srv/uav table 中的位置
            std::vector<UavBinding> UavTextureTableSlots{};            // uav texture 在 srv/uav table 中的位置
        } m_Bindings[ShaderProgram::NumTypes];
    };

    struct GfxPipelineState;
    struct ShaderCompilationContext;

    class ShaderPass
    {
        friend Shader;
        friend ShaderBinding;
        friend GfxPipelineState;

    public:
        ShaderPass(Shader* shader);

        Shader* GetShader() const;
        const std::string& GetName() const;
        const std::unordered_map<std::string, std::string>& GetTags() const;
        const std::unordered_map<int32_t, ShaderPropertyLocation>& GetPropertyLocations() const;
        ShaderProgram* GetProgram(ShaderProgramType type, const ShaderKeywordSet& keywords);
        ShaderProgram* GetProgram(ShaderProgramType type, int32_t index) const;
        int32_t GetProgramCount(ShaderProgramType type) const;
        const ShaderPassRenderState& GetRenderState() const;

        GfxRootSignature* GetRootSignature(const ShaderKeywordSet& keywords);

    private:
        struct ProgramMatch
        {
            int32_t Indices[ShaderProgram::NumTypes]; // -1 表示 nullptr
            size_t Hash;
        };

        const ProgramMatch& GetProgramMatch(const ShaderKeywordSet& keywords);
        bool CompileRecursive(ShaderCompilationContext& context);
        bool Compile(const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error);

        Shader* m_Shader;
        std::string m_Name;
        std::unordered_map<std::string, std::string> m_Tags;
        std::unordered_map<int32_t, ShaderPropertyLocation> m_PropertyLocations; // shader property 在 cbuffer 中的位置
        std::vector<std::unique_ptr<ShaderProgram>> m_Programs[ShaderProgram::NumTypes];
        ShaderPassRenderState m_RenderState;

        std::unordered_map<ShaderKeywordSet::data_t, ProgramMatch> m_ProgramMatches;
        std::unordered_map<size_t, std::unique_ptr<GfxRootSignature>> m_RootSignatures;
        std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_PipelineStates;
    };

    class Shader
    {
        friend ShaderPass;
        friend ShaderBinding;

    public:
        const std::string& GetName() const;
        const ShaderKeywordSpace& GetKeywordSpace() const;
        const std::unordered_map<int32_t, ShaderProperty>& GetProperties() const;
        ShaderPass* GetPass(int32_t index) const;
        int32_t GetFirstPassIndexWithTagValue(const std::string& tag, const std::string& value) const;
        ShaderPass* GetFirstPassWithTagValue(const std::string& tag, const std::string& value) const;
        int32_t GetPassCount() const;
        int32_t GetVersion() const;

        static std::string GetEngineShaderPathUnixStyle();

        static int32_t GetNameId(const std::string& name);
        static const std::string& GetIdName(int32_t id);

        static int32_t GetMaterialConstantBufferId();

        static IDxcUtils* GetDxcUtils();
        static IDxcCompiler3* GetDxcCompiler();
        static void ClearRootSignatureCache();

    private:
        std::string m_Name;
        ShaderKeywordSpace m_KeywordSpace;
        std::unordered_map<int32_t, ShaderProperty> m_Properties;
        std::vector<std::unique_ptr<ShaderPass>> m_Passes;
        int32_t m_Version = 0;
    };
}
