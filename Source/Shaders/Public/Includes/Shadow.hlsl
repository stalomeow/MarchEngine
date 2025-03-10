#ifndef _SHADOW_INCLUDED
#define _SHADOW_INCLUDED

#include "Common.hlsl"
#include "Sampling.hlsl"

cbuffer cbShadow
{
    float4x4 _ShadowMatrix;
};

Texture2D _ShadowMap;
SamplerComparisonState sampler_ShadowMap;

float4 TransformWorldToShadowCoord(float3 positionWS)
{
    return mul(_ShadowMatrix, float4(positionWS, 1));
}

bool CompareDepthCloser(float lhs, float rhs)
{
#ifdef MARCH_REVERSED_Z
    return lhs > rhs;
#else
    return lhs < rhs;
#endif
}

float GetBlockerSearchRadius(float receiverDepth, float depth2RadialScale)
{
#ifdef MARCH_REVERSED_Z
    return (1.0 - receiverDepth) * depth2RadialScale;
#else
    return receiverDepth * depth2RadialScale;
#endif
}

bool BlockerSearch(float4 shadowCoord, float depth2RadialScale, int sampleCount, float2x2 jitter, out float blockerDepth, out float cnt)
{
    // assume orthographic projection
    // shadowCoord /= shadowCoord.w;

    float blockerSum = 0;
    int blockerCount = 0;
    float radius = GetBlockerSearchRadius(shadowCoord.z, depth2RadialScale);
    radius = max(radius, 1.0 / 2048.0);

    for (int i = 0; i < sampleCount && i < FIBONACCI_SPIRAL_DISK_SAMPLE_COUNT; i++)
    {
        float2 offset = SampleFibonacciSpiralDiskClumped(i, sampleCount, 1.0) * radius;
        float2 uv = shadowCoord.xy + mul(jitter, offset);

        if (any(uv < 0.0) || any(uv > 1.0))
        {
            continue;
        }

        float zOffset = length(offset) / depth2RadialScale;

#ifdef MARCH_REVERSED_Z
        float z = shadowCoord.z + zOffset;
#else
        float z = shadowCoord.z - zOffset;
#endif

        float blocker = _ShadowMap.SampleLevel(sampler_PointClamp, uv, 0).r;

        if (CompareDepthCloser(blocker, z))
        {
            blockerSum += blocker;
            blockerCount++;
        }
    }

    cnt = blockerCount;

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

float EstimatePenumbra(float recevierDepth, float blockerDepth, float depth2RadialScale)
{
    return abs(recevierDepth - blockerDepth) * depth2RadialScale;
}

float PCSSFilter(float4 shadowCoord, float depth2RadialScale, int sampleCount, float2x2 jitter, float filterSize)
{
    // assume orthographic projection
    // shadowCoord /= shadowCoord.w;

    float sum = 0;
    int count = 0;
    filterSize = max(filterSize, 1.0 / 2048.0);

    for (int i = 0; i < sampleCount && i < FIBONACCI_SPIRAL_DISK_SAMPLE_COUNT; i++)
    {
        float2 offset = SampleFibonacciSpiralDiskUniform(i, sampleCount) * filterSize;
        float2 uv = shadowCoord.xy + mul(jitter, offset);

        if (any(uv < 0.0) || any(uv > 1.0))
        {
            continue;
        }
        
        float zOffset = length(offset) / depth2RadialScale;

#ifdef MARCH_REVERSED_Z
        float z = shadowCoord.z + zOffset;
#else
        float z = shadowCoord.z - zOffset;
#endif

        sum += _ShadowMap.SampleCmpLevelZero(sampler_ShadowMap, uv, z).r;
        count++;
    }

    return count > 0 ? (sum / count) : 1.0;
}

float SampleShadowMap(float2 positionSS, float4 shadowCoord)
{
    float jitterAngle = 2 * PI * InterleavedGradientNoise(positionSS, 1);
    float jitterCos = cos(jitterAngle);
    float jitterSin = sin(jitterAngle);
    float2x2 jitter = { jitterCos, -jitterSin, jitterSin, jitterCos };

    const float AngularDiameter = 180.0 / PI * 10;
    const float depth2RadialScale = tan(0.5 * AngularDiameter) * 0.1;

    float blockerDepth;
    float cnt;
    if (!BlockerSearch(shadowCoord, depth2RadialScale, 64, jitter, blockerDepth, cnt))
    {
        return 1.0;
    }
    //return cnt / 64.0;

    float penumbra = EstimatePenumbra(shadowCoord.z, blockerDepth, depth2RadialScale);
    return PCSSFilter(shadowCoord, depth2RadialScale, 64, jitter, penumbra);
}

#endif
