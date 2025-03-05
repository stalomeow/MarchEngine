#ifndef _LIGHTING_INCLUDED
#define _LIGHTING_INCLUDED

#include "Common.hlsl"
#include "BRDF.hlsl"
#include "Shadow.hlsl"
#include "SH9.hlsl"

struct LightRawData
{
    float4 position;      // 位置（w==1, 点光源/聚光灯）；方向（w==0, 平行光）
    float4 spotDirection; // 聚光灯方向，w 未使用
    float4 color;         // 颜色，接收阴影（w==1）
    float4 params;        // AttenuationRadius, cos(SpotOuterConeAngle), rcp(cos(SpotInnerConeAngle)-cos(SpotOuterConeAngle)), IsSpotLight (w==1)
};

cbuffer cbLight
{
    int4 _NumLights;   // x: numDirectionalLights, y: numPunctualLights
    int4 _NumClusters; // xyz: numClusters
};

StructuredBuffer<LightRawData> _DirectionalLights;
StructuredBuffer<LightRawData> _PunctualLights;

#ifdef LIGHT_CULLING
    RWStructuredBuffer<int2> _ClusterPunctualLightRanges;
    RWStructuredBuffer<int> _ClusterPunctualLightIndices;
#else
    StructuredBuffer<int2> _ClusterPunctualLightRanges;
    StructuredBuffer<int> _ClusterPunctualLightIndices;
#endif

int GetNumDirectionalLights()
{
    return _NumLights.x;
}

int GetNumPunctualLights()
{
    return _NumLights.y;
}

int GetClusterIndex(int3 id)
{
    // return id.x + id.y * _NumClusters.x + id.z * _NumClusters.x * _NumClusters.y;
    // 优化为 2 mad
    return id.x + (id.y + id.z * _NumClusters.y) * _NumClusters.x;
}

int3 GetClusterId(float2 uv, float3 positionVS)
{
    float near = GetCameraNearZ();
    float far = GetCameraFarZ();
    float zSlice = log(positionVS.z / near) / log(far / near);
    return int3(uv.xy * _NumClusters.xy, zSlice * _NumClusters.z);
}

void GetClusterViewSpaceZRange(int3 id, out float zMin, out float zMax)
{
    float numRcp = rcp((float) _NumClusters.z);
    float begin = id.z * numRcp;

    float near = GetCameraNearZ();
    float far = GetCameraFarZ();
    float fn = far / near;

    zMin = near * pow(fn, begin);
    zMax = zMin * pow(fn, numRcp);
}

float3 LineIntersectionWithZPlane(float3 direction, float z)
{
    return direction * (z / direction.z);
}

struct ClusterFrustum
{
    float4 planes[6];
    float4 sphere; // xyz: center, w: radius
};

ClusterFrustum GetClusterViewSpaceFrustum(int3 id)
{
    float zMin, zMax;
    GetClusterViewSpaceZRange(id, zMin, zMax);

    float2 numRcp = rcp((float2) _NumClusters.xy);
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
    float3 p6 = LineIntersectionWithZPlane(dir4, zMin);
    float3 p7 = LineIntersectionWithZPlane(dir4, zMax);

    ClusterFrustum result;

    result.planes[0] = GetPlane(float3(0, 0, 1), p0); // near plane
    result.planes[1] = GetPlane(float3(0, 0, -1), p7); // far plane
    result.planes[2] = GetPlane(p0, p2, p1); // left plane
    result.planes[3] = GetPlane(p4, p7, p6); // right plane
    result.planes[4] = GetPlane(p2, p6, p7); // top plane
    result.planes[5] = GetPlane(p0, p1, p4); // bottom plane

    float3 aabbMin = min(p0, p1);
    float3 aabbMax = max(p6, p7);
    result.sphere.xyz = (aabbMin + aabbMax) * 0.5;
    result.sphere.w = length(aabbMax - result.sphere.xyz);

    return result;
}

struct Light
{
    float3 color;
    float3 direction;
    float attenuation;
};

Light GetLight(float3 positionWS, LightRawData data, float shadow)
{
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

float3 GetDebugClusterIdView(int3 clusterId)
{
    return clusterId / float3(_NumClusters.xyz - 1);
}

float GetDebugClusterIndexView(int clusterIndex)
{
    return clusterIndex / (float) (_NumClusters.x * _NumClusters.y * _NumClusters.z - 1);
}

float GetDebugClusterViewSpaceFrustumView(int3 clusterId, int frustumPlaneIndex, float3 positionVS)
{
    ClusterFrustum f = GetClusterViewSpaceFrustum(clusterId);
    return abs(dot(f.planes[frustumPlaneIndex], float4(positionVS, 1.0)));
}

float GetDebugClusterNumLightsView(int numLights)
{
    return numLights / 16.0;
}

float3 DirectLighting(Light light, BRDFData data, float3 N, float3 V, float NoV)
{
    float3 L = light.direction;
    float3 H = normalize(L + V);
    float NoL = saturate(dot(N, L));
    float NoH = saturate(dot(N, H));
    float LoH = saturate(dot(L, H));

    float3 brdf = DirectBRDF(data, NoV, NoL, NoH, LoH);
    float3 irradiance = light.color * (light.attenuation * NoL);
    return brdf * irradiance;
}

float3 FragmentPBR(BRDFData data, float3 positionWS, float3 normalWS, float3 positionVS, float2 uv, float3 emission, float occlusion)
{
    int3 clusterId = GetClusterId(uv, positionVS);
    int clusterIndex = GetClusterIndex(clusterId);
    int2 punctualLightRange = _ClusterPunctualLightRanges[clusterIndex];

    //return GetDebugClusterIdView(clusterId);
    //return GetDebugClusterIndexView(clusterIndex);
    //return GetDebugClusterViewSpaceFrustumView(clusterId, 4, positionVS);
    //return GetDebugClusterNumLightsView(punctualLightRange.y);

    float3 result = emission;

    float3 N = normalWS;
    float3 V = normalize(GetCameraWorldSpacePosition() - positionWS);
    float NoV = saturate(dot(N, V));
    float shadow = SampleShadowMap(TransformWorldToShadowCoord(positionWS));

    // Directional lights
    for (int i = 0; i < GetNumDirectionalLights(); i++)
    {
        Light light = GetLight(positionWS, _DirectionalLights[i], shadow);
        result += DirectLighting(light, data, N, V, NoV);
    }

    // Punctual lights
    for (int i = 0; i < punctualLightRange.y; i++)
    {
        int punctualLightIndex = _ClusterPunctualLightIndices[punctualLightRange.x + i];
        Light light = GetLight(positionWS, _PunctualLights[punctualLightIndex], shadow);
        result += DirectLighting(light, data, N, V, NoV);
    }

    // Environment
    result += EnvironmentDiffuseBRDF(data, NoV) * SampleSH9(N) * occlusion;

    return result;
}

#endif
