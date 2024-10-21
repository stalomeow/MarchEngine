#ifndef _COMMON_INCLUDED
#define _COMMON_INCLUDED

// 内置宏
// MARCH_REVERSED_Z         在开启 Reversed-Z Buffer 时定义
// MARCH_NEAR_CLIP_VALUE    近裁剪平面的深度
// MARCH_FAR_CLIP_VALUE     远裁剪平面的深度

#define SAMPLER(texture) SamplerState sampler##texture

cbuffer cbObject
{
    float4x4 _MatrixWorld;
};

cbuffer cbCamera
{
    float4x4 _MatrixView;
    float4x4 _MatrixProjection;
    float4x4 _MatrixViewProjection;
    float4x4 _MatrixInvView;
    float4x4 _MatrixInvProjection;
    float4x4 _MatrixInvViewProjection;
    float4 _CameraPositionWS;
};

float3 TransformObjectToWorld(float3 positionOS)
{
    return mul(_MatrixWorld, float4(positionOS, 1.0)).xyz;
}

float3 TransformObjectToWorldDir(float3 dirOS)
{
    // Normalize to support uniform scaling
    return normalize(mul((float3x3) _MatrixWorld, dirOS));
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

float3 TransformObjectToWorldNormal(float3 normalOS)
{
    // assume uniform scale
    return normalize(mul((float3x3) _MatrixWorld, normalOS));
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

#endif
