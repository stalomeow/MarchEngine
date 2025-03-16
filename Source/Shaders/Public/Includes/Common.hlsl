#ifndef _COMMON_INCLUDED
#define _COMMON_INCLUDED

#define PI      3.14159265

#define FLT_INF asfloat(0x7F800000)
#define FLT_NAN asfloat(0xffc00000)
#define FLT_EPS 5.960464478e-8  // 2^-24, machine epsilon: 1 + EPS = 1 (half of the ULP for 1.0f)
#define FLT_MIN 1.175494351e-38 // Minimum normalized positive floating-point number
#define FLT_MAX 3.402823466e+38 // Maximum representable floating-point number

#define HALF_EPS 4.8828125e-4    // 2^-11, machine epsilon: 1 + EPS = 1 (half of the ULP for 1.0f)
#define HALF_MIN 6.103515625e-5  // 2^-14, the same value for 10, 11 and 16-bit: https://www.khronos.org/opengl/wiki/Small_Float_Formats
#define HALF_MAX 65504.0

struct InstanceData
{
    float4x4 _MatrixWorld;
    float4x4 _MatrixWorldIT; // 逆转置
    float4x4 _MatrixPrevWorld; // 上一帧的矩阵
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
    float4x4 _MatrixNonJitteredViewProjection; // non-jittered
    float4x4 _MatrixPrevNonJitteredViewProjection; // 上一帧的 non-jittered
    float4 _TAAParams; // x: TAAFrameIndex
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

float4x4 GetObjectToWorldMatrixLastFrame(uint instanceID)
{
    return _InstanceBuffer[instanceID]._MatrixPrevWorld;
}

float3 TransformObjectToWorld(uint instanceID, float3 positionOS)
{
    return mul(GetObjectToWorldMatrix(instanceID), float4(positionOS, 1.0)).xyz;
}

float3 TransformObjectToWorldLastFrame(uint instanceID, float3 positionOS)
{
    return mul(GetObjectToWorldMatrixLastFrame(instanceID), float4(positionOS, 1.0)).xyz;
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

float4 TransformWorldToHClipNonJittered(float3 positionWS)
{
    return mul(_MatrixNonJitteredViewProjection, float4(positionWS, 1.0));
}

float4 TransformWorldToHClipNonJitteredLastFrame(float3 positionWS)
{
    return mul(_MatrixPrevNonJitteredViewProjection, float4(positionWS, 1.0));
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

float3 GetNDCFromScreenUV(float2 uv, float ndcDepth)
{
    // screen uv 原点在左上角，xy 范围是 [0, 1]，y 轴朝下
    // ndc 原点在中心，xy 范围是 [-1, 1]，z 范围是 [0, 1]，y 轴朝上
    uv.y = 1.0 - uv.y;
    return float3(uv * 2.0 - 1.0, ndcDepth);
}

float2 GetScreenUVFromNDC(float2 positionNDC)
{
    // screen uv 原点在左上角，xy 范围是 [0, 1]，y 轴朝下
    // ndc 原点在中心，xy 范围是 [-1, 1]，z 范围是 [0, 1]，y 轴朝上
    float2 uv = positionNDC.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

float3 ComputeWorldSpacePosition(float2 screenUV, float ndcDepth)
{
    float3 ndc = GetNDCFromScreenUV(screenUV, ndcDepth);
    float4 positionWS = mul(_MatrixInvViewProjection, float4(ndc, 1.0));
    return positionWS.xyz / positionWS.w;
}

float3 ComputeViewSpacePosition(float2 screenUV, float ndcDepth)
{
    float3 ndc = GetNDCFromScreenUV(screenUV, ndcDepth);
    float4 positionVS = mul(_MatrixInvProjection, float4(ndc, 1.0));
    return positionVS.xyz / positionVS.w;
}

float2 GetUVFromTexelLocation(uint2 location, float width, float height)
{
    // 加 0.5 放到纹素中心
    return (location + 0.5) / float2(width, height);
}

uint2 GetScreenPositionFromSVPositionInput(float4 svPositionInput)
{
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics#direct3d-9-vpos-and-direct3d-10-sv_position
    // In Direct3D 10 and later, the SV_Position semantic (when used in the context of a pixel shader) specifies screen space coordinates (offset by 0.5).
    return uint2(svPositionInput.xy);
}

float2 GetScreenUVFromSVPositionInput(float4 svPositionInput, float screenWidth, float screenHeight)
{
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics#direct3d-9-vpos-and-direct3d-10-sv_position
    // In Direct3D 10 and later, the SV_Position semantic (when used in the context of a pixel shader) specifies screen space coordinates (offset by 0.5).
    return svPositionInput.xy / float2(screenWidth, screenHeight);
}

float GetNDCDepthFromSVPositionInput(float4 svPositionInput)
{
    // https://zhuanlan.zhihu.com/p/597918725
    return svPositionInput.z;
}

float GetViewSpaceZFromSVPositionInput(float4 svPositionInput)
{
    // https://zhuanlan.zhihu.com/p/597918725
    return svPositionInput.w;
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

// 检查 lhs 深度是否比 rhs 深度更近
bool IsDepthNearer(float lhs, float rhs)
{
#ifdef MARCH_REVERSED_Z
    return lhs > rhs;
#else
    return lhs < rhs;
#endif
}

float GetNearestDepth(float lhs, float rhs)
{
#if MARCH_REVERSED_Z
    return max(lhs, rhs);
#else
    return min(lhs, rhs);
#endif
}

float GetFarthestDepth(float lhs, float rhs)
{
#if MARCH_REVERSED_Z
    return min(lhs, rhs);
#else
    return max(lhs, rhs);
#endif
}

uint GetTAAFrameIndex()
{
    return uint(_TAAParams.x);
}

float Min3(float x, float y, float z)
{
    return min(min(x, y), z);
}

float Max3(float x, float y, float z)
{
    return max(max(x, y), z);
}

// Ref: https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl
// Inserts the bits indicated by 'mask' from 'src' into 'dst'.
uint BitFieldInsert(uint mask, uint src, uint dst)
{
    return (src & mask) | (dst & ~mask);
}

// Ref: https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl
// Composes a floating point value with the magnitude of 'x' and the sign of 's'.
// See the comment about FastSign() below.
float CopySign(float x, float s, bool ignoreNegZero = true)
{
    if (ignoreNegZero)
    {
        return (s >= 0) ? abs(x) : -abs(x);
    }
    else
    {
        uint negZero = 0x80000000u;
        uint signBit = negZero & asuint(s);
        return asfloat(BitFieldInsert(negZero, signBit, asuint(x)));
    }
}

// Ref: https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.core/ShaderLibrary/Common.hlsl
// Returns -1 for negative numbers and 1 for positive numbers.
// 0 can be handled in 2 different ways.
// The IEEE floating point standard defines 0 as signed: +0 and -0.
// However, mathematics typically treats 0 as unsigned.
// Therefore, we treat -0 as +0 by default: FastSign(+0) = FastSign(-0) = 1.
// If (ignoreNegZero = false), FastSign(-0, false) = -1.
// Note that the sign() function in HLSL implements signum, which returns 0 for 0.
float FastSign(float s, bool ignoreNegZero = true)
{
    return CopySign(1.0, s, ignoreNegZero);
}

// Static Samplers
// https://docs.unity3d.com/Manual/SL-SamplerStates.html

SamplerState sampler_PointRepeat;
SamplerState sampler_LinearRepeat;
SamplerState sampler_TrilinearRepeat;
SamplerState sampler_Aniso1Repeat;
SamplerState sampler_Aniso2Repeat;
SamplerState sampler_Aniso3Repeat;
SamplerState sampler_Aniso4Repeat;
SamplerState sampler_Aniso5Repeat;
SamplerState sampler_Aniso6Repeat;
SamplerState sampler_Aniso7Repeat;
SamplerState sampler_Aniso8Repeat;
SamplerState sampler_Aniso9Repeat;
SamplerState sampler_Aniso10Repeat;
SamplerState sampler_Aniso11Repeat;
SamplerState sampler_Aniso12Repeat;
SamplerState sampler_Aniso13Repeat;
SamplerState sampler_Aniso14Repeat;
SamplerState sampler_Aniso15Repeat;
SamplerState sampler_Aniso16Repeat;

SamplerState sampler_PointClamp;
SamplerState sampler_LinearClamp;
SamplerState sampler_TrilinearClamp;
SamplerState sampler_Aniso1Clamp;
SamplerState sampler_Aniso2Clamp;
SamplerState sampler_Aniso3Clamp;
SamplerState sampler_Aniso4Clamp;
SamplerState sampler_Aniso5Clamp;
SamplerState sampler_Aniso6Clamp;
SamplerState sampler_Aniso7Clamp;
SamplerState sampler_Aniso8Clamp;
SamplerState sampler_Aniso9Clamp;
SamplerState sampler_Aniso10Clamp;
SamplerState sampler_Aniso11Clamp;
SamplerState sampler_Aniso12Clamp;
SamplerState sampler_Aniso13Clamp;
SamplerState sampler_Aniso14Clamp;
SamplerState sampler_Aniso15Clamp;
SamplerState sampler_Aniso16Clamp;

SamplerState sampler_PointMirror;
SamplerState sampler_LinearMirror;
SamplerState sampler_TrilinearMirror;
SamplerState sampler_Aniso1Mirror;
SamplerState sampler_Aniso2Mirror;
SamplerState sampler_Aniso3Mirror;
SamplerState sampler_Aniso4Mirror;
SamplerState sampler_Aniso5Mirror;
SamplerState sampler_Aniso6Mirror;
SamplerState sampler_Aniso7Mirror;
SamplerState sampler_Aniso8Mirror;
SamplerState sampler_Aniso9Mirror;
SamplerState sampler_Aniso10Mirror;
SamplerState sampler_Aniso11Mirror;
SamplerState sampler_Aniso12Mirror;
SamplerState sampler_Aniso13Mirror;
SamplerState sampler_Aniso14Mirror;
SamplerState sampler_Aniso15Mirror;
SamplerState sampler_Aniso16Mirror;

SamplerState sampler_PointMirrorOnce;
SamplerState sampler_LinearMirrorOnce;
SamplerState sampler_TrilinearMirrorOnce;
SamplerState sampler_Aniso1MirrorOnce;
SamplerState sampler_Aniso2MirrorOnce;
SamplerState sampler_Aniso3MirrorOnce;
SamplerState sampler_Aniso4MirrorOnce;
SamplerState sampler_Aniso5MirrorOnce;
SamplerState sampler_Aniso6MirrorOnce;
SamplerState sampler_Aniso7MirrorOnce;
SamplerState sampler_Aniso8MirrorOnce;
SamplerState sampler_Aniso9MirrorOnce;
SamplerState sampler_Aniso10MirrorOnce;
SamplerState sampler_Aniso11MirrorOnce;
SamplerState sampler_Aniso12MirrorOnce;
SamplerState sampler_Aniso13MirrorOnce;
SamplerState sampler_Aniso14MirrorOnce;
SamplerState sampler_Aniso15MirrorOnce;
SamplerState sampler_Aniso16MirrorOnce;

#endif
