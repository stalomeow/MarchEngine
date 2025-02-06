#ifndef _GBUFFER_INCLUDED
#define _GBUFFER_INCLUDED

#include "Common.hlsl"
#include "Packing.hlsl"

struct PixelGBufferOutput
{
    float4 GBuffer0 : SV_Target0; // albedo, shininess
    float4 GBuffer1 : SV_Target1; // normalWS, unused
    float4 GBuffer2 : SV_Target2; // fresnelR0, unused
    float4 GBuffer3 : SV_Target3; // depth, unused
};

Texture2D _GBuffer0;
Texture2D _GBuffer1;
Texture2D _GBuffer2;
Texture2D _GBuffer3;

struct GBufferData
{
    float3 albedo;
    float shininess;
    float3 normalWS;
    float depth;
    float3 fresnelR0;
};

PixelGBufferOutput PackGBufferData(GBufferData data)
{
    PixelGBufferOutput output;
    output.GBuffer0.xyz = data.albedo;
    output.GBuffer0.w = data.shininess;
    output.GBuffer1.xyz = PackFloat2To888(PackNormalOctQuadEncode(data.normalWS) * 0.5 + 0.5);
    output.GBuffer1.w = 0;
    output.GBuffer2.xyz = data.fresnelR0;
    output.GBuffer2.w = 0;
    output.GBuffer3.x = data.depth;
    output.GBuffer3.yzw = 0;
    return output;
}

GBufferData UnpackGBufferData(float4 gbuffer0, float4 gbuffer1, float4 gbuffer2, float4 gbuffer3)
{
    GBufferData data;
    data.albedo = gbuffer0.rgb;
    data.shininess = gbuffer0.a;
    data.normalWS = UnpackNormalOctQuadEncode(Unpack888ToFloat2(gbuffer1.rgb) * 2.0 - 1.0);
    data.fresnelR0 = gbuffer2.rgb;
    data.depth = gbuffer3.r;
    return data;
}

GBufferData LoadGBufferData(int2 location)
{
    float4 gbuffer0 = _GBuffer0.Load(int3(location, 0));
    float4 gbuffer1 = _GBuffer1.Load(int3(location, 0));
    float4 gbuffer2 = _GBuffer2.Load(int3(location, 0));
    float4 gbuffer3 = _GBuffer3.Load(int3(location, 0));
    return UnpackGBufferData(gbuffer0, gbuffer1, gbuffer2, gbuffer3);
}

GBufferData SampleGBufferData(float2 uv)
{
    float4 gbuffer0 = _GBuffer0.SampleLevel(sampler_PointClamp, uv, 0);
    float4 gbuffer1 = _GBuffer1.SampleLevel(sampler_PointClamp, uv, 0);
    float4 gbuffer2 = _GBuffer2.SampleLevel(sampler_PointClamp, uv, 0);
    float4 gbuffer3 = _GBuffer3.SampleLevel(sampler_PointClamp, uv, 0);
    return UnpackGBufferData(gbuffer0, gbuffer1, gbuffer2, gbuffer3);
}

#endif
