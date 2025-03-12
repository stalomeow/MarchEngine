#ifndef _SHADOW_INCLUDED
#define _SHADOW_INCLUDED

#include "Common.hlsl"
#include "Sampling.hlsl"

cbuffer cbShadow
{
    float4x4 _ShadowMatrix;
    float4 _ShadowParams; // x: depth2RadialScale
};

Texture2D _ShadowMap;
SamplerComparisonState sampler_ShadowMap;

float4 TransformWorldToShadowCoord(float3 positionWS)
{
    return mul(_ShadowMatrix, float4(positionWS, 1.0));
}

float GetShadowMapSize()
{
    float width, height;
    _ShadowMap.GetDimensions(width, height);
    return width; // ShadowMap 是正方形的
}

float GetBlockerSearchRadius(float depth2RadialScale, float receiverDepth)
{
#ifdef MARCH_REVERSED_Z
    return (1.0 - receiverDepth) * depth2RadialScale;
#else
    return receiverDepth * depth2RadialScale;
#endif
}

float GetReceiverDepthBias(float depth2RadialScale, float2 sampleOffset)
{
    float totalBias = 1e-5; // 避免 shadow map 精度问题

    if (depth2RadialScale != 0.0)
    {
        // 参考 HDRP，加一个圆锥形的偏移，减少自遮挡
        // 但是有时偏移量太大会导致阴影变硬/断裂，所以 4 次方减缓增长
        // float coneBias = length(sampleOffset) / depth2RadialScale;
        // totalBias += pow(coneBias, 4);

        // 上面两行的优化版本
        totalBias += Square(dot(sampleOffset, sampleOffset) / Square(depth2RadialScale));
    }

#ifndef MARCH_REVERSED_Z
    totalBias = -totalBias;
#endif

    return totalBias;
}

bool BlockerSearch(float depth2RadialScale, float3 shadowCoord, float2x2 jitter, out float blockerDepth)
{
    const float radius = GetBlockerSearchRadius(depth2RadialScale, shadowCoord.z);
    float blockerSum = 0;
    int blockerCount = 0;

    for (int i = 0; i < FIBONACCI_SPIRAL_DISK_SAMPLE_COUNT; i++)
    {
        float2 offset = SampleFibonacciSpiralDiskClumped(i, FIBONACCI_SPIRAL_DISK_SAMPLE_COUNT, 2.0);
        offset = mul(jitter, offset * radius);
        float2 uv = shadowCoord.xy + offset;

        if (any(uv < 0.0) || any(uv > 1.0))
        {
            continue;
        }

        float receiver = shadowCoord.z + GetReceiverDepthBias(depth2RadialScale, offset);
        float blocker = _ShadowMap.SampleLevel(sampler_PointClamp, uv, 0).r;

        if (IsDepthCloser(blocker, receiver))
        {
            blockerSum += blocker;
            blockerCount++;
        }
    }

    if (blockerCount > 0)
    {
        blockerDepth = blockerSum / blockerCount;
        return true;
    }
    else
    {
        blockerDepth = 0.0;
        return false;
    }
}

float EstimatePenumbra(float depth2RadialScale, float recevierDepth, float blockerDepth)
{
    return abs(recevierDepth - blockerDepth) * depth2RadialScale;
}

float PercentageCloserFiltering(float depth2RadialScale, float3 shadowCoord, float2x2 jitter, float penumbra)
{
    const float baseRadius = 3.5 / GetShadowMapSize(); // 至少 7x7 的 filter，保证近处阴影不会太硬
    const float radius = baseRadius + penumbra;

    float sum = 0;
    int count = 0;

    for (int i = 0; i < FIBONACCI_SPIRAL_DISK_SAMPLE_COUNT; i++)
    {
        float2 offset = SampleFibonacciSpiralDiskUniform(i, FIBONACCI_SPIRAL_DISK_SAMPLE_COUNT);
        offset = mul(jitter, offset * radius);
        float2 uv = shadowCoord.xy + offset;

        if (any(uv < 0.0) || any(uv > 1.0))
        {
            continue;
        }

        float receiver = shadowCoord.z + GetReceiverDepthBias(depth2RadialScale, offset);
        sum += _ShadowMap.SampleCmpLevelZero(sampler_ShadowMap, uv, receiver).r;
        count++;
    }

    return (count > 0) ? (sum / count) : 1.0;
}

float2x2 GetPCSSRotationJitter(float2 positionSS)
{
    float angle = InterleavedGradientNoise(positionSS, 1) * 2 * PI;
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);
    return float2x2(cosAngle, -sinAngle, sinAngle, cosAngle);
}

float SampleShadowMap(float2 positionSS, float4 shadowCoord)
{
    const float depth2RadialScale = _ShadowParams.x;
    const float2x2 jitter = GetPCSSRotationJitter(positionSS);

    // assume orthographic projection
    // shadowCoord /= shadowCoord.w;

    float blockerDepth;
    if (!BlockerSearch(depth2RadialScale, shadowCoord.xyz, jitter, blockerDepth))
    {
        return 1.0;
    }

    float penumbra = EstimatePenumbra(depth2RadialScale, shadowCoord.z, blockerDepth);
    return PercentageCloserFiltering(depth2RadialScale, shadowCoord.xyz, jitter, penumbra);
}

#endif
