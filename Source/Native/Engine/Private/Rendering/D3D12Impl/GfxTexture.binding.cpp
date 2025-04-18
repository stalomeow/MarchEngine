#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxTexture.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Scripting/InteropServices.h"

struct CSharpTextureDesc
{
    cs<GfxTextureFormat> Format;
    cs<GfxTextureFlags> Flags;

    cs<GfxTextureDimension> Dimension;
    cs_uint Width;
    cs_uint Height;
    cs_uint DepthOrArraySize;
    cs_uint MSAASamples;

    cs<GfxTextureFilterMode> Filter;
    cs<GfxTextureWrapMode> Wrap;
    cs_float MipmapBias;
};

struct CSharpLoadTextureFileArgs
{
    cs<GfxTextureFlags> Flags;
    cs<GfxTextureFilterMode> Filter;
    cs<GfxTextureWrapMode> Wrap;
    cs_float MipmapBias;
    cs<GfxTextureCompression> Compression;
};

NATIVE_EXPORT_AUTO GfxTexture_GetMipLevels(cs<GfxTexture*> t)
{
    retcs t->GetMipLevels();
}

NATIVE_EXPORT_AUTO GfxTexture_GetDesc(cs<GfxTexture*> t)
{
    const GfxTextureDesc& desc = t->GetDesc();

    CSharpTextureDesc result{};
    result.Format.assign(desc.Format);
    result.Flags.assign(desc.Flags);
    result.Dimension.assign(desc.Dimension);
    result.Width.assign(desc.Width);
    result.Height.assign(desc.Height);
    result.DepthOrArraySize.assign(desc.DepthOrArraySize);
    result.MSAASamples.assign(desc.MSAASamples);
    result.Filter.assign(desc.Filter);
    result.Wrap.assign(desc.Wrap);
    result.MipmapBias.assign(desc.MipmapBias);
    retcs result;
}

NATIVE_EXPORT_AUTO GfxTexture_GetIsReadOnly(cs<GfxTexture*> t)
{
    retcs t->IsReadOnly();
}

NATIVE_EXPORT_AUTO GfxExternalTexture_New()
{
    retcs MARCH_NEW GfxExternalTexture(GetGfxDevice());
}

NATIVE_EXPORT_AUTO GfxExternalTexture_GetName(cs<GfxExternalTexture*> t)
{
    retcs t->GetName();
}

NATIVE_EXPORT_AUTO GfxExternalTexture_GetPixelsData(cs<GfxExternalTexture*> t)
{
    retcs t->GetPixelsData();
}

NATIVE_EXPORT_AUTO GfxExternalTexture_GetPixelsSize(cs<GfxExternalTexture*> t)
{
    retcs static_cast<int64_t>(t->GetPixelsSize());
}

NATIVE_EXPORT_AUTO GfxExternalTexture_LoadFromPixels(cs<GfxExternalTexture*> t, cs_string name, cs_ptr<CSharpTextureDesc> desc, cs_ptr<cs_void> pixelsData, cs_long pixelsSize, cs_uint mipLevels)
{
    GfxTextureDesc d =
    {
        desc->Format,
        desc->Flags,
        desc->Dimension,
        desc->Width,
        desc->Height,
        desc->DepthOrArraySize,
        desc->MSAASamples,
        desc->Filter,
        desc->Wrap,
        desc->MipmapBias
    };

    t->LoadFromPixels(name, d, pixelsData, static_cast<size_t>(pixelsSize.data), mipLevels);
}

NATIVE_EXPORT_AUTO GfxExternalTexture_LoadFromFile(cs<GfxExternalTexture*> t, cs_string name, cs_string filePath, cs_ptr<CSharpLoadTextureFileArgs> args)
{
    LoadTextureFileArgs as =
    {
        args->Flags,
        args->Filter,
        args->Wrap,
        args->MipmapBias,
        args->Compression
    };

    t->LoadFromFile(name, filePath, as);
}
