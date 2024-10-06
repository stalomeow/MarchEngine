#include "GfxTexture.h"
#include "GfxDevice.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO Texture_New()
{
    retcs DBG_NEW GfxTexture2D(GetGfxDevice());
}

NATIVE_EXPORT_AUTO Texture_Delete(cs<GfxTexture*> pTexture)
{
    delete pTexture;
}

NATIVE_EXPORT_AUTO Texture_SetDDSData(cs<GfxTexture*> pTexture, cs_string name, cs<void*> pSourceDDS, cs_int size)
{
    auto tex2D = reinterpret_cast<GfxTexture2D*>(pTexture.data);
    tex2D->LoadFromDDS(name, pSourceDDS, static_cast<uint32_t>(size));
}

NATIVE_EXPORT_AUTO Texture_SetFilterMode(cs<GfxTexture*> pTexture, cs<GfxFilterMode> mode)
{
    pTexture->SetFilterMode(mode);
}

NATIVE_EXPORT_AUTO Texture_SetWrapMode(cs<GfxTexture*> pTexture, cs<GfxWrapMode> mode)
{
    pTexture->SetWrapMode(mode);
}

NATIVE_EXPORT_AUTO Texture_GetFilterMode(cs<GfxTexture*> pTexture)
{
    retcs pTexture->GetFilterMode();
}

NATIVE_EXPORT_AUTO Texture_GetWrapMode(cs<GfxTexture*> pTexture)
{
    retcs pTexture->GetWrapMode();
}
