#ifndef _COMMON_INCLUDED
#define _COMMON_INCLUDED

#include "Sampler.hlsl"

struct InstanceData
{
    float4x4 _MatrixWorld;
    float4x4 _MatrixWorldIT; // 逆转置
};

StructuredBuffer<InstanceData> _InstanceBuffer;

cbuffer cbCamera
{
    float4x4 _MatrixView;
    float4x4 _MatrixProjection;
    float4x4 _MatrixViewProjection;
    float4x4 _MatrixInvView;
    float4x4 _MatrixInvProjection;
    float4x4 _MatrixInvViewProjection;
};

float Square(float x)
{
    return x * x;
}

float4x4 GetObjectToWorldMatrix(uint instanceID)
{
    return _InstanceBuffer[instanceID]._MatrixWorld;
}

float4x4 GetObjectToWorldMatrixIT(uint instanceID)
{
    return _InstanceBuffer[instanceID]._MatrixWorldIT;
}

float3 TransformObjectToWorld(uint instanceID, float3 positionOS)
{
    return mul(GetObjectToWorldMatrix(instanceID), float4(positionOS, 1.0)).xyz;
}

float3 TransformObjectToWorldDir(uint instanceID, float3 dirOS)
{
    // Normalize to support uniform scaling
    return normalize(mul((float3x3) GetObjectToWorldMatrix(instanceID), dirOS));
}

float3 TransformWorldToView(float3 positionWS)
{
    return mul(_MatrixView, float4(positionWS, 1.0)).xyz;
}

float3 TransformWorldToViewDir(float3 dirWS)
{
    // Normalize to support uniform scaling
    return normalize(mul((float3x3) _MatrixView, dirWS));
}

float4 TransformViewToHClip(float3 positionVS)
{
    return mul(_MatrixProjection, float4(positionVS, 1.0));
}

float4 TransformWorldToHClip(float3 positionWS)
{
    return mul(_MatrixViewProjection, float4(positionWS, 1.0));
}

float3 TransformObjectToWorldNormal(uint instanceID, float3 normalOS)
{
    return normalize(mul((float3x3) GetObjectToWorldMatrixIT(instanceID), normalOS));
}

float3 TransformWorldToViewNormal(float3 normalWS)
{
    // 需要使用逆转置矩阵
    // Tips: 把向量当成行向量可以省去转置
    return normalize(mul(normalWS, (float3x3) _MatrixInvView));
}

float2 GetFullScreenTriangleTexCoord(uint vertexID)
{
    // https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl
    // 我们 uv starts at top
    return float2(vertexID & 2, 1.0 - ((vertexID << 1) & 2));
}

float4 GetFullScreenTriangleVertexPositionCS(uint vertexID, float z = MARCH_NEAR_CLIP_VALUE)
{
    // https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl
    // note: the triangle vertex position coordinates are x2 so the returned UV coordinates are in range -1, 1 on the screen.
    // 我们顺时针为正面，和 unity 相反
    float2 uv = float2(vertexID & 2, ((vertexID << 1) & 2));
    return float4(uv * 2.0 - 1.0, z, 1.0);
}

float3 ComputeWorldSpacePosition(float2 screenUV, float ndcDepth)
{
    // screen uv 原点在左上角，xy 范围是 [0, 1]，y 轴朝下
    // ndc 原点在中心，xy 范围是 [-1, 1]，z 范围是 [0, 1]，y 轴朝上

    float4 ndc = float4(screenUV.x, 1 - screenUV.y, ndcDepth, 1);
    ndc.xy = ndc.xy * 2 - 1;

    float4 positionWS = mul(_MatrixInvViewProjection, ndc);
    return positionWS.xyz / positionWS.w;
}

float3 ComputeViewSpacePosition(float2 screenUV, float ndcDepth)
{
    // screen uv 原点在左上角，xy 范围是 [0, 1]，y 轴朝下
    // ndc 原点在中心，xy 范围是 [-1, 1]，z 范围是 [0, 1]，y 轴朝上

    float4 ndc = float4(screenUV.x, 1 - screenUV.y, ndcDepth, 1);
    ndc.xy = ndc.xy * 2 - 1;

    float4 positionVS = mul(_MatrixInvProjection, ndc);
    return positionVS.xyz / positionVS.w;
}

float GetCameraNearZ()
{
#if MARCH_REVERSED_Z
    return _MatrixProjection[2][3] / (1 - _MatrixProjection[2][2]);
#else
    return _MatrixProjection[2][3] / -_MatrixProjection[2][2];
#endif
}

float GetCameraFarZ()
{
#if MARCH_REVERSED_Z
    return _MatrixProjection[2][3] / -_MatrixProjection[2][2];
#else
    return _MatrixProjection[2][3] / (1 - _MatrixProjection[2][2]);
#endif
}

float3 GetCameraWorldSpacePosition()
{
    return _MatrixInvView._m03_m13_m23;
}

// TODO not tested
float GetLinearEyeDepth(float ndcDepth)
{
    // ndcDepth = A + B / viewZ
    // A = Proj[2][2]; B = Proj[2][3]
    return _MatrixProjection[2][3] / (ndcDepth - _MatrixProjection[2][2]);
}

// 三个点必须是顺时针排列，左手定则
float3 GetPlaneNormal(float3 a, float3 b, float3 c)
{
    return normalize(cross(b - a, c - a));
}

// 法线必须是归一化的，a 是平面上任意一点
float4 GetPlane(float3 normal, float3 a)
{
    return float4(normal, -dot(normal, a));
}

// 三个点必须是顺时针排列，左手定则
float4 GetPlane(float3 a, float3 b, float3 c)
{
    return GetPlane(GetPlaneNormal(a, b, c), a);
}

#endif
