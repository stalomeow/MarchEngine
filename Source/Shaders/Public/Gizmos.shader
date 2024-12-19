Shader "Gizmos"
{
    HLSLINCLUDE
    #pragma target 6.0
    #pragma vs vert
    #pragma ps frag

    #include "Includes/Common.hlsl"

    struct Attributes
    {
        float3 positionOS : POSITION;
        float4 color : COLOR;
        uint instanceID : SV_InstanceID;
    };

    struct Varyings
    {
        float4 positionCS : SV_Position;
        float4 color : COLOR;
    };

    Varyings vert(Attributes input)
    {
        float3 positionWS = TransformObjectToWorld(input.instanceID, input.positionOS);

        Varyings output;
        output.positionCS = TransformWorldToHClip(positionWS);
        output.color = input.color;
        return output;
    }
    ENDHLSL

    Cull Off
    ZWrite Off
    Blend SrcAlpha OneMinusSrcAlpha, One Zero

    Pass
    {
        Name "GizmosVisible"

        ZTest Less

        HLSLPROGRAM
        float4 frag(Varyings input) : SV_Target
        {
            return input.color;
        }
        ENDHLSL
    }

    Pass
    {
        Name "GizmosInvisible"

        ZTest GEqual

        HLSLPROGRAM
        float4 frag(Varyings input) : SV_Target
        {
            return float4(input.color.rgb, input.color.a * 0.5);
        }
        ENDHLSL
    }
}
