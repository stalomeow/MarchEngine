#ifndef _COLOR_SPACE_INCLUDED
#define _COLOR_SPACE_INCLUDED

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/ColorSpaceUtility.hlsli
// https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.core/ShaderLibrary/Color.hlsl

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

// This function take a rgb color (best is to provide color in sRGB space)
// and return a YCoCg color in [0..1] space for 8bit (An offset is apply in the function)
// Ref: http://www.nvidia.com/object/real-time-ycocg-dxt-compression.html
#define YCOCG_CHROMA_BIAS (128.0 / 255.0)

float3 RGBToYCoCg(float3 rgb)
{
    float3 YCoCg;
    YCoCg.x = dot(rgb, float3(0.25, 0.5, 0.25));
    YCoCg.y = dot(rgb, float3(0.5, 0.0, -0.5)) + YCOCG_CHROMA_BIAS;
    YCoCg.z = dot(rgb, float3(-0.25, 0.5, -0.25)) + YCOCG_CHROMA_BIAS;
    return YCoCg;
}

float3 YCoCgToRGB(float3 YCoCg)
{
    float Y = YCoCg.x;
    float Co = YCoCg.y - YCOCG_CHROMA_BIAS;
    float Cg = YCoCg.z - YCOCG_CHROMA_BIAS;

    float3 rgb;
    rgb.r = Y + Co - Cg;
    rgb.g = Y + Cg;
    rgb.b = Y - Co - Cg;
    return rgb;
}

#endif
