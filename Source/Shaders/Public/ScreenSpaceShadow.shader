Shader "ScreenSpaceShadow"
{
    Pass
    {
        Name "Shadow"

        Cull Off
        ZTest Always
        ZWrite Off

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Includes/Common.hlsl"
        #include "Includes/GBuffer.hlsl"
        #include "Includes/Shadow.hlsl"

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float2 uv : TEXCOORD0;
        };

        Varyings vert(uint vertexID : SV_VertexID)
        {
            Varyings output;
            output.positionCS = GetFullScreenTriangleVertexPositionCS(vertexID);
            output.uv = GetFullScreenTriangleTexCoord(vertexID);
            return output;
        }

        float4 frag(Varyings input) : SV_Target
        {
            GBufferData gbuffer = LoadGBufferData(input.positionCS.xy);

            float3 positionWS = ComputeWorldSpacePosition(input.uv, gbuffer.depth);
            float4 shadowCoord = TransformWorldToShadowCoord(positionWS);
            return SampleShadowMap(shadowCoord);
        }
        ENDHLSL
    }
}
