Shader "Gizmos"
{
    Pass
    {
        Name "GizmosVisible"

        Cull Off
        ZTest Less
        ZWrite Off

        Blend 0 SrcAlpha OneMinusSrcAlpha, One Zero

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Includes/Common.hlsl"

        struct Attributes
        {
            float3 positionWS : POSITION;
            float4 color : COLOR;
        };

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float4 color : COLOR;
        };

        Varyings vert(Attributes input)
        {
            Varyings output;
            output.positionCS = TransformWorldToHClip(input.positionWS);
            output.color = input.color;
            return output;
        }

        float4 frag(Varyings input) : SV_Target
        {
            return input.color;
        }
        ENDHLSL
    }

    Pass
    {
        Name "GizmosInvisible"

        Cull Off
        ZTest GEqual
        ZWrite Off

        Blend 0 SrcAlpha OneMinusSrcAlpha, One Zero

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Includes/Common.hlsl"

        struct Attributes
        {
            float3 positionWS : POSITION;
            float4 color : COLOR;
        };

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float4 color : COLOR;
        };

        Varyings vert(Attributes input)
        {
            Varyings output;
            output.positionCS = TransformWorldToHClip(input.positionWS);
            output.color = input.color;
            return output;
        }

        float4 frag(Varyings input) : SV_Target
        {
            return float4(input.color.rgb, input.color.a * 0.5);
        }
        ENDHLSL
    }
}
