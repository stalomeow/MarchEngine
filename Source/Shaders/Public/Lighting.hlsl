#ifndef _LIGHTING_INCLUDED
#define _LIGHTING_INCLUDED

#define MAX_LIGHT_COUNT 16

#define TEXTURE_SAMPLER(texture) SamplerState sampler##texture

struct LightData
{
    float4 position;      // λ�ã�w==1, ���Դ/�۹�ƣ�������w==0, ƽ�й⣩
    float4 spotDirection; // �۹�Ʒ���w Ϊ�۹�ǿ�ȣ�0 ��ʾ�Ǿ۹�ƣ�
    float4 color;         // ��ɫ��w δʹ��
    float4 falloff;       // ˥������ʼ�ͽ������루���Դ/�۹�ƣ���zw δʹ��
};

struct Light
{
    float3 color;
    float3 direction;
    float attenuation;
};

void GetLights(float3 positionWS, LightData lighData[MAX_LIGHT_COUNT], int lightCount, out Light outLights[MAX_LIGHT_COUNT])
{
    for (int i = 0; i < lightCount; i++)
    {
        float3 direction = lighData[i].position.xyz - positionWS * lighData[i].position.w;
        float dist = length(direction);
        direction /= dist; // normalize

        float distAtten = saturate((lighData[i].falloff.y - dist) / (lighData[i].falloff.y - lighData[i].falloff.x));
        float spotAngleAtten = pow(max(dot(lighData[i].spotDirection.xyz, direction), 0.01), lighData[i].spotDirection.w);

        outLights[i].color = lighData[i].color.rgb;
        outLights[i].direction = direction;
        outLights[i].attenuation = lerp(1.0, distAtten, lighData[i].position.w) * spotAngleAtten;
    }
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

float3 BlinnPhong(Light light, float3 normalWS, float3 viewDirWS, float3 diffuseAlbedo, float3 fresnelR0, float shininess)
{
    float3 H = normalize(light.direction + viewDirWS);
    float NdotL = max(0.01, dot(normalWS, light.direction));
    float NdotH = max(0.01, dot(normalWS, H));

    float m = shininess * 256.0;
    float roughnessFactor = pow(NdotH, m) * (m + 8.0) / 8.0;
    float3 fresnelFactor = SchlickFresnel(fresnelR0, H, light.direction);
    float3 specularAlbedo = roughnessFactor * fresnelFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specularAlbedo /= (specularAlbedo + 1.0);

    return (diffuseAlbedo + specularAlbedo) * light.color * light.attenuation * NdotL;
}

#endif
