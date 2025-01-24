#pragma once

#include "Engine/Object.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/GfxBuffer.h"
#include <DirectXMath.h>
#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace march
{
    class GfxTexture;
    class MaterialInternalUtility;

    class Material final : public MarchObject
    {
        friend MaterialInternalUtility;

    public:
        Material();
        Material(Shader* shader);

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
        void SetShader(Shader* pShader);

        const ShaderKeywordSet& GetKeywords();
        void EnableKeyword(const std::string& keyword);
        void DisableKeyword(const std::string& keyword);
        void SetKeyword(const std::string& keyword, bool value);
        GfxBuffer* GetConstantBuffer(int32_t passIndex);
        const ShaderPassRenderState& GetResolvedRenderState(int32_t passIndex, size_t* outHash = nullptr);

    private:
        void CheckShaderVersion();
        void RebuildKeywordCache();
        void ClearConstantBuffers();
        void ClearResolvedRenderStates();

        Shader* m_Shader;
        int32_t m_ShaderVersion;
        ShaderKeywordSet m_KeywordCache;
        std::unordered_set<std::string> m_EnabledKeywords;
        std::unordered_map<int32_t, GfxBuffer> m_ConstantBuffers; // Key 是 ShaderPassIndex

        // Key 是 ShaderPassIndex, Value 是 ShaderPassRenderState 和对应的 Hash
        std::unordered_map<int32_t, std::pair<ShaderPassRenderState, size_t>> m_ResolvedRenderStates;

        std::unordered_map<int32_t, int32_t> m_Ints;
        std::unordered_map<int32_t, float> m_Floats;
        std::unordered_map<int32_t, DirectX::XMFLOAT4> m_Vectors;
        std::unordered_map<int32_t, DirectX::XMFLOAT4> m_Colors;
        std::unordered_map<int32_t, GfxTexture*> m_Textures;
    };

    class MaterialInternalUtility
    {
    public:
        static const std::unordered_map<int32_t, int32_t>& GetRawInts(Material* m);
        static const std::unordered_map<int32_t, float>& GetRawFloats(Material* m);
        static const std::unordered_map<int32_t, DirectX::XMFLOAT4>& GetRawVectors(Material* m);
        static const std::unordered_map<int32_t, DirectX::XMFLOAT4>& GetRawColors(Material* m);
        static const std::unordered_map<int32_t, GfxTexture*>& GetRawTextures(Material* m);
        static const std::unordered_set<std::string>& GetRawEnabledKeywords(Material* m);
    };
}
