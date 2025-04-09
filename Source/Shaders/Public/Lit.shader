Shader "Lit"
{
    Properties
    {
        _BaseMap("Albedo", 2D) = "white" {}
        _BaseColor("Color", Color) = (1, 1, 1, 1)

        [Range(0, 1)] _Metallic("Metallic", Float) = 1.0
        [Range(0, 1)] _Roughness("Roughness", Float) = 1.0
        _MetallicRoughnessMap("Metallic Roughness Map", 2D) = "white" {} // G: Roughness, B: Metallic

        _BumpMap("Bump Map", 2D) = "bump" {}
        _BumpScale("Bump Scale", Float) = 1.0

        [Range(0, 1)] _OcclusionStrength("Occlusion Strength", Float) = 1.0
        _OcclusionMap("Occlusion Map", 2D) = "white" {}

        [HDR] _EmissionColor("Emission Color", Color) = (0, 0, 0, 1)
        _EmissionMap("Emission Map", 2D) = "white" {}

        [Range(0, 1)] _Cutoff("Alpha Cutoff", Float) = 0.5
        _CullMode("Cull Mode", Int) = 2
    }

    HLSLINCLUDE
    #include "Includes/Common.hlsl"
    ENDHLSL

    Pass
    {
        Name "GBuffer"

        Cull [_CullMode]
        ZTest Less
        ZWrite On

        Tags
        {
            "LightMode" = "GBuffer"
        }

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

        #include "Includes/GBuffer.hlsl"

        struct Attributes
        {
            float3 positionOS : POSITION;
            float3 normalOS : NORMAL;
            float4 tangentOS : TANGENT;
            float2 uv : TEXCOORD0;
            uint instanceID : SV_InstanceID;
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
            float3 positionWS = TransformObjectToWorld(input.instanceID, input.positionOS);

            Varyings output;
            output.positionCS = TransformWorldToHClip(positionWS);
            output.normalWS = TransformObjectToWorldNormal(input.instanceID, input.normalOS);
            output.tangentWS.xyz = TransformObjectToWorldDir(input.instanceID, input.tangentOS.xyz);
            output.tangentWS.w = input.tangentOS.w  * GetOddNegativeScale(input.instanceID);
            output.uv = input.uv;
            return output;
        }

        PixelGBufferOutput frag(Varyings input)
        {
            float4 albedo = _BaseMap.Sample(sampler_BaseMap, input.uv) * _BaseColor;

            #ifdef _ALPHATEST_ON
                clip(albedo.a - _Cutoff);
            #endif

            float3 normalTS = _BumpMap.Sample(sampler_BumpMap, input.uv).xyz * 2.0 - 1.0;
            normalTS.xy *= _BumpScale;
            normalTS = normalize(normalTS);
            float3 N = normalize(input.normalWS);
            float3 T = normalize(input.tangentWS.xyz - dot(input.tangentWS.xyz, N) * N);
            float3 B = cross(N, T) * input.tangentWS.w;
            float3 bumpedNomalWS = normalize(mul(normalTS, float3x3(T, B, N))); // float3x3() 是行主序矩阵

            float4 metallicRoughness = _MetallicRoughnessMap.Sample(sampler_MetallicRoughnessMap, input.uv);
            float metallic = metallicRoughness.b * _Metallic;
            float roughness = metallicRoughness.g * _Roughness;

            float occlusion = _OcclusionMap.Sample(sampler_OcclusionMap, input.uv).r;
            occlusion = lerp(1.0, occlusion, _OcclusionStrength);

            float4 emission = _EmissionMap.Sample(sampler_EmissionMap, input.uv) * _EmissionColor;

            GBufferData data;
            data.albedo = albedo.rgb;
            data.metallic = metallic;
            data.roughness = roughness;
            data.normalWS = bumpedNomalWS;
            data.emission = emission.rgb;
            data.occlusion = occlusion;
            return PackGBufferData(data);
        }
        ENDHLSL
    }

    Pass
    {
        Name "MotionVector"

        Cull [_CullMode]
        ZTest Equal
        ZWrite Off
        ColorMask RG

        Tags
        {
            "LightMode" = "MotionVector"
        }

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #pragma multi_compile _ _ALPHATEST_ON

        struct Attributes
        {
            float3 positionOS : POSITION;
            float2 uv : TEXCOORD0;
            uint instanceID : SV_InstanceID;
        };

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float2 uv : TEXCOORD0;
            float4 positionCSCurrNonJittered : TEXCOORD1;
            float4 positionCSPrevNonJittered : TEXCOORD2;
        };

        Varyings vert(Attributes input)
        {
            float3 positionWSCurr = TransformObjectToWorld(input.instanceID, input.positionOS);
            float3 positionWSPrev = TransformObjectToWorldLastFrame(input.instanceID, input.positionOS);

            Varyings output;
            output.positionCS = TransformWorldToHClip(positionWSCurr);
            output.uv = input.uv;
            output.positionCSCurrNonJittered = TransformWorldToHClipNonJittered(positionWSCurr);
            output.positionCSPrevNonJittered = TransformWorldToHClipNonJitteredLastFrame(positionWSPrev);
            return output;
        }

        float2 frag(Varyings input) : SV_Target
        {
            float4 albedo = _BaseMap.Sample(sampler_BaseMap, input.uv) * _BaseColor;

            #ifdef _ALPHATEST_ON
                clip(albedo.a - _Cutoff);
            #endif

            float2 curr = GetScreenUVFromNDC(input.positionCSCurrNonJittered.xy / input.positionCSCurrNonJittered.w);
            float2 prev = GetScreenUVFromNDC(input.positionCSPrevNonJittered.xy / input.positionCSPrevNonJittered.w);
            return curr - prev;
        }
        ENDHLSL
    }

    Pass
    {
        Name "ShadowCaster"

        Cull [_CullMode]
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
        #pragma ps frag

        #pragma multi_compile _ _ALPHATEST_ON

        struct Attributes
        {
            float3 positionOS : POSITION;
            float2 uv : TEXCOORD0;
            uint instanceID : SV_InstanceID;
        };

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float2 uv : TEXCOORD0;
        };

        Varyings vert(Attributes input)
        {
            float3 positionWS = TransformObjectToWorld(input.instanceID, input.positionOS);

            Varyings output;
            output.positionCS = TransformWorldToHClip(positionWS);
            output.uv = input.uv;
            return output;
        }

        void frag(Varyings input)
        {
            float4 albedo = _BaseMap.Sample(sampler_BaseMap, input.uv) * _BaseColor;

            #ifdef _ALPHATEST_ON
                clip(albedo.a - _Cutoff);
            #endif
        }
        ENDHLSL
    }
}
