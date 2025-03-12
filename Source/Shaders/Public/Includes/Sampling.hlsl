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

// 单位半球上 GGX 权重采样，立体角 pdf 是 D_GGX * NoH
float3 SampleHemisphereGGX(float2 xi, float a2, float3x3 tbn)
{
    float cosThetaSq = (1.0 - xi.x) / (1 + (a2 - 1) * xi.x);
    float cosTheta = sqrt(cosThetaSq);
    float sinTheta = sqrt(1.0 - cosThetaSq);
    float phi = 2.0 * PI * xi.y;

    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    float z = cosTheta;
    return normalize(mul(tbn, float3(x, y, z)));
}

float3x3 GetTBNForRandomSampling(float3 N)
{
    // Ref: https://learnopengl.com/PBR/IBL/Specular-IBL
    // from tangent-space vector to world-space sample vector

    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
    return transpose(float3x3(tangent, bitangent, N)); // 行主序转置成列主序
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

#define FIBONACCI_SPIRAL_DISK_SAMPLE_COUNT 64

// Ref: https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.high-definition/Runtime/Lighting/Shadow/HDPCSS.hlsl
// Fibonacci Spiral Disk Sampling Pattern
// https://people.irisa.fr/Ricardo.Marques/articles/2013/SF_CGF.pdf
//
// Normalized direction vector portion of fibonacci spiral can be baked into a LUT, regardless of sampleCount.
// This allows us to treat the directions as a progressive sequence, using any sampleCount in range [0, n <= LUT_LENGTH]
// the radius portion of spiral construction is coupled to sample count, but is fairly cheap to compute at runtime per sample.
// Generated (in javascript) with:
// var res = "";
// for (var i = 0; i < 64; ++i)
// {
//     var a = Math.PI * (3.0 - Math.sqrt(5.0));
//     var b = a / (2.0 * Math.PI);
//     var c = i * b;
//     var theta = (c - Math.floor(c)) * 2.0 * Math.PI;
//     res += "float2 (" + Math.cos(theta) + ", " + Math.sin(theta) + "),\n";
// }
static const float2 g_FibonacciSpiralDirection[FIBONACCI_SPIRAL_DISK_SAMPLE_COUNT] =
{
    float2(1.0, 0.0),
    float2(-0.7373688780783197, 0.6754902942615238),
    float2(0.08742572471695988, -0.9961710408648278),
    float2(0.6084388609788625, 0.793600751291696),
    float2(-0.9847134853154288, -0.174181950379311),
    float2(0.8437552948123969, -0.5367280526263233),
    float2(-0.25960430490148884, 0.9657150743757782),
    float2(-0.46090702471337114, -0.8874484292452536),
    float2(0.9393212963241183, 0.3430386308741014),
    float2(-0.924345556137805, 0.3815564084749356),
    float2(0.423845995047909, -0.9057342725556143),
    float2(0.29928386444487326, 0.9541641203078969),
    float2(-0.8652112097532296, -0.501407581232427),
    float2(0.9766757736281757, -0.21471942904125949),
    float2(-0.5751294291397363, 0.8180624302199686),
    float2(-0.12851068979899202, -0.9917081236973847),
    float2(0.7646489954560439, 0.6444469828838233),
    float2(-0.9991460540072823, 0.04131782619737919),
    float2(0.7088294143034162, -0.7053799411794157),
    float2(-0.04619144594036213, 0.9989326054954552),
    float2(-0.6407091449636957, -0.7677836880006569),
    float2(0.9910694127331615, 0.1333469877603031),
    float2(-0.8208583369658855, 0.5711318504807807),
    float2(0.21948136924637865, -0.9756166914079191),
    float2(0.4971808749652937, 0.8676469198750981),
    float2(-0.952692777196691, -0.30393498034490235),
    float2(0.9077911335843911, -0.4194225289437443),
    float2(-0.38606108220444624, 0.9224732195609431),
    float2(-0.338452279474802, -0.9409835569861519),
    float2(0.8851894374032159, 0.4652307598491077),
    float2(-0.9669700052147743, 0.25489019011123065),
    float2(0.5408377383579945, -0.8411269468800827),
    float2(0.16937617250387435, 0.9855514761735877),
    float2(-0.7906231749427578, -0.6123030256690173),
    float2(0.9965856744766464, -0.08256508601054027),
    float2(-0.6790793464527829, 0.7340648753490806),
    float2(0.0048782771634473775, -0.9999881011351668),
    float2(0.6718851669348499, 0.7406553331023337),
    float2(-0.9957327006438772, -0.09228428288961682),
    float2(0.7965594417444921, -0.6045602168251754),
    float2(-0.17898358311978044, 0.9838520605119474),
    float2(-0.5326055939855515, -0.8463635632843003),
    float2(0.9644371617105072, 0.26431224169867934),
    float2(-0.8896863018294744, 0.4565723210368687),
    float2(0.34761681873279826, -0.9376366819478048),
    float2(0.3770426545691533, 0.9261958953890079),
    float2(-0.9036558571074695, -0.4282593745796637),
    float2(0.9556127564793071, -0.2946256262683552),
    float2(-0.50562235513749, 0.8627549095688868),
    float2(-0.2099523790012021, -0.9777116131824024),
    float2(0.8152470554454873, 0.5791133210240138),
    float2(-0.9923232342597708, 0.12367133357503751),
    float2(0.6481694844288681, -0.7614961060013474),
    float2(0.036443223183926, 0.9993357251114194),
    float2(-0.7019136816142636, -0.7122620188966349),
    float2(0.998695384655528, 0.05106396643179117),
    float2(-0.7709001090366207, 0.6369560596205411),
    float2(0.13818011236605823, -0.9904071165669719),
    float2(0.5671206801804437, 0.8236347091470045),
    float2(-0.9745343917253847, -0.22423808629319533),
    float2(0.8700619819701214, -0.49294233692210304),
    float2(-0.30857886328244405, 0.9511987621603146),
    float2(-0.4149890815356195, -0.9098263912451776),
    float2(0.9205789302157817, 0.3905565685566777)
};

float2 SampleFibonacciSpiralDiskUniform(int sampleIndex, int sampleCount)
{
    // Samples biased away from the center, so that sample 0 doesn't fall at (0, 0), or it will not be affected by sample jitter and create a visible edge.
    float xi = (sampleIndex + 0.5) / sampleCount;
    float r = sqrt(xi); // 在单位圆上均匀采样
    return r * g_FibonacciSpiralDirection[sampleIndex];
}

float2 SampleFibonacciSpiralDiskClumped(int sampleIndex, int sampleCount, float clumpExponent)
{
    // Samples biased away from the center, so that sample 0 doesn't fall at (0, 0), or it will not be affected by sample jitter and create a visible edge.
    float xi = (sampleIndex + 0.5) / sampleCount;
    float r = pow(xi, clumpExponent); // clumpExponent > 0.5 时，采样点在中心更加密集，非均匀采样
    return r * g_FibonacciSpiralDirection[sampleIndex];
}

// From  Next Generation Post Processing in Call of Duty: Advanced Warfare [Jimenez 2014]
// http://advances.realtimerendering.com/s2014/index.html
float InterleavedGradientNoise(float2 pixCoord, int frameCount)
{
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    float2 frameMagicScale = float2(2.083f, 4.867f);
    pixCoord += frameCount * frameMagicScale;
    return frac(magic.z * frac(dot(pixCoord, magic.xy)));
}

#endif
