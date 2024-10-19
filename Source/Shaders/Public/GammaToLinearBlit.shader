Shader "GammaToLinearBlit"
{
    Pass
    {
        Name "GammaToLinearBlit"

        Cull Off
        ZTest Always
        ZWrite Off

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Common.hlsl"

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float2 uv : TEXCOORD0;
        };

        Texture2D _SrcTex;
        SamplerState sampler_SrcTex;

        Varyings vert(uint vertexID : SV_VertexID)
        {
            Varyings output;
            output.positionCS = GetFullScreenTriangleVertexPositionCS(vertexID);
            output.uv = GetFullScreenTriangleTexCoord(vertexID);
            return output;
        }

        float4 GammaToLinear(float4 c)
        {
            return float4(pow(c.rgb, 2.2), c.a);
        }

        float4 frag(Varyings input) : SV_Target
        {
            float4 c = _SrcTex.Sample(sampler_SrcTex, input.uv);
            return GammaToLinear(c);
        }
        ENDHLSL
    }
}
