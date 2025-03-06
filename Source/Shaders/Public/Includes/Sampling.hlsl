#ifndef _SAMPLING_INCLUDED
#define _SAMPLING_INCLUDED

#include "Common.hlsl"

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

// 单位球上均匀采样，立体角 pdf 是 1 / (4 * PI)
float3 SampleSphereUniform(float2 xi)
{
    float cosTheta = 1.0 - 2.0 * xi.x;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * PI * xi.y;

    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;
    return float3(x, y, z);
}

// 单位半球上余弦权重采样，立体角 pdf 是 cos(theta) / PI
float3 SampleHemisphereCosine(float2 xi, float3x3 tbn)
{
    // Malley's Method
    float r = sqrt(xi.x);
    float phi = 2.0 * PI * xi.y;

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(1.0 - x * x - y * y);
    return normalize(mul(tbn, float3(x, y, z)));
}

// 将 Cubemap 上某个 Face 的 uv 转换为相对 Cube 原点的方向
// uv 原点在左上角，xy 范围是 [0, 1]
float3 CubeFaceUVToDirection(float2 uv, int face)
{
    static const float sqrt3 = 1.73205080757f;
    static const float3x3 matrices[6] =
    {
        { 0, 0, sqrt3 / 4.0, 0, -sqrt3 / 2.0, sqrt3 / 4.0, -sqrt3 / 2.0, 0, sqrt3 / 4.0, }, // +X
        { 0, 0, -sqrt3 / 4.0, 0, -sqrt3 / 2.0, sqrt3 / 4.0, sqrt3 / 2.0, 0, -sqrt3 / 4.0, }, // -X
        { sqrt3 / 2.0, 0, -sqrt3 / 4.0, 0, 0, sqrt3 / 4.0, 0, sqrt3 / 2.0, -sqrt3 / 4.0, }, // +Y
        { sqrt3 / 2.0, 0, -sqrt3 / 4.0, 0, 0, -sqrt3 / 4.0, 0, -sqrt3 / 2.0, sqrt3 / 4.0, }, // -Y
        { sqrt3 / 2.0, 0, -sqrt3 / 4.0, 0, -sqrt3 / 2.0, sqrt3 / 4.0, 0, 0, sqrt3 / 4.0, }, // +Z
        { -sqrt3 / 2.0, 0, sqrt3 / 4.0, 0, -sqrt3 / 2.0, sqrt3 / 4.0, 0, 0, -sqrt3 / 4.0, }, // -Z
    };

    return mul(matrices[face], float3(uv, 1.0));
}

#endif
