#ifndef _BRDF_INCLUDED
#define _BRDF_INCLUDED

#define PI 3.14159265

struct BRDFData
{
    float3 albedo;
    float metallic;
    float a2;
};

float RoughnessToAlpha2(float roughness)
{
    float a = roughness * roughness;
    return a * a;
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
    return 0.5 * rcp(smithV + smithL);
}

float3 DiffuseSpecularBRDF(BRDFData data, float NoV, float NoL, float NoH, float LoH)
{
    float3 f0 = lerp(0.04, data.albedo, data.metallic);
    float3 F = F_Schlick(f0, 1.0, LoH);

    float3 diffuseAlbedo = lerp(data.albedo, 0, data.metallic);
    float3 diffuseTerm = (1.0 - F) * diffuseAlbedo / PI;
    float3 specularTerm = F * D_GGX(data.a2, NoH) * V_SmithJointGGX(data.a2, NoV, NoL);
    return diffuseTerm + specularTerm;
}

#endif // _BRDF_INCLUDED
