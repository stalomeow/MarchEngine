Shader "BlitImGui"
{
    Pass
    {
        Name "BlitImGui"

        Cull Off
        ZTest Always
        ZWrite Off

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Common.hlsl"
        #include "ColorSpace.hlsl"

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float2 uv : TEXCOORD0;
        };

        Texture2D _SrcTex;
        SAMPLER(_SrcTex);

        Varyings vert(uint vertexID : SV_VertexID)
        {
            Varyings output;
            output.positionCS = GetFullScreenTriangleVertexPositionCS(vertexID);
            output.uv = GetFullScreenTriangleTexCoord(vertexID);
            return output;
        }

        float4 frag(Varyings input) : SV_Target
        {
            float4 c = _SrcTex.Sample(sampler_SrcTex, input.uv);

            #ifdef MARCH_COLORSPACE_GAMMA
                return c;
            #else
                return SRGBToLinear(c);
            #endif
        }
        ENDHLSL
    }
}
