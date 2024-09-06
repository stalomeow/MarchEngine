#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include "Core/Debug.h"
#include "Rendering/Resource/Texture.h"
#include "Rendering/Shader.h"
#include "Rendering/Resource/GpuBuffer.h"
#include "Scripting/ScriptTypes.h"
#include <DirectXMath.h>
#include <stdint.h>
#include <unordered_map>
#include <string>
#include <memory>

namespace dx12demo
{
    class Material final
    {
    private:
        void CheckShaderVersion();
        void RecreateConstantBuffers();

    public:
        void Reset();

        void SetInt(const std::string& name, int32_t value);
        void SetFloat(const std::string& name, float value);
        void SetVector(const std::string& name, DirectX::XMFLOAT4 value);
        void SetTexture(const std::string& name, Texture* texture); // nullptr to remove

        bool GetInt(const std::string& name, int32_t* outValue) const;
        bool GetFloat(const std::string& name, float* outValue) const;
        bool GetVector(const std::string& name, DirectX::XMFLOAT4* outValue) const;
        bool GetTexture(const std::string& name, Texture** outValue) const;

        Shader* GetShader() const { return m_Shader; }
        void SetShader(Shader* pShader);

        ConstantBuffer* GetConstantBuffer(ShaderPass* pass, ConstantBuffer* defaultValue = nullptr)
        {
            CheckShaderVersion();
            auto it = m_ConstantBuffers.find(pass);
            return it == m_ConstantBuffers.end() ? defaultValue : it->second.get();
        }

    private:

        template<typename T>
        void SetConstantBufferValue(const std::string& name, const T& value)
        {
            CheckShaderVersion();

            for (const auto& kv : m_ConstantBuffers)
            {
                const ShaderPass* pass = kv.first;
                const ConstantBuffer* cb = kv.second.get();
                const auto prop = pass->MaterialProperties.find(name);

                if (prop != pass->MaterialProperties.end())
                {
                    BYTE* p = cb->GetPointer(0);
                    assert(sizeof(value) >= prop->second.Size); // 有时候会把 Vector4 绑定到 Vector3 上，所以用 >=
                    memcpy(p + prop->second.Offset, &value, prop->second.Size);
                }
            }
        }

        Shader* m_Shader;
        int32_t m_ShaderVersion;
        std::unordered_map<ShaderPass*, std::unique_ptr<ConstantBuffer>> m_ConstantBuffers;

        std::unordered_map<std::string, int32_t> m_Ints;
        std::unordered_map<std::string, float> m_Floats;
        std::unordered_map<std::string, DirectX::XMFLOAT4> m_Vectors;
        std::unordered_map<std::string, Texture*> m_Textures;
    };

    namespace binding
    {
        inline CSHARP_API(Material*) Material_New()
        {
            return new Material();
        }

        inline CSHARP_API(void) Material_Delete(Material* pMaterial)
        {
            delete pMaterial;
        }

        inline CSHARP_API(void) Material_Reset(Material* pMaterial)
        {
            pMaterial->Reset();
        }

        inline CSHARP_API(void) Material_SetShader(Material* pMaterial, Shader* pShader)
        {
            pMaterial->SetShader(pShader);
        }

        inline CSHARP_API(void) Material_SetInt(Material* pMaterial, CSharpString name, CSharpInt value)
        {
            pMaterial->SetInt(CSharpString_ToUtf8(name), static_cast<int32_t>(value));
        }

        inline CSHARP_API(void) Material_SetFloat(Material* pMaterial, CSharpString name, CSharpFloat value)
        {
            pMaterial->SetFloat(CSharpString_ToUtf8(name), static_cast<float>(value));
        }

        inline CSHARP_API(void) Material_SetVector(Material* pMaterial, CSharpString name, CSharpVector4 value)
        {
            pMaterial->SetVector(CSharpString_ToUtf8(name), ToXMFLOAT4(value));
        }

        inline CSHARP_API(void) Material_SetTexture(Material* pMaterial, CSharpString name, Texture* pTexture)
        {
            pMaterial->SetTexture(CSharpString_ToUtf8(name), pTexture);
        }
    }
}
