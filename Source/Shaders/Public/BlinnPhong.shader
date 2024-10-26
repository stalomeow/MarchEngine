Shader "BlinnPhong"
{
    Properties
    {
        [Tooltip(The diffuse albedo.)] _DiffuseAlbedo("Diffuse Albedo", Color) = (1, 1, 1, 1)
        [Tooltip(The fresnel R0.)] _FresnelR0("Fresnel R0", Vector) = (0.01, 0.01, 0.01, 0)
        [Tooltip(The roughness.)] [Range(0, 1)] _Roughness("Roughness", Float) = 0.25
        [Tooltip(The diffuse map.)] _DiffuseMap("Diffuse Map", 2D) = "white" {}
        [Range(0, 1)] _DitherAlpha("Dither Alpha", Float) = 1
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

        #include "Includes/Common.hlsl"
        #include "Includes/GBuffer.hlsl"

        Texture2D _DiffuseMap;
        SAMPLER(_DiffuseMap);

        cbuffer cbMaterial
        {
            float4 _DiffuseAlbedo;
            float3 _FresnelR0;
            float _Roughness;
            float _DitherAlpha;
        };

        struct Attributes
        {
            float3 positionOS : POSITION;
            float3 normalOS : NORMAL;
            float2 uv : TEXCOORD0;
        };

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float3 normalWS : NORMAL;
            float2 uv : TEXCOORD0;
        };

        Varyings vert(Attributes input)
        {
            float3 positionWS = TransformObjectToWorld(input.positionOS);

            Varyings output;
            output.positionCS = TransformWorldToHClip(positionWS);
            output.normalWS = TransformObjectToWorldNormal(input.normalOS);
            output.uv = input.uv;
            return output;
        }

        void DoDitherAlphaEffect(float4 pos, float ditherAlpha)
        {
            static const float4 thresholds[4] =
            {
                float4(01.0 / 17.0, 09.0 / 17.0, 03.0 / 17.0, 11.0 / 17.0),
                float4(13.0 / 17.0, 05.0 / 17.0, 15.0 / 17.0, 07.0 / 17.0),
                float4(04.0 / 17.0, 12.0 / 17.0, 02.0 / 17.0, 10.0 / 17.0),
                float4(16.0 / 17.0, 08.0 / 17.0, 14.0 / 17.0, 06.0 / 17.0)
            };

            uint xIndex = fmod(pos.x - 0.5, 4);
            uint yIndex = fmod(pos.y - 0.5, 4);
            clip(ditherAlpha - thresholds[yIndex][xIndex]);
        }

        PixelGBufferOutput frag(Varyings input)
        {
            DoDitherAlphaEffect(input.positionCS, _DitherAlpha);

            PixelGBufferOutput output;
            output.GBuffer0.xyz = (_DiffuseMap.Sample(sampler_DiffuseMap, input.uv) * _DiffuseAlbedo).xyz;
            output.GBuffer0.w = 1 - _Roughness;
            output.GBuffer1.xyz = normalize(input.normalWS) * 0.5 + 0.5;
            output.GBuffer1.w = 0;
            output.GBuffer2.xyz = _FresnelR0;
            output.GBuffer2.w = 0;
            output.GBuffer3.x = input.positionCS.z;
            output.GBuffer3.yzw = 0;
            return output;
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
