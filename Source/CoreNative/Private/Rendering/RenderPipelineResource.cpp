#include "pch.h"
#include "Engine/Rendering/RenderPipeline.h"

namespace march
{
    void RenderPipelineResource::Reset()
    {
        memset(this, 0, sizeof(RenderPipelineResource));
    }

    void RenderPipelineResource::RequestGBuffers(RenderGraph* graph, uint32_t width, uint32_t height)
    {
        static int32 ids[NumGBuffers] =
        {
            ShaderUtils::GetIdFromString("_GBuffer0"),
            ShaderUtils::GetIdFromString("_GBuffer1"),
            ShaderUtils::GetIdFromString("_GBuffer2"),
            ShaderUtils::GetIdFromString("_GBuffer3"),
        };

        GfxTextureDesc desc{};
        //desc.Format = GfxTextureFormat::R8G8B8A8_UNorm;
        //desc.Flags = GfxTextureFlags::None;
        desc.Dimension = GfxTextureDimension::Tex2D;
        desc.Width = width;
        desc.Height = height;
        desc.DepthOrArraySize = 1;
        desc.MSAASamples = 1;
        desc.Filter = GfxTextureFilterMode::Bilinear;
        desc.Wrap = GfxTextureWrapMode::Clamp;
        desc.MipmapBias = 0.0f;

        for (size_t i = 0; i < NumGBuffers; i++)
        {
            desc.Format = GBufferFormats[i];
            desc.Flags = GBufferFlags[i];
            GBuffers[i] = graph->RequestTexture(ids[i], desc);
        }
    }

    InlineArray<TextureHandle, RenderPipelineResource::NumGBuffers> RenderPipelineResource::GetGBuffers(GBufferElements elements) const
    {
        InlineArray<TextureHandle, NumGBuffers> results{};

        for (size_t i = 0; i < NumGBuffers; i++)
        {
            if ((elements & GBufferData[i]) != GBufferElements::None)
            {
                results.Append(GBuffers[i]);
            }
        }

        return results;
    }
}
