#pragma once

#include "Engine/Object.h"
#include "Engine/HashUtils.h"
#include <directx/d3dx12.h>
#include <d3d12shader.h> // Shader reflection
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
#include <limits>

namespace march
{
    class GfxTexture;
    class Shader;
    class ShaderPass;
    class ShaderBinding;
    class ShaderKeywordSet;
    class ShaderKeywordSpace;
    class ComputeShaderBinding;

    enum class GfxDefaultTexture;
    enum class GfxTextureDimension;

    class ShaderKeywordSet
    {
    public:
        using DataType = std::bitset<128>;

        ShaderKeywordSet();

        size_t GetEnabledKeywordCount() const;
        size_t GetMatchingKeywordCount(const ShaderKeywordSet& other) const;
        std::vector<std::string> GetEnabledKeywords(const ShaderKeywordSpace& space) const;
        const DataType& GetData() const { return m_Keywords; }

        void SetKeyword(const ShaderKeywordSpace& space, const std::string& keyword, bool value);
        void EnableKeyword(const ShaderKeywordSpace& space, const std::string& keyword);
        void DisableKeyword(const ShaderKeywordSpace& space, const std::string& keyword);
        void Clear();

    private:
        DataType m_Keywords;
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

    struct alignas(4) ShaderProgramHash
    {
        uint8_t Data[16];

        void SetData(const DxcShaderHash& hash);
        bool operator==(const ShaderProgramHash& other) const;
        bool operator!=(const ShaderProgramHash& other) const;
    };

    class ShaderProgram
    {
        friend ShaderBinding;
        friend ComputeShaderBinding;
        friend struct ShaderProgramUtils;

    public:
        ShaderProgram();

        const ShaderProgramHash& GetHash() const { return m_Hash; }
        const ShaderKeywordSet& GetKeywords() const { return m_Keywords; }
        const uint8_t* GetBinaryData() const { return reinterpret_cast<const uint8_t*>(m_Binary->GetBufferPointer()); }
        uint64_t GetBinarySize() const { return static_cast<uint64_t>(m_Binary->GetBufferSize()); }

        const std::vector<ShaderBuffer>& GetSrvCbvBuffers() const { return m_SrvCbvBuffers; }
        const std::vector<ShaderTexture>& GetSrvTextures() const { return m_SrvTextures; }
        const std::vector<ShaderBuffer>& GetUavBuffers() const { return m_UavBuffers; }
        const std::vector<ShaderTexture>& GetUavTextures() const { return m_UavTextures; }
        const std::unordered_map<int32_t, ShaderStaticSampler>& GetStaticSamplers() const { return m_StaticSamplers; }

        void GetThreadGroupSize(uint32_t* pOutX, uint32_t* pOutY, uint32_t* pOutZ) const
        {
            *pOutX = m_ThreadGroupSizeX;
            *pOutY = m_ThreadGroupSizeY;
            *pOutZ = m_ThreadGroupSizeZ;
        }

    private:
        ShaderProgramHash m_Hash;
        ShaderKeywordSet m_Keywords;
        Microsoft::WRL::ComPtr<IDxcBlob> m_Binary;

        std::vector<ShaderBuffer> m_SrvCbvBuffers;
        std::vector<ShaderTexture> m_SrvTextures;
        std::vector<ShaderBuffer> m_UavBuffers;
        std::vector<ShaderTexture> m_UavTextures;
        std::unordered_map<int32_t, ShaderStaticSampler> m_StaticSamplers;

        uint32_t m_ThreadGroupSizeX;
        uint32_t m_ThreadGroupSizeY;
        uint32_t m_ThreadGroupSizeZ;
    };

    struct GfxRootSignatureBufferBinding
    {
        int32_t Id;
        uint32_t BindPoint;
        bool IsConstantBuffer;
    };

    struct GfxRootSignatureTextureBinding
    {
        int32_t Id;
        uint32_t BindPointTexture;
        std::optional<uint32_t> BindPointSampler;
    };

    struct GfxRootSignatureUavBinding
    {
        int32_t Id;
        uint32_t BindPoint;
    };

    template <size_t _NumProgramTypes>
    class GfxRootSignature final
    {
        friend struct GfxRootSignatureUtils;

        static_assert(_NumProgramTypes > 0, "_NumProgramTypes must be greater than 0");

    public:
        static constexpr size_t NumProgramTypes = _NumProgramTypes;

        GfxRootSignature() : m_RootSignature(nullptr), m_Bindings{} {}

        ID3D12RootSignature* GetD3DRootSignature() const { return m_RootSignature.Get(); }

        template <typename T>
        auto GetSrvUavTableRootParamIndex(T index) const { return GetBinding(index).SrvUavTableRootParamIndex; }

        template <typename T>
        auto GetSamplerTableRootParamIndex(T index) const { return GetBinding(index).SamplerTableRootParamIndex; }

        template <typename T>
        const auto& GetSrvCbvBufferRootParamIndices(T index) const { return GetBinding(index).SrvCbvBufferRootParamIndices; }

        template <typename T>
        const auto& GetSrvTextureTableSlots(T index) const { return GetBinding(index).SrvTextureTableSlots; }

        template <typename T>
        const auto& GetUavBufferTableSlots(T index) const { return GetBinding(index).UavBufferTableSlots; }

        template <typename T>
        const auto& GetUavTextureTableSlots(T index) const { return GetBinding(index).UavTextureTableSlots; }

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

        struct
        {
            std::optional<uint32_t> SrvUavTableRootParamIndex = std::nullopt;
            std::optional<uint32_t> SamplerTableRootParamIndex = std::nullopt;

            std::vector<GfxRootSignatureBufferBinding> SrvCbvBufferRootParamIndices{}; // srv/cbv buffer 都使用 root srv/cbv
            std::vector<GfxRootSignatureTextureBinding> SrvTextureTableSlots{};        // srv texture 在 srv/uav table 中的位置和 sampler 在 sampler table 中的位置
            std::vector<GfxRootSignatureUavBinding> UavBufferTableSlots{};             // uav buffer 在 srv/uav table 中的位置
            std::vector<GfxRootSignatureUavBinding> UavTextureTableSlots{};            // uav texture 在 srv/uav table 中的位置
        } m_Bindings[NumProgramTypes];

        template <typename T>
        const auto& GetBinding(T index) const
        {
            size_t i = static_cast<size_t>(index);

            if (i >= NumProgramTypes)
            {
                throw std::out_of_range("Program index out of range");
            }

            return m_Bindings[i];
        }
    };

    template <size_t _NumProgramTypes>
    class ShaderProgramGroup
    {
        friend ShaderBinding;
        friend ComputeShaderBinding;
        friend struct GfxPipelineState;
        friend struct GfxRootSignatureUtils;
        friend struct ShaderProgramUtils;
        friend class ComputeShader;

    public:
        using RootSignatureType = GfxRootSignature<_NumProgramTypes>;

        const std::string& GetName() const { return m_Name; }

        template <typename T>
        ShaderProgram* GetProgram(T type, const ShaderKeywordSet& keywords)
        {
            size_t typeIndex = static_cast<size_t>(type);
            std::optional<size_t> programIndex = GetProgramMatch(keywords).Indices[typeIndex];
            return programIndex ? m_Programs[typeIndex][*programIndex].get() : nullptr;
        }

        template <typename T>
        ShaderProgram* GetProgram(T type, size_t index) const
        {
            return m_Programs[static_cast<size_t>(type)][index].get();
        }

        template <typename T>
        size_t GetProgramCount(T type) const
        {
            return m_Programs[static_cast<size_t>(type)].size();
        }

        virtual ~ShaderProgramGroup() = default;

    private:
        struct ProgramMatch
        {
            std::optional<size_t> Indices[_NumProgramTypes]{};
            size_t Hash{};
        };

        std::string m_Name;
        std::vector<std::unique_ptr<ShaderProgram>> m_Programs[_NumProgramTypes];

        std::unordered_map<ShaderKeywordSet::DataType, ProgramMatch> m_ProgramMatches;
        std::unordered_map<size_t, std::unique_ptr<RootSignatureType>> m_RootSignatures;
        std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_PipelineStates;

        static __forceinline size_t AbsDiff(size_t a, size_t b)
        {
            return a > b ? a - b : b - a;
        }

        const ProgramMatch& GetProgramMatch(const ShaderKeywordSet& keywords)
        {
            auto [it, isNew] = m_ProgramMatches.try_emplace(keywords.GetData());

            if (isNew)
            {
                DefaultHash hash{};
                ProgramMatch& m = it->second;
                size_t targetKeywordCount = keywords.GetEnabledKeywordCount();

                for (size_t i = 0; i < _NumProgramTypes; i++)
                {
                    size_t minDiff = std::numeric_limits<size_t>::max();
                    m.Indices[i] = std::nullopt;

                    for (size_t j = 0; j < m_Programs[i].size(); j++)
                    {
                        const ShaderKeywordSet& ks = m_Programs[i][j]->GetKeywords();
                        size_t matchingCount = ks.GetMatchingKeywordCount(keywords);
                        size_t enabledCount = ks.GetEnabledKeywordCount();

                        // 没 match 的数量 + 多余的数量
                        size_t diff = AbsDiff(targetKeywordCount, matchingCount) + AbsDiff(enabledCount, matchingCount);

                        if (diff < minDiff)
                        {
                            minDiff = diff;
                            m.Indices[i] = j;
                        }
                    }

                    if (m.Indices[i])
                    {
                        ShaderProgram* program = m_Programs[i][*m.Indices[i]].get();
                        hash.Append(program->GetHash());
                    }
                }

                m.Hash = *hash;
            }

            return it->second;
        }

        virtual D3D12_SHADER_VISIBILITY GetShaderVisibility(size_t programType) = 0;
        virtual bool GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType) = 0;
        virtual std::string GetTargetProfile(const std::string& shaderModel, size_t programType) = 0;
        virtual void RecordEntrypointCallback(size_t programType, std::string& entrypoint) = 0;
        virtual void RecordConstantBufferCallback(ID3D12ShaderReflectionConstantBuffer* cbuffer) = 0;
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

    enum class ShaderProgramType
    {
        Vertex,
        Pixel,
        Domain,
        Hull,
        Geometry,
    };

    class ShaderPass final : public ShaderProgramGroup<5>
    {
        friend ShaderBinding;

    public:
        RootSignatureType* GetRootSignature(const ShaderKeywordSet& keywords);

        const std::unordered_map<std::string, std::string>& GetTags() const { return m_Tags; }
        const std::unordered_map<int32_t, ShaderPropertyLocation>& GetPropertyLocations() const { return m_PropertyLocations; }
        const ShaderPassRenderState& GetRenderState() const { return m_RenderState; }

    protected:
        D3D12_SHADER_VISIBILITY GetShaderVisibility(size_t programType) override;
        bool GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType) override;
        std::string GetTargetProfile(const std::string& shaderModel, size_t programType) override;
        void RecordEntrypointCallback(size_t programType, std::string& entrypoint) override {}
        void RecordConstantBufferCallback(ID3D12ShaderReflectionConstantBuffer* cbuffer) override;

    private:
        std::unordered_map<std::string, std::string> m_Tags;
        std::unordered_map<int32_t, ShaderPropertyLocation> m_PropertyLocations; // shader property 在 cbuffer 中的位置
        ShaderPassRenderState m_RenderState;

        bool Compile(ShaderKeywordSpace& keywordSpace, const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error);
    };

    class Shader : public MarchObject
    {
        friend ShaderBinding;

    public:
        using RootSignatureType = ShaderPass::RootSignatureType;
        static constexpr size_t NumProgramTypes = RootSignatureType::NumProgramTypes;

        ShaderPass* GetPass(size_t index) const;
        std::optional<size_t> GetFirstPassIndexWithTagValue(const std::string& tag, const std::string& value) const;
        ShaderPass* GetFirstPassWithTagValue(const std::string& tag, const std::string& value) const;

        const std::string& GetName() const { return m_Name; }
        const ShaderKeywordSpace& GetKeywordSpace() const { return m_KeywordSpace; }
        const std::unordered_map<int32_t, ShaderProperty>& GetProperties() const { return m_Properties; }
        size_t GetPassCount() const { return m_Passes.size(); }
        uint32_t GetVersion() const { return m_Version; }

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
        uint32_t m_Version;
    };

    class ComputeShaderKernel final : public ShaderProgramGroup<1>
    {
        using Base = ShaderProgramGroup<1>;

    public:
        RootSignatureType* GetRootSignature(const ShaderKeywordSet& keywords);

        ShaderProgram* GetProgram(const ShaderKeywordSet& keywords) { return Base::GetProgram(0, keywords); }

        ShaderProgram* GetProgram(size_t index) const { return Base::GetProgram(0, index); }

        size_t GetProgramCount() const { return Base::GetProgramCount(0); }

        void GetThreadGroupSize(const ShaderKeywordSet& keywords, uint32_t* pOutX, uint32_t* pOutY, uint32_t* pOutZ)
        {
            ShaderProgram* program = GetProgram(keywords);
            program->GetThreadGroupSize(pOutX, pOutY, pOutZ);
        }

    protected:
        D3D12_SHADER_VISIBILITY GetShaderVisibility(size_t programType) override;
        bool GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType) override;
        std::string GetTargetProfile(const std::string& shaderModel, size_t programType) override;
        void RecordEntrypointCallback(size_t programType, std::string& entrypoint) override;
        void RecordConstantBufferCallback(ID3D12ShaderReflectionConstantBuffer* cbuffer) override {}
    };

    class ComputeShader : public MarchObject
    {
        friend ComputeShaderBinding;

    public:
        using RootSignatureType = ComputeShaderKernel::RootSignatureType;
        static constexpr size_t NumProgramTypes = RootSignatureType::NumProgramTypes;

        ComputeShaderKernel* GetKernel(size_t index) const;
        ComputeShaderKernel* GetKernel(const std::string& name) const;

        const std::string& GetName() const { return m_Name; }
        const ShaderKeywordSpace& GetKeywordSpace() const { return m_KeywordSpace; }
        size_t GetKernelCount() const { return m_Kernels.size(); }

    private:
        std::string m_Name;
        ShaderKeywordSpace m_KeywordSpace;
        std::vector<std::unique_ptr<ComputeShaderKernel>> m_Kernels;

        bool Compile(const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error);
    };
}
