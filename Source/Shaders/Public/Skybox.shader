Shader "Skybox"
{
    Properties
    {
        _Cubemap("Skybox", Cube) = "" {}
    }

    Pass
    {
        Cull Off
        ZTest LEqual
        ZWrite Off

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Includes/Common.hlsl"

        TextureCube _Cubemap;
        SamplerState sampler_Cubemap;

        struct Attributes
        {
            float3 positionOS : POSITION;
        };

        struct Varyings
        {
            float4 positionCS : SV_POSITION;
            float3 uv         : TEXCOORD0;
        };

        Varyings vert(Attributes input)
        {
            Varyings output;
            float3 positionWS = _CameraPositionWS.xyz + input.positionOS; // 相机为天空盒的中心
            output.positionCS = TransformWorldToHClip(positionWS);
            output.positionCS.z = MARCH_FAR_CLIP_VALUE * output.positionCS.w; // 放在远平面
            output.uv = normalize(input.positionOS);
            return output;
        }

        float4 frag(Varyings input) : SV_TARGET
        {
            return _Cubemap.Sample(sampler_Cubemap, input.uv);
        }
        ENDHLSL
    }
}
