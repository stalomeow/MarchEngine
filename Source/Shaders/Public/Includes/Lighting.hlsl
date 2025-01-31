#ifndef _LIGHTING_INCLUDED
#define _LIGHTING_INCLUDED

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

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float3 BlinnPhong(float3 positionWS, float3 normalWS, float3 viewDirWS, float3 diffuseAlbedo, float3 fresnelR0, float shininess)
{
    float3 result = 0.4 * diffuseAlbedo;
    float m = shininess * 256.0;

    for (int i = 0; i < _LightCount; i++)
    {
        Light light = GetLight(positionWS, i);

        float3 H = normalize(light.direction + viewDirWS);
        float NdotL = max(0.01, dot(normalWS, light.direction));
        float NdotH = max(0.01, dot(normalWS, H));

        float roughnessFactor = pow(NdotH, m) * (m + 8.0) / 8.0;
        float3 fresnelFactor = SchlickFresnel(fresnelR0, H, light.direction);
        float3 specularAlbedo = roughnessFactor * fresnelFactor;

        // Our spec formula goes outside [0,1] range, but we are
        // doing LDR rendering.  So scale it down a bit.
        specularAlbedo /= (specularAlbedo + 1.0);

        result += (diffuseAlbedo + specularAlbedo) * light.color * light.attenuation * NdotL;
    }

    return result;
}

#endif
