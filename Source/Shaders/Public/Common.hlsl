#ifndef _COMMON_INCLUDED
#define _COMMON_INCLUDED

// 内置宏
// MARCH_REVERSED_Z         在开启 Reversed-Z Buffer 时定义
// MARCH_NEAR_CLIP_VALUE    近裁剪平面的深度
// MARCH_FAR_CLIP_VALUE     远裁剪平面的深度

cbuffer cbCamera : register(b1, space2)
{
    float4x4 _MatrixView;
    float4x4 _MatrixProjection;
    float4x4 _MatrixViewProjection;
    float4x4 _MatrixInvView;
    float4x4 _MatrixInvProjection;
    float4x4 _MatrixInvViewProjection;
    float4 _CameraPositionWS;
};

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
