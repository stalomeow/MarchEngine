#ifndef _GBUFFER_INCLUDED
#define _GBUFFER_INCLUDED

#include "Common.hlsl"
#include "Packing.hlsl"

struct PixelGBufferOutput
{
    float4 GBuffer0 : SV_Target0; // albedo, metallic
    float4 GBuffer1 : SV_Target1; // normalWS, roughness
    float4 GBuffer2 : SV_Target2; // depth, unused
};

Texture2D _GBuffer0;
Texture2D _GBuffer1;
Texture2D _GBuffer2;

struct GBufferData
{
    float3 albedo;
    float metallic;
    float roughness;
    float3 normalWS;
    float depth;
};

PixelGBufferOutput PackGBufferData(GBufferData data)
{
    PixelGBufferOutput output;
    output.GBuffer0.rgb = data.albedo;
    output.GBuffer0.a = data.metallic;
    output.GBuffer1.rgb = PackFloat2To888(PackNormalOctQuadEncode(data.normalWS) * 0.5 + 0.5);
    output.GBuffer1.a = data.roughness;
    output.GBuffer2.r = data.depth;
    output.GBuffer2.gba = 0;
    return output;
}

GBufferData UnpackGBufferData(float4 gbuffer0, float4 gbuffer1, float4 gbuffer2)
{
    GBufferData data;
    data.albedo = gbuffer0.rgb;
    data.metallic = gbuffer0.a;
    data.normalWS = UnpackNormalOctQuadEncode(Unpack888ToFloat2(gbuffer1.rgb) * 2.0 - 1.0);
    data.roughness = gbuffer1.a;
    data.depth = gbuffer2.r;
    return data;
}

GBufferData LoadGBufferData(int2 location)
{
    float4 gbuffer0 = _GBuffer0.Load(int3(location, 0));
    float4 gbuffer1 = _GBuffer1.Load(int3(location, 0));
    float4 gbuffer2 = _GBuffer2.Load(int3(location, 0));
    return UnpackGBufferData(gbuffer0, gbuffer1, gbuffer2);
}

GBufferData SampleGBufferData(float2 uv)
{
    float4 gbuffer0 = _GBuffer0.SampleLevel(sampler_PointClamp, uv, 0);
    float4 gbuffer1 = _GBuffer1.SampleLevel(sampler_PointClamp, uv, 0);
    float4 gbuffer2 = _GBuffer2.SampleLevel(sampler_PointClamp, uv, 0);
    return UnpackGBufferData(gbuffer0, gbuffer1, gbuffer2);
}

#endif
