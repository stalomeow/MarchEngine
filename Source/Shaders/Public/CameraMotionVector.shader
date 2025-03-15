Shader "CameraMotionVector"
{
    Pass
    {
        Name "MotionVector"

        Cull Off
        ZTest LEqual
        ZWrite Off
        ColorMask RG

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Includes/Common.hlsl"

        struct Varyings
        {
            float4 positionCS : SV_Position;
            float2 uv : TEXCOORD0;
        };

        Varyings vert(uint vertexID : SV_VertexID)
        {
            Varyings output;
            output.positionCS = GetFullScreenTriangleVertexPositionCS(vertexID, MARCH_FAR_CLIP_VALUE);
            output.uv = GetFullScreenTriangleTexCoord(vertexID);
            return output;
        }

        float2 frag(Varyings input) : SV_Target
        {
            // 在远平面处绘制一个全屏三角形，补充空白部分的 Motion Vector，避免天空盒在 TAA 后出现 Ghosting

            float3 positionWS = ComputeWorldSpacePosition(input.uv, MARCH_FAR_CLIP_VALUE);
            float4 positionCSCurrNonJittered = TransformWorldToHClipNonJittered(positionWS);
            float4 positionCSPrevNonJittered = TransformWorldToHClipNonJitteredLastFrame(positionWS);
            float2 curr = GetScreenUVFromNDC(positionCSCurrNonJittered.xy / positionCSCurrNonJittered.w);
            float2 prev = GetScreenUVFromNDC(positionCSPrevNonJittered.xy / positionCSPrevNonJittered.w);
            return curr - prev;
        }
        ENDHLSL
    }
}
