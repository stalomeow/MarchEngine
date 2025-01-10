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

            Comp Equal
            Pass Keep
            Fail Keep
            ZFail Keep
        }

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Includes/Common.hlsl"
        #include "Includes/GBuffer.hlsl"
        #include "Includes/Lighting.hlsl"
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
            int3 location = int3(input.positionCS.xy, 0);
            GBufferData gbuffer = LoadGBufferData(location);

            float3 positionWS = ComputeWorldSpacePosition(input.uv, gbuffer.depth);
            float3 viewDirWS = normalize(_CameraPositionWS.xyz - positionWS);

            float shadow = SampleScreenSpaceShadowMap(input.uv);
            float3 color = BlinnPhong(positionWS, gbuffer.normalWS, viewDirWS, gbuffer.albedo, gbuffer.fresnelR0, gbuffer.shininess);
            return float4(color * lerp(0.05, 1, shadow), 1.0);
        }
        ENDHLSL
    }
}
