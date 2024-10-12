#pragma once

#include "GfxBuffer.h"
#include <DirectXMath.h>
#include <stdint.h>
#include <unordered_map>
#include <string>
#include <memory>

namespace march
{
    class Shader;
    class ShaderPass;
    class GfxTexture;
    class GfxConstantBuffer;
    class MaterialInternalUtility;

    class Material final
    {
        friend MaterialInternalUtility;

    public:
        Material();
        ~Material() = default;

        void Reset();

        void SetInt(const std::string& name, int32_t value);
        void SetFloat(const std::string& name, float value);
        void SetVector(const std::string& name, const DirectX::XMFLOAT4& value);
        void SetColor(const std::string& name, const DirectX::XMFLOAT4& value);
        void SetTexture(const std::string& name, GfxTexture* texture); // nullptr to remove

        bool GetInt(const std::string& name, int32_t* outValue) const;
        bool GetFloat(const std::string& name, float* outValue) const;
        bool GetVector(const std::string& name, DirectX::XMFLOAT4* outValue) const;
        bool GetColor(const std::string& name, DirectX::XMFLOAT4* outValue) const;
        bool GetTexture(const std::string& name, GfxTexture** outValue) const;

        Shader* GetShader() const;
        void SetShader(Shader* pShader);
        GfxConstantBuffer* GetConstantBuffer(ShaderPass* pass, GfxConstantBuffer* defaultValue = nullptr);

    private:
        void CheckShaderVersion();
        void RecreateConstantBuffers();

        template<typename T>
        void SetConstantBufferValue(const std::string& name, const T& value);

        Shader* m_Shader;
        int32_t m_ShaderVersion;
        std::unordered_map<ShaderPass*, std::unique_ptr<GfxConstantBuffer>> m_ConstantBuffers;

        std::unordered_map<std::string, int32_t> m_Ints;
        std::unordered_map<std::string, float> m_Floats;
        std::unordered_map<std::string, DirectX::XMFLOAT4> m_Vectors;
        std::unordered_map<std::string, DirectX::XMFLOAT4> m_Colors;
        std::unordered_map<std::string, GfxTexture*> m_Textures;
    };

    class MaterialInternalUtility
    {
    public:
        static const std::unordered_map<std::string, int32_t>& GetRawInts(Material* m);
        static const std::unordered_map<std::string, float>& GetRawFloats(Material* m);
        static const std::unordered_map<std::string, DirectX::XMFLOAT4>& GetRawVectors(Material* m);
        static const std::unordered_map<std::string, DirectX::XMFLOAT4>& GetRawColors(Material* m);
        static const std::unordered_map<std::string, GfxTexture*>& GetRawTextures(Material* m);
    };
}
