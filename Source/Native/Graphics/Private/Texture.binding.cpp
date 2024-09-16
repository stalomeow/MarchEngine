#include "Texture.h"
#include "InteropServices.h"

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

NATIVE_EXPORT(void) Texture_SetFilterMode(Texture* pTexture, CSharpInt mode)
{
    pTexture->SetFilterMode(static_cast<FilterMode>(mode));
}

NATIVE_EXPORT(void) Texture_SetWrapMode(Texture* pTexture, CSharpInt mode)
{
    pTexture->SetWrapMode(static_cast<WrapMode>(mode));
}

NATIVE_EXPORT(CSharpInt) Texture_GetFilterMode(Texture* pTexture)
{
    return static_cast<CSharpInt>(pTexture->GetFilterMode());
}

NATIVE_EXPORT(CSharpInt) Texture_GetWrapMode(Texture* pTexture)
{
    return static_cast<CSharpInt>(pTexture->GetWrapMode());
}
