#include "GfxTexture.h"
#include "GfxDevice.h"
#include "InteropServices.h"

using namespace march;

NATIVE_EXPORT(GfxTexture*) Texture_New()
{
    return new GfxTexture2D(GetGfxDevice());
}

NATIVE_EXPORT(void) Texture_Delete(GfxTexture* pTexture)
{
    delete pTexture;
}

NATIVE_EXPORT(void) Texture_SetDDSData(GfxTexture* pTexture, CSharpString name, const void* pSourceDDS, CSharpInt size)
{
    reinterpret_cast<GfxTexture2D*>(pTexture)->LoadFromDDS(CSharpString_ToUtf8(name), pSourceDDS, static_cast<size_t>(size));
}

NATIVE_EXPORT(void) Texture_SetFilterMode(GfxTexture* pTexture, CSharpInt mode)
{
    pTexture->SetFilterMode(static_cast<GfxFilterMode>(mode));
}

NATIVE_EXPORT(void) Texture_SetWrapMode(GfxTexture* pTexture, CSharpInt mode)
{
    pTexture->SetWrapMode(static_cast<GfxWrapMode>(mode));
}

NATIVE_EXPORT(CSharpInt) Texture_GetFilterMode(GfxTexture* pTexture)
{
    return static_cast<CSharpInt>(pTexture->GetFilterMode());
}

NATIVE_EXPORT(CSharpInt) Texture_GetWrapMode(GfxTexture* pTexture)
{
    return static_cast<CSharpInt>(pTexture->GetWrapMode());
}
