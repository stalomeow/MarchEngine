#ifndef _GBUFFER_INCLUDED
#define _GBUFFER_INCLUDED

#include "Common.hlsl"

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

GBufferData LoadGBufferData(int3 location)
{
    float4 gbuffer0 = _GBuffer0.Load(location);
    float4 gbuffer1 = _GBuffer1.Load(location);
    float4 gbuffer2 = _GBuffer2.Load(location);
    float4 gbuffer3 = _GBuffer3.Load(location);

    GBufferData data;
    data.albedo = gbuffer0.rgb;
    data.shininess = gbuffer0.a;
    data.normalWS = gbuffer1.rgb * 2.0 - 1.0;
    data.fresnelR0 = gbuffer2.rgb;
    data.depth = gbuffer3.r;
    return data;
}

#endif
