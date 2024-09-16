#include "Texture.h"
#include "ScriptTypes.h"

using namespace march;

NATIVE_EXPORT(Texture*) Texture_New()
{
    return new Texture();
}

NATIVE_EXPORT(void) Texture_Delete(Texture* pTexture)
{
    delete pTexture;
}

NATIVE_EXPORT(void) Texture_SetDDSData(Texture* pTexture, CSharpString name, const void* pSourceDDS, CSharpInt size)
{
    pTexture->SetDDSData(CSharpString_ToUtf16(name), pSourceDDS, static_cast<size_t>(size));
}

NATIVE_EXPORT(void) Texture_SetFilterMode(Texture* pTexture, FilterMode mode)
{
    pTexture->SetFilterMode(mode);
}

NATIVE_EXPORT(void) Texture_SetWrapMode(Texture* pTexture, WrapMode mode)
{
    pTexture->SetWrapMode(mode);
}

NATIVE_EXPORT(FilterMode) Texture_GetFilterMode(Texture* pTexture)
{
    return pTexture->GetFilterMode();
}

NATIVE_EXPORT(WrapMode) Texture_GetWrapMode(Texture* pTexture)
{
    return pTexture->GetWrapMode();
}
