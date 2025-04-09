#pragma once

#include "Engine/Object.h"
#include "Engine/Rendering/D3D12Impl/GfxPipeline.h"
#include "Engine/Rendering/D3D12Impl/ShaderGraphics.h"
#include <directx/d3dx12.h>
#include <DirectXMath.h>
#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <optional>
#include <memory>

namespace march
{
    class GfxBuffer;
    class GfxTexture;

    class Material final : public MarchObject
    {
        friend struct MaterialInternalUtility;

        struct ResolvedRenderState
        {
            std::optional<ShaderPassRenderState> State = std::nullopt;
            size_t Hash{};
            uint32_t Version{};
        };

        Shader* m_Shader = nullptr;
        uint32_t m_ShaderVersion = 0;

        DynamicShaderKeywordSet m_Keywords{};
        bool m_IsKeywordDirty = true;

        std::unordered_map<int32_t, int32_t> m_Ints{};
        std::unordered_map<int32_t, float> m_Floats{};
        std::unordered_map<int32_t, DirectX::XMFLOAT4> m_Vectors{};
        std::unordered_map<int32_t, DirectX::XMFLOAT4> m_Colors{};
        std::unordered_map<int32_t, GfxTexture*> m_Textures{};

        std::unique_ptr<GfxBuffer> m_ConstantBuffer = nullptr;
        bool m_IsConstantBufferDirty = true;

        // 每个 pass 都有一个 ResolvedRenderState
        std::vector<ResolvedRenderState> m_ResolvedRenderStates{};
        uint32_t m_ResolvedRenderStateVersion = 0;

        void CheckShaderVersion();
        void UpdateKeywords();

    public:
        Material() = default;
        Material(Shader* shader) : Material() { SetShader(shader); }

        void Reset();

        void SetInt(int32_t id, int32_t value);
        void SetFloat(int32_t id, float value);
        void SetVector(int32_t id, const DirectX::XMFLOAT4& value);
        void SetColor(int32_t id, const DirectX::XMFLOAT4& value);
        void SetTexture(int32_t id, GfxTexture* texture); // nullptr to remove

        void SetInt(const std::string& name, int32_t value);
        void SetFloat(const std::string& name, float value);
        void SetVector(const std::string& name, const DirectX::XMFLOAT4& value);
        void SetColor(const std::string& name, const DirectX::XMFLOAT4& value);
        void SetTexture(const std::string& name, GfxTexture* texture); // nullptr to remove

        bool GetInt(int32_t id, int32_t* outValue) const;
        bool GetFloat(int32_t id, float* outValue) const;
        bool GetVector(int32_t id, DirectX::XMFLOAT4* outValue) const;
        bool GetColor(int32_t id, DirectX::XMFLOAT4* outValue) const;
        bool GetTexture(int32_t id, GfxTexture** outValue) const;

        bool GetInt(const std::string& name, int32_t* outValue) const;
        bool GetFloat(const std::string& name, float* outValue) const;
        bool GetVector(const std::string& name, DirectX::XMFLOAT4* outValue) const;
        bool GetColor(const std::string& name, DirectX::XMFLOAT4* outValue) const;
        bool GetTexture(const std::string& name, GfxTexture** outValue) const;

        Shader* GetShader() const;
        void SetShader(Shader* shader);

        const ShaderKeywordSet& GetKeywords();
        void SetKeyword(int32_t id, bool value);
        void EnableKeyword(int32_t id);
        void DisableKeyword(int32_t id);
        void SetKeyword(const std::string& keyword, bool value);
        void EnableKeyword(const std::string& keyword);
        void DisableKeyword(const std::string& keyword);

        GfxBuffer* GetConstantBuffer(size_t passIndex);
        const ShaderPassRenderState& GetResolvedRenderState(size_t passIndex, size_t* outHash = nullptr);
        ID3D12PipelineState* GetPSO(size_t passIndex, bool hasOddNegativeScaling, const GfxInputDesc& inputDesc, const GfxOutputDesc& outputDesc);
    };

    struct MaterialInternalUtility
    {
        static const std::unordered_map<int32_t, int32_t>& GetRawInts(Material* m);
        static const std::unordered_map<int32_t, float>& GetRawFloats(Material* m);
        static const std::unordered_map<int32_t, DirectX::XMFLOAT4>& GetRawVectors(Material* m);
        static const std::unordered_map<int32_t, DirectX::XMFLOAT4>& GetRawColors(Material* m);
        static const std::unordered_map<int32_t, GfxTexture*>& GetRawTextures(Material* m);
        static std::vector<std::string> GetRawEnabledKeywords(Material* m);
    };
}
