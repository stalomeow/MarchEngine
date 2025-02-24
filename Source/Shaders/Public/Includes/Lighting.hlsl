#ifndef _LIGHTING_INCLUDED
#define _LIGHTING_INCLUDED

#include "BRDF.hlsl"

#define MAX_LIGHT_COUNT 16

struct LightData
{
    float4 position;      // 位置（w==1, 点光源/聚光灯）；方向（w==0, 平行光）
    float4 spotDirection; // 聚光灯方向，w 未使用
    float4 color;         // 颜色，w 未使用
    float4 params;        // AttenuationRadius, cos(SpotOuterConeAngle), rcp(cos(SpotInnerConeAngle)-cos(SpotOuterConeAngle)), IsSpotLight (w==1)
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

float Square(float x)
{
    return x * x;
}

Light GetLight(float3 positionWS, int i)
{
    LightData data = _LightData[i];

    float3 direction = data.position.xyz - positionWS * data.position.w;
    float distSq = dot(direction, direction);
    direction *= rsqrt(distSq); // normalize

    float attenRadiusSq = Square(data.params.x);
    float distAtten = Square(saturate(1 - Square(distSq / attenRadiusSq))) / (distSq + 1.0);
    distAtten = lerp(1.0, distAtten, data.position.w);

    float spotAngleMask = saturate((dot(data.spotDirection.xyz, direction) - data.params.y) * data.params.z);
    float spotAngleAtten = Square(spotAngleMask);
    spotAngleAtten = lerp(1.0, spotAngleAtten, data.params.w);

    Light light;
    light.color = data.color.rgb;
    light.direction = direction;
    light.attenuation = distAtten * spotAngleAtten;
    return light;
}

float3 FragmentPBR(BRDFData data, float3 positionWS, float3 normalWS, float3 viewDirWS, float3 emission, float occlusion)
{
    float3 result = emission + (occlusion * (0.03 * data.albedo));

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
