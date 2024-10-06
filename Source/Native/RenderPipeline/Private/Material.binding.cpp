#include "Material.h"
#include "InteropServices.h"
#include "GfxTexture.h"

NATIVE_EXPORT_AUTO Material_New()
{
    retcs new Material();
}

NATIVE_EXPORT_AUTO Material_Delete(cs<Material*> pMaterial)
{
    delete pMaterial;
}

NATIVE_EXPORT_AUTO Material_Reset(cs<Material*> pMaterial)
{
    pMaterial->Reset();
}

NATIVE_EXPORT_AUTO Material_SetShader(cs<Material*> pMaterial, cs<Shader*> pShader)
{
    pMaterial->SetShader(pShader);
}

NATIVE_EXPORT_AUTO Material_SetInt(cs<Material*> pMaterial, cs_string name, cs_int value)
{
    pMaterial->SetInt(name, value);
}

NATIVE_EXPORT_AUTO Material_SetFloat(cs<Material*> pMaterial, cs_string name, cs_float value)
{
    pMaterial->SetFloat(name, value);
}

NATIVE_EXPORT_AUTO Material_SetVector(cs<Material*> pMaterial, cs_string name, cs_vec4 value)
{
    pMaterial->SetVector(name, value);
}

NATIVE_EXPORT_AUTO Material_SetTexture(cs<Material*> pMaterial, cs_string name, cs<GfxTexture*> pTexture)
{
    pMaterial->SetTexture(name, pTexture);
}
