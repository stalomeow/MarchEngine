#ifndef _DEPTH_INCLUDED
#define _DEPTH_INCLUDED

Texture2D _HiZTexture; // 第一层就是场景的原始深度
SamplerState sampler_HiZTexture;

float SampleSceneDepth(float2 uv)
{
    return _HiZTexture.SampleLevel(sampler_HiZTexture, uv, 0).r;
}

void GetSceneDepthDimensions(out uint width, out uint height)
{
    _HiZTexture.GetDimensions(width, height);
}

void GetSceneDepthDimensions(out float width, out float height)
{
    _HiZTexture.GetDimensions(width, height);
}

// 采样 Hi-Z Buffer，每层 mip 保存的都是距离相机最近的深度值
float SampleNearestHierarchicalSceneDepth(float2 uv, float mipLevel)
{
    return _HiZTexture.SampleLevel(sampler_HiZTexture, uv, mipLevel).r;
}

// 采样 Hi-Z Buffer，每层 mip 保存的都是距离相机最远的深度值
//float SampleFarthestHierarchicalSceneDepth(float2 uv, float mipLevel)
//{
//}

int GetHierarchicalSceneDepthMaxMipLevel()
{
    int width, height, numberOfLevels;
    _HiZTexture.GetDimensions(0, width, height, numberOfLevels);
    return numberOfLevels - 1;
}

void GetHierarchicalSceneDepthDimensions(uint mipLevel, out uint width, out uint height)
{
    uint numberOfLevels;
    _HiZTexture.GetDimensions(mipLevel, width, height, numberOfLevels);
}

void GetHierarchicalSceneDepthDimensions(uint mipLevel, out float width, out float height)
{
    float numberOfLevels;
    _HiZTexture.GetDimensions(mipLevel, width, height, numberOfLevels);
}

#endif
