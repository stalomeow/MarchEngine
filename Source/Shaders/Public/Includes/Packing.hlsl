#ifndef _PACKING_INCLUDED
#define _PACKING_INCLUDED

// Ref: https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.core/ShaderLibrary/Packing.hlsl

// Ref: http://jcgt.org/published/0003/02/01/paper.pdf "A Survey of Efficient Representations for Independent Unit Vectors"
// Encode with Oct, this function work with any size of output
// return float between [-1, 1]
float2 PackNormalOctQuadEncode(float3 n)
{
    //float l1norm    = dot(abs(n), 1.0);
    //float2 res0     = n.xy * (1.0 / l1norm);

    //float2 val      = 1.0 - abs(res0.yx);
    //return (n.zz < float2(0.0, 0.0) ? (res0 >= 0.0 ? val : -val) : res0);

    // Optimized version of above code:
    n *= rcp(max(dot(abs(n), 1.0), 1e-6));
    float t = saturate(-n.z);
    return n.xy + float2(n.x >= 0.0 ? t : -t, n.y >= 0.0 ? t : -t);
}

float3 UnpackNormalOctQuadEncode(float2 f)
{
    // NOTE: Do NOT use abs() in this line. It causes miscompilations. (UUM-62216, UUM-70600)
    float3 n = float3(f.x, f.y, 1.0 - (f.x < 0 ? -f.x : f.x) - (f.y < 0 ? -f.y : f.y));

    //float2 val = 1.0 - abs(n.yx);
    //n.xy = (n.zz < float2(0.0, 0.0) ? (n.xy >= 0.0 ? val : -val) : n.xy);

    // Optimized version of above code:
    float t = max(-n.z, 0.0);
    n.xy += float2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);

    return normalize(n);
}

// Pack float2 (each of 12 bit) in 888
uint3 PackFloat2To888UInt(float2 f)
{
    uint2 i = (uint2) (f * 4095.5);
    uint2 hi = i >> 8;
    uint2 lo = i & 255;
    // 8 bit in lo, 4 bit in hi
    uint3 cb = uint3(lo, hi.x | (hi.y << 4));
    return cb;
}

// Pack float2 (each of 12 bit) in 888
float3 PackFloat2To888(float2 f)
{
    return PackFloat2To888UInt(f) / 255.0;
}

// Unpack 2 float of 12bit packed into a 888
float2 Unpack888UIntToFloat2(uint3 x)
{
    // 8 bit in lo, 4 bit in hi
    uint hi = x.z >> 4;
    uint lo = x.z & 15;
    uint2 cb = x.xy | uint2(lo << 8, hi << 8);

    return cb / 4095.0;
}

// Unpack 2 float of 12bit packed into a 888
float2 Unpack888ToFloat2(float3 x)
{
    uint3 i = (uint3) (x * 255.5); // +0.5 to fix precision error on iOS
    return Unpack888UIntToFloat2(i);
}

#endif
