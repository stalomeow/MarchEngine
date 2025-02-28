#ifndef _AMBIENT_OCCLUSION_INCLUDED
#define _AMBIENT_OCCLUSION_INCLUDED

#ifdef AMBIENT_OCCLUSION_CALCULATING
    RWTexture2D<float> _SSAOMap;
#else
    Texture2D _SSAOMap;
    SamplerState sampler_SSAOMap;

    float SampleScreenSpaceAmbientOcclusion(float2 uv)
    {
        return _SSAOMap.Sample(sampler_SSAOMap, uv).r;
    }
#endif

#endif // _AMBIENT_OCCLUSION_INCLUDED
