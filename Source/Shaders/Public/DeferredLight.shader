Shader "DeferredLight"
{
    Pass
    {
        Name "Lit"

        Cull Off
        ZTest Always
        ZWrite Off

        Stencil
        {
            Ref 1

            CompFront Equal
            PassFront Keep
            FailFront Keep
            ZFailFront Keep

            CompBack Equal
            PassBack Keep
            FailBack Keep
            ZFailBack Keep
        }

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Includes/Common.hlsl"
        #include "Includes/GBuffer.hlsl"
        #include "Includes/Lighting.hlsl"

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
            int3 location = int3(input.positionCS.xy, 0);
            GBufferData gbuffer = LoadGBufferData(location);

            float4 ndc = float4(input.uv.x, 1 - input.uv.y, gbuffer.depth, 1);
            ndc.xy = ndc.xy * 2 - 1;
            float4 positionWS = mul(_MatrixInvViewProjection, ndc);
            positionWS /= positionWS.w;

            float3 viewDirWS = normalize(_CameraPositionWS.xyz - positionWS.xyz);

            float3 color = BlinnPhong(positionWS.xyz, gbuffer.normalWS, viewDirWS, gbuffer.albedo, gbuffer.fresnelR0, gbuffer.shininess);
            return float4(color, 1.0);
        }
        ENDHLSL
    }
}
