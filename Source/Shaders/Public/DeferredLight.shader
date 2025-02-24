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

        Texture2D _AOMap;
        SamplerState sampler_AOMap;

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

            BRDFData brdfData;
            brdfData.albedo = gbuffer.albedo;
            brdfData.metallic = gbuffer.metallic;
            brdfData.a2 = RoughnessToAlpha2(gbuffer.roughness);

            float3 positionWS = ComputeWorldSpacePosition(input.uv, gbuffer.depth);
            float3 viewDirWS = normalize(_CameraPositionWS.xyz - positionWS);

            float shadow = SampleScreenSpaceShadowMap(input.uv);
            float occlusion = min(_AOMap.Sample(sampler_AOMap, input.uv).r, gbuffer.occlusion);
            float3 color = FragmentPBR(brdfData, positionWS, gbuffer.normalWS, viewDirWS, gbuffer.emission, occlusion);
            return float4(color * lerp(0.05, 1, shadow), 1.0);
        }
        ENDHLSL
    }
}
