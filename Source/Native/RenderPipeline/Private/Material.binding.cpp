#include "Material.h"
#include "InteropServices.h"
#include "GfxTexture.h"

using namespace march;

NATIVE_EXPORT(Material*) Material_New()
{
    return new Material();
}

NATIVE_EXPORT(void) Material_Delete(Material* pMaterial)
{
    delete pMaterial;
}

NATIVE_EXPORT(void) Material_Reset(Material* pMaterial)
{
    pMaterial->Reset();
}

NATIVE_EXPORT(void) Material_SetShader(Material* pMaterial, Shader* pShader)
{
    pMaterial->SetShader(pShader);
}

NATIVE_EXPORT(void) Material_SetInt(Material* pMaterial, CSharpString name, CSharpInt value)
{
    pMaterial->SetInt(CSharpString_ToUtf8(name), static_cast<int32_t>(value));
}

NATIVE_EXPORT(void) Material_SetFloat(Material* pMaterial, CSharpString name, CSharpFloat value)
{
    pMaterial->SetFloat(CSharpString_ToUtf8(name), static_cast<float>(value));
}

NATIVE_EXPORT(void) Material_SetVector(Material* pMaterial, CSharpString name, CSharpVector4 value)
{
    pMaterial->SetVector(CSharpString_ToUtf8(name), ToXMFLOAT4(value));
}

NATIVE_EXPORT(void) Material_SetTexture(Material* pMaterial, CSharpString name, GfxTexture* pTexture)
{
    pMaterial->SetTexture(CSharpString_ToUtf8(name), pTexture);
}
