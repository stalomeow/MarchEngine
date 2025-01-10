#ifndef _SHADOW_INCLUDED
#define _SHADOW_INCLUDED

cbuffer cbShadow
{
    float4x4 _ShadowMatrix;
};

Texture2D _ShadowMap;
SamplerComparisonState sampler_ShadowMap;

float4 TransformWorldToShadowCoord(float3 positionWS)
{
    return mul(_ShadowMatrix, float4(positionWS, 1));
}

float SampleShadowMap(float4 shadowCoord)
{
    // assume orthographic projection
    // shadowCoord /= shadowCoord.w;

    return _ShadowMap.SampleCmpLevelZero(sampler_ShadowMap, shadowCoord.xy, shadowCoord.z).r;
}

Texture2D _ScreenSpaceShadowMap;
SamplerState sampler_ScreenSpaceShadowMap;

float SampleScreenSpaceShadowMap(float2 uv)
{
    return _ScreenSpaceShadowMap.Sample(sampler_ScreenSpaceShadowMap, uv).r;
}

#endif
