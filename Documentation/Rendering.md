# Rendering

## GfxCommandContext

这是对 `ID3D12CommandList` 的封装。我不建议直接使用这个，知道有这个东西就行。

## RenderGraph

编写一个 Pass 的基本流程

1. 申请资源
2. 调用 `RenderGraph::AddPass` 获取 `RenderGraphBuilder`
3. 利用 `RenderGraphBuilder` 配置 Pass

具体可以参考 `RenderPipeline` 里的代码。

### In / Out / InOut

在该项目的实现中，没有使用常见的 Read / Write 来标记资源，而是使用 In / Out

``` cpp
builder.In(m_Resource.DepthStencilTarget);
```

RenderGraph 是利用资源的读写关系构建出一张有向无环图，为此，如果一个 Pass 需要一个资源，则必须明确两点

1. 该 Pass 是否需要读取前面 Pass 向该资源写入的内容
2. 该 Pass 是否会向该资源写入内容

使用 Read / Write 的表达能力是不够的，考虑下面两种情况

1. 该 Pass 先写入资源 A，再读取自己写入的内容
2. 该 Pass 先读取前面 Pass 写入资源 A 的内容，然后自己再写入资源 A 

这两种情况是要分开处理的，但却都对应 `builder.ReadWrite(A)`。使用 In / Out 就能解决问题，此时两种情况分别对应 `builder.Out(A)` 和 `builder.InOut(A)`。

总结一下

- 如果 Pass 需要读取前面 Pass 向资源 A 写入的内容，则使用 `builder.In(A)` 来标记资源
- 如果 Pass 需要写入资源 A，则使用 `builder.Out(A)` 来标记资源

`builder.InOut(A)` 就是既 In 又 Out 的简写。把一个 Pass 想象成一个函数就容易理解了，In 就是参数，Out 就是返回值。

## Shader

可以使用 [HLSL Tools for Visual Studio](https://marketplace.visualstudio.com/items?itemName=TimGJones.HLSLToolsforVisualStudio) 扩展编写 Shader。如果有更好的扩展，请推荐给我！

### 预定义宏

|宏|描述|
|:-|:-|
|`MARCH_REVERSED_Z`|在开启 Reversed-Z Buffer 时定义|
|`MARCH_NEAR_CLIP_VALUE`|近裁剪平面的深度（NDC）|
|`MARCH_FAR_CLIP_VALUE`|远裁剪平面的深度（NDC）|
|`MARCH_COLORSPACE_GAMMA`|在使用 Gamma 颜色空间时定义，此时不会自动执行 sRGB 的 OETF / EOTF|
|`SHADER_STAGE_VERTEX`|当前处于 Vertex Shader Stage|
|`SHADER_STAGE_PIXEL`|当前处于 Pixel Shader Stage|
|`SHADER_STAGE_DOMAIN`|当前处于 Domain Shader Stage|
|`SHADER_STAGE_HULL`|当前处于 Hull Shader Stage|
|`SHADER_STAGE_GEOMETRY`|当前处于 Geometry Shader Stage|
|`SHADER_STAGE_COMPUTE`|当前处于 Compute Shader Stage|

### 额外的预处理指令

|预处理指令|描述|
|:-|:-|
|`#pragma target <sm>`|设置 Shader Model，不能低于 6.0，默认是 6.0|
|`#pragma multi_compile <keyword1> <keyword2> ...`|创建变体，如果 `<keyword>` 是纯 `_`，则表示无 Keyword 的情况|
|`#pragma vs <func>`|设置 Vertex Shader，在 Compute Shader 中无效|
|`#pragma ps <func>`|设置 Pixel Shader，在 Compute Shader 中无效|
|`#pragma ds <func>`|设置 Domain Shader，在 Compute Shader 中无效|
|`#pragma hs <func>`|设置 Hull Shader，在 Compute Shader 中无效|
|`#pragma gs <func>`|设置 Geometry Shader，在 Compute Shader 中无效|
|`#pragma kernel <func>`|设置 Compute Shader Kernel，只在 Compute Shader 中有效|

### 与 Unity ShaderLab 的区别

- 我只实现了大多数常用的功能
- 不需要写 SubShader Block，Pass Block 直接写在 Shader Block 中
- 如果要声明 Range Property，需要写成 `[Range(0, 1)] _Cutoff("Alpha Cutoff", Float) = 0.5`
- Properties 不需要在 HLSL 中定义就能直接使用，但这样代码提示会失效，可以用下面的方式解决

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
