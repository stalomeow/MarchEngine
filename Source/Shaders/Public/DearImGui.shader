Shader "DearImGui"
{
    Pass
    {
        Name "Draw"

        Cull Off
        ZTest Disabled
        ZWrite Off

        // Ref: https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_dx12.cpp
        Blend SrcAlpha OneMinusSrcAlpha, One OneMinusSrcAlpha
        BlendOp Add, Add

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Includes/ColorSpace.hlsl"

        cbuffer cbImGui
        {
            float4x4 _MatrixMVP;
        };

        struct Attributes
        {
            float2 positionOS : POSITION;
            float4 color : COLOR0;
            float2 uv  : TEXCOORD0;
        };

        struct Varyings
        {
            float4 positionCS : SV_POSITION;
            float4 color : COLOR0;
            float2 uv  : TEXCOORD0;
        };

        Texture2D _Texture;
        SamplerState sampler_Texture;

        Varyings vert(Attributes input)
        {
            Varyings output;
            output.positionCS = mul(_MatrixMVP, float4(input.positionOS, 0, 1));
            output.color = input.color;
            output.uv  = input.uv;
            return output;
        }

        float4 frag(Varyings input) : SV_Target
        {
            float4 color = _Texture.Sample(sampler_Texture, input.uv);

            #ifndef MARCH_COLORSPACE_GAMMA
                color = LinearToSRGB(color);
            #endif

            return input.color * color;
        }
        ENDHLSL
    }

    Pass
    {
        Name "Blit"

        Cull Off
        ZTest Always
        ZWrite Off

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Includes/Common.hlsl"
        #include "Includes/ColorSpace.hlsl"

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float2 uv : TEXCOORD0;
        };

        Texture2D _Texture;
        SamplerState sampler_Texture;

        Varyings vert(uint vertexID : SV_VertexID)
        {
            Varyings output;
            output.positionCS = GetFullScreenTriangleVertexPositionCS(vertexID);
            output.uv = GetFullScreenTriangleTexCoord(vertexID);
            return output;
        }

        float4 frag(Varyings input) : SV_Target
        {
            float4 color = _Texture.Sample(sampler_Texture, input.uv);

            #ifndef MARCH_COLORSPACE_GAMMA
                color = SRGBToLinear(color);
            #endif

            return color;
        }
        ENDHLSL
    }
}
