# Shader

## 预定义宏

|宏|描述|
|:-|:-|
|`MARCH_REVERSED_Z`|在开启 Reversed-Z Buffer 时定义|
|`MARCH_NEAR_CLIP_VALUE`|近裁剪平面的深度（NDC）|
|`MARCH_FAR_CLIP_VALUE`|远裁剪平面的深度（NDC）|
|`MARCH_COLORSPACE_GAMMA`|在使用 Gamma 颜色空间时定义，此时不会自动执行 sRGB 的 OETF/EOTF|
|`SHADER_STAGE_VERTEX`|当前处于 Vertex Shader Stage|
|`SHADER_STAGE_PIXEL`|当前处于 Pixel Shader Stage|
|`SHADER_STAGE_DOMAIN`|当前处于 Domain Shader Stage|
|`SHADER_STAGE_HULL`|当前处于 Hull Shader Stage|
|`SHADER_STAGE_GEOMETRY`|当前处于 Geometry Shader Stage|
|`SHADER_STAGE_COMPUTE`|当前处于 Compute Shader Stage|

## Properties

Shader Properties 不需要在 HLSL 中定义就能直接使用，但这样代码提示会失效，可以用下面的方式解决

``` hlsl
// 手动定义 Shader Properties 以实现代码提示
#ifndef MARCH_SHADER_PROPERTIES
    Texture2D _DiffuseMap;
    SamplerState sampler_DiffuseMap;
    Texture2D _BumpMap;
    SamplerState sampler_BumpMap;

    cbuffer cbMaterial
    {
        float4 _DiffuseAlbedo;
        float3 _FresnelR0;
        float _Roughness;
        float _DitherAlpha;
        float _Cutoff;
    };
#endif
```

`MARCH_SHADER_PROPERTIES` 仅在编译时定义。
