#ifndef _BRDF_INCLUDED
#define _BRDF_INCLUDED

#include "Common.hlsl"

struct BRDFData
{
    float3 f0;
    float3 diffuseBRDF; // Lambertian diffuse BRDF = albedo / PI
    float roughness;
    float a2; // a = roughness^2
};

float RoughnessToAlphaSquared(float roughness)
{
    float a = Square(roughness);
    return Square(a);
}

BRDFData GetBRDFData(float3 albedo, float metallic, float roughness)
{
    BRDFData data;
    data.f0 = lerp(0.04, albedo, metallic);
    data.diffuseBRDF = lerp(albedo, 0, metallic) / PI;
    data.roughness = roughness;
    data.a2 = RoughnessToAlphaSquared(roughness);
    return data;
}

float3 F_Schlick(float3 f0, float3 f90, float cosTheta)
{
    float x = 1.0 - cosTheta;
    float x2 = x * x;
    float x5 = x2 * x2 * x;
    return (f90 - f0) * x5 + f0;
}

float D_GGX(float a2, float NoH)
{
    float d = (NoH * a2 - NoH) * NoH + 1.0; // 2 mad
    return a2 / (PI * d * d);               // 4 mul, 1 rcp
}

float V_SmithJointGGX(float a2, float NoV, float NoL)
{
    float smithV = NoL * sqrt(NoV * (NoV - NoV * a2) + a2);
    float smithL = NoV * sqrt(NoL * (NoL - NoL * a2) + a2);
    return 0.5 * rcp(smithV + smithL + FLT_EPS); // 必须加 epsilon，避免除以 0
}

float3 DirectBRDF(BRDFData data, float NoV, float NoL, float NoH, float LoH)
{
    float3 F = F_Schlick(data.f0, 1.0, LoH);
    float3 diffuseTerm = (1.0 - F) * data.diffuseBRDF;
    float3 specularTerm = F * D_GGX(data.a2, NoH) * V_SmithJointGGX(data.a2, NoV, NoL);
    return diffuseTerm + specularTerm;
}

float3 EnvironmentDiffuseBRDF(BRDFData data, float NoV)
{
    float3 f90 = max(1.0 - data.roughness, data.f0);
    float3 F = F_Schlick(data.f0, f90, NoV);
    return (1.0 - F) * data.diffuseBRDF;
}

float3 EnvironmentSpecularBRDF(BRDFData data, float NoV, Texture2D lut, SamplerState samplerState)
{
    float2 uv = float2(NoV, data.roughness);
    float2 brdf = lut.SampleLevel(samplerState, uv, 0).rg;
    return data.f0 * brdf.x + brdf.y;
}

#endif // _BRDF_INCLUDED
