#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include "Rendering/Resource/Texture.h"
#include "Rendering/Shader.h"
#include "Scripting/ScriptTypes.h"
#include <DirectXMath.h>
#include <stdint.h>
#include <unordered_map>
#include <string>

namespace dx12demo
{
    class Material
    {
    public:
        void SetInt(std::string name, int32_t value);
        void SetFloat(std::string name, float value);
        void SetVector(std::string name, DirectX::XMFLOAT4 value);
        void SetTexture(std::string name, Texture* texture);

        Shader* pShader;
        std::unordered_map<std::string, int32_t> Ints;
        std::unordered_map<std::string, float> Floats;
        std::unordered_map<std::string, DirectX::XMFLOAT4> Vectors;
        std::unordered_map<std::string, Texture*> Textures;
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

        inline CSHARP_API(void) Material_SetShader(Material* pMaterial, Shader* pShader)
        {
            pMaterial->pShader = pShader;
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
