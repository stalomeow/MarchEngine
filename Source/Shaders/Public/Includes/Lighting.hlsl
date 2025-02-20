#ifndef _LIGHTING_INCLUDED
#define _LIGHTING_INCLUDED

#include "BRDF.hlsl"

#define MAX_LIGHT_COUNT 16

struct LightData
{
    float4 position;
    float4 spotDirection;
    float4 color;
    float4 falloff;
};

struct Light
{
    float3 color;
    float3 direction;
    float attenuation;
};

cbuffer cbLight
{
    LightData _LightData[MAX_LIGHT_COUNT];
    int _LightCount;
};

Light GetLight(float3 positionWS, int i)
{
    LightData data = _LightData[i];

    float3 direction = data.position.xyz - positionWS * data.position.w;
    float dist = length(direction);
    direction /= dist; // normalize

    float distAtten = saturate((data.falloff.y - dist) / (data.falloff.y - data.falloff.x));
    float spotAngleAtten = pow(max(dot(data.spotDirection.xyz, direction), 0.01), data.spotDirection.w);

    Light light;
    light.color = data.color.rgb;
    light.direction = direction;
    light.attenuation = lerp(1.0, distAtten, data.position.w) * spotAngleAtten;
    return light;
}

float3 FragmentPBR(BRDFData data, float3 positionWS, float3 normalWS, float3 viewDirWS, float ao)
{
    float3 result = ao * 0.4 * data.albedo;

    float3 N = normalWS;
    float3 V = viewDirWS;
    float NoV = saturate(dot(N, V));

    for (int i = 0; i < _LightCount; i++)
    {
        Light light = GetLight(positionWS, i);

        float3 L = light.direction;
        float3 H = normalize(L + V);
        float NoL = saturate(dot(N, L));
        float NoH = saturate(dot(N, H));
        float LoH = saturate(dot(L, H));

        float3 brdf = DiffuseSpecularBRDF(data, NoV, NoL, NoH, LoH);
        float3 irradiance = light.color * (light.attenuation * NoL);
        result += brdf * irradiance;
    }

    return result;
}

#endif
