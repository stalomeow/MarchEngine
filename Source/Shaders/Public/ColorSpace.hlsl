#ifndef _COLOR_SPACE_INCLUDED
#define _COLOR_SPACE_INCLUDED

// 内置宏
// MARCH_COLORSPACE_GAMMA 在使用 Gamma (sRGB) 颜色空间时定义

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ColorSpaceUtility.hlsli

float3 LinearToSRGB(float3 x)
{
    // Approximately pow(x, 1.0 / 2.2)
    return select(x < 0.0031308, 12.92 * x, 1.055 * pow(x, 1.0 / 2.4) - 0.055);
}

float4 LinearToSRGB(float4 x)
{
    return float4(LinearToSRGB(x.rgb), x.a);
}

float3 SRGBToLinear(float3 x)
{
    // Approximately pow(x, 2.2)
    return select(x < 0.04045, x / 12.92, pow((x + 0.055) / 1.055, 2.4));
}

float4 SRGBToLinear(float4 x)
{
    return float4(SRGBToLinear(x.rgb), x.a);
}

float3 LinearToGamma22(float3 x)
{
    return pow(x, 1.0 / 2.2);
}

float4 LinearToGamma22(float4 x)
{
    return float4(LinearToGamma22(x.rgb), x.a);
}

float3 Gamma22ToLinear(float3 x)
{
    return pow(x, 2.2);
}

float4 Gamma22ToLinear(float4 x)
{
    return float4(Gamma22ToLinear(x.rgb), x.a);
}

#endif
