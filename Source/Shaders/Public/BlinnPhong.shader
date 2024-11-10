Shader "BlinnPhong"
{
    Properties
    {
        [Tooltip(The diffuse albedo.)] _DiffuseAlbedo("Diffuse Albedo", Color) = (1, 1, 1, 1)
        [Tooltip(The fresnel R0.)] _FresnelR0("Fresnel R0", Vector) = (0.01, 0.01, 0.01, 0)
        [Tooltip(The roughness.)] [Range(0, 1)] _Roughness("Roughness", Float) = 0.25
        [Tooltip(The diffuse map.)] _DiffuseMap("Diffuse Map", 2D) = "white" {}
        [Tooltip(The normal map.)] _BumpMap("Bump Map", 2D) = "bump" {}
        [Range(0, 1)] _Cutoff("Alpha Cutoff", Float) = 0.5
    }

    Pass
    {
        Name "GBuffer"

        Cull Back
        ZTest Less
        ZWrite On

        Tags
        {
            "LightMode" = "GBuffer"
        }

        // Blend 0 SrcAlpha OneMinusSrcAlpha, One Zero
        // BlendOp 0 Add, Add
        // ColorMask 0 RGBA

        Stencil
        {
            Ref 1

            Comp Always
            Pass Replace
            Fail Keep
            ZFail Keep
        }

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #pragma multi_compile _ _ALPHATEST_ON

        #include "Includes/Common.hlsl"
        #include "Includes/GBuffer.hlsl"

        Texture2D _DiffuseMap;
        SAMPLER(_DiffuseMap);
        Texture2D _BumpMap;
        SAMPLER(_BumpMap);

        cbuffer cbMaterial
        {
            float4 _DiffuseAlbedo;
            float3 _FresnelR0;
            float _Roughness;
            float _DitherAlpha;
            float _Cutoff;
        };

        struct Attributes
        {
            float3 positionOS : POSITION;
            float3 normalOS : NORMAL;
            float4 tangentOS : TANGENT;
            float2 uv : TEXCOORD0;
        };

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float3 normalWS : NORMAL;
            float4 tangentWS : TANGENT;
            float2 uv : TEXCOORD0;
        };

        Varyings vert(Attributes input)
        {
            float3 positionWS = TransformObjectToWorld(input.positionOS);

            Varyings output;
            output.positionCS = TransformWorldToHClip(positionWS);
            output.normalWS = TransformObjectToWorldNormal(input.normalOS);
            output.tangentWS.xyz = TransformObjectToWorldDir(input.tangentOS.xyz);
            output.tangentWS.w = input.tangentOS.w;
            output.uv = input.uv;
            return output;
        }

        PixelGBufferOutput frag(Varyings input)
        {
            float4 diffuse = _DiffuseMap.Sample(sampler_DiffuseMap, input.uv);

            #ifdef _ALPHATEST_ON
                clip(diffuse.a - _Cutoff);
            #endif

            float3 normalTS = normalize(_BumpMap.Sample(sampler_BumpMap, input.uv).xyz * 2.0 - 1.0);
            float3 N = normalize(input.normalWS);
            float3 T = normalize(input.tangentWS.xyz - dot(input.tangentWS.xyz, N) * N);
            float3 B = cross(N, T) * input.tangentWS.w;
            float3 bumpedNomalWS = normalize(mul(normalTS, float3x3(T, B, N))); // float3x3() 是行主序矩阵

            GBufferData data;
            data.albedo = (diffuse * _DiffuseAlbedo).xyz;
            data.shininess = 1 - _Roughness;
            data.normalWS = bumpedNomalWS;
            data.depth = input.positionCS.z;
            data.fresnelR0 = _FresnelR0;
            return PackGBufferData(data);
        }
        ENDHLSL
    }

    Pass
    {
        Name "ShadowCaster"

        Cull Back
        ZTest Less
        ZWrite On
        ColorMask 0

        Tags
        {
            "LightMode" = "ShadowCaster"
        }

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert

        #include "Includes/Common.hlsl"

        struct Attributes
        {
            float3 positionOS : POSITION;
        };

        struct Varyings
        {
            float4 positionCS : SV_Position;
        };

        Varyings vert(Attributes input)
        {
            float3 positionWS = TransformObjectToWorld(input.positionOS);

            Varyings output;
            output.positionCS = TransformWorldToHClip(positionWS);
            return output;
        }
        ENDHLSL
    }
}
