#ifndef _LIGHTING_INCLUDED
#define _LIGHTING_INCLUDED

#include "Common.hlsl"
#include "BRDF.hlsl"
#include "Shadow.hlsl"

#define NUM_MAX_LIGHT_IN_CLUSTER 16

struct LightData
{
    float4 position;      // 位置（w==1, 点光源/聚光灯）；方向（w==0, 平行光）
    float4 spotDirection; // 聚光灯方向，w 未使用
    float4 color;         // 颜色，接收阴影（w==1）
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
    int4 _Nums; // xyz: numClusters, w: numLights
};

StructuredBuffer<LightData> _Lights;

#ifdef LIGHT_CULLING
    RWStructuredBuffer<int> _NumVisibleLights;
    RWStructuredBuffer<int> _VisibleLightIndices;
#else
    StructuredBuffer<int> _NumVisibleLights;
    StructuredBuffer<int> _VisibleLightIndices;
#endif

int GetClusterIndex(int3 id)
{
    // return id.x + id.y * _Nums.x + id.z * _Nums.x * _Nums.y;
    // 优化为 2 mad
    return id.x + (id.y + id.z * _Nums.y) * _Nums.x;
}

int GetClusterVisibleLightIndicesIndex(int clusterIndex, int lightIndex)
{
    return clusterIndex * NUM_MAX_LIGHT_IN_CLUSTER + lightIndex;
}

void GetClusterZRange(int3 id, out float zMin, out float zMax)
{
    #if MARCH_REVERSED_Z
        float near = _MatrixProjection[2][3] / (1 - _MatrixProjection[2][2]);
        float far = _MatrixProjection[2][3] / -_MatrixProjection[2][2];
    #else
        float near = _MatrixProjection[2][3] / -_MatrixProjection[2][2];
        float far = _MatrixProjection[2][3] / (1 - _MatrixProjection[2][2]);
    #endif

    float fn = far / near;

    float numRcp = rcp((float) _Nums.z);
    float begin = id.z * numRcp;

    zMin = near * pow(fn, begin);
    zMax = near * pow(fn, begin + numRcp);
}

float3 LineIntersectionWithZPlane(float3 direction, float z)
{
    return direction * (z / direction.z);
}

float4 GetPlane(float3 normal, float3 a)
{
    return float4(normal, -dot(normal, a));
}

float4 GetPlane(float3 a, float3 b, float3 c)
{
    float3 normal = normalize(cross(b - a, c - a));
    return GetPlane(normal, a);
}

void GetClusterPlanes(int3 id, out float4 planes[6])
{
    float zMin, zMax;
    GetClusterZRange(id, zMin, zMax);

    float2 numRcp = rcp((float2) _Nums.xy);
    float4 uv = float4(id.xy * numRcp, 0, 0); // xy: uvMin
    uv.zw = uv.xy + numRcp; // zw: uvMax

    float3 dir1 = ComputeViewSpacePosition(uv.xw, MARCH_NEAR_CLIP_VALUE); // minX, maxY
    float3 dir2 = ComputeViewSpacePosition(uv.xy, MARCH_NEAR_CLIP_VALUE); // minX, minY
    float3 dir3 = ComputeViewSpacePosition(uv.zw, MARCH_NEAR_CLIP_VALUE); // maxX, maxY
    float3 dir4 = ComputeViewSpacePosition(uv.zy, MARCH_NEAR_CLIP_VALUE); // maxX, minY

    float3 p0 = LineIntersectionWithZPlane(dir1, zMin);
    float3 p1 = LineIntersectionWithZPlane(dir1, zMax);
    float3 p2 = LineIntersectionWithZPlane(dir2, zMin);
    float3 p4 = LineIntersectionWithZPlane(dir3, zMin);
    float3 p5 = LineIntersectionWithZPlane(dir3, zMax);
    float3 p6 = LineIntersectionWithZPlane(dir4, zMin);
    float3 p7 = LineIntersectionWithZPlane(dir4, zMax);

    planes[0] = GetPlane(p0, p4, p6); // near plane
    planes[1] = GetPlane(p1, p7, p5); // far plane
    planes[2] = GetPlane(p0, p2, p1); // left plane
    planes[3] = GetPlane(p4, p7, p6); // right plane
    planes[4] = GetPlane(p2, p6, p7); // top plane
    planes[5] = GetPlane(p0, p1, p4); // bottom plane
}

int3 GetClusterId(float2 uv, float3 positionVS)
{
    #if MARCH_REVERSED_Z
        float near = _MatrixProjection[2][3] / (1 - _MatrixProjection[2][2]);
        float far = _MatrixProjection[2][3] / -_MatrixProjection[2][2];
    #else
        float near = _MatrixProjection[2][3] / -_MatrixProjection[2][2];
        float far = _MatrixProjection[2][3] / (1 - _MatrixProjection[2][2]);
    #endif

    int z = int(log(positionVS.z / near) / log(far / near) * _Nums.z);
    return int3(uv.xy * _Nums.xy, z);
}

float Square(float x)
{
    return x * x;
}

Light GetLight(float3 positionWS, int clusterIndex, int lightIndex, float shadow)
{
    LightData data = _Lights[_VisibleLightIndices[GetClusterVisibleLightIndicesIndex(clusterIndex, lightIndex)]];

    float3 direction = data.position.xyz - positionWS * data.position.w;
    float distSq = dot(direction, direction);
    direction *= rsqrt(distSq); // normalize

    float attenRadiusSq = Square(data.params.x);
    float distAtten = Square(saturate(1 - Square(distSq / attenRadiusSq))) / (distSq + 1.0);
    distAtten = lerp(1.0, distAtten, data.position.w);

    float spotAngleMask = saturate((dot(data.spotDirection.xyz, direction) - data.params.y) * data.params.z);
    float spotAngleAtten = Square(spotAngleMask);
    spotAngleAtten = lerp(1.0, spotAngleAtten, data.params.w);

    float shadowAtten = lerp(1.0, shadow, data.color.w);

    Light light;
    light.color = data.color.rgb;
    light.direction = direction;
    light.attenuation = distAtten * spotAngleAtten * shadowAtten;
    return light;
}

float3 FragmentPBR(BRDFData data, float3 positionWS, float3 normalWS, float3 positionVS, float2 uv, float3 emission, float occlusion)
{
    float3 result = emission + (occlusion * (0.03 * data.albedo));

    float3 N = normalWS;
    float3 V = normalize(_CameraPositionWS.xyz - positionWS);
    float NoV = saturate(dot(N, V));

    float shadow = SampleShadowMap(TransformWorldToShadowCoord(positionWS));

    int3 clusterId = GetClusterId(uv, positionVS);
    int clusterIndex = GetClusterIndex(clusterId);
    //return clusterIndex / (float) (_Nums.x * _Nums.y * _Nums.z - 1);
    //return clusterId.xyz / float3(_Nums.xyz - 1);
    //return clusterId.zzz / float3(_Nums.zzz - 1);
    //float zMin, zMax;
    //GetClusterZRange(clusterId, zMin, zMax);
    //return (zMax - zMin) / 500;
    //return _NumVisibleLights[clusterIndex] / (float) NUM_MAX_LIGHT_IN_CLUSTER;

    //float4 planes[6];
    //GetClusterPlanes(clusterId, planes);
    ////return planes[4].xyz;
    //return abs(dot(planes[5].xyz, positionVS));

    for (int i = 0; i < _NumVisibleLights[clusterIndex]; i++)
    {
        Light light = GetLight(positionWS, clusterIndex, i, shadow);

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
