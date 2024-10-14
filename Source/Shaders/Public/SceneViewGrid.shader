Shader "SceneViewGrid"
{
    Properties
    {
        _XAxisColor("X Axis Color", Color) = (1, 0, 0, 0.5)
        _ZAxisColor("Z Axis Color", Color) = (0, 0, 1, 0.5)
        _LineColor("Line Color", Color) = (0.5, 0.5, 0.5, 0.5)
        [Range(0, 1)] _Antialiasing("Anti-aliasing", Float) = 0.5
        [Range(0, 1)] _FadeOut("Fade Out", Float) = 0.8
    }

    Pass
    {
        Name "WorldGrid"

        Cull Off
        ZTest Less
        ZWrite Off

        Blend 0 SrcAlpha OneMinusSrcAlpha, Zero One

        HLSLPROGRAM
        #pragma target 6.0
        #pragma vs vert
        #pragma ps frag

        #include "Common.hlsl"
        #include "Lighting.hlsl"

        cbuffer cbMaterial
        {
            float4 _XAxisColor;
            float4 _ZAxisColor;
            float4 _LineColor;
            float _Antialiasing;
            float _FadeOut;
        };

        cbuffer cbPass
        {
            float4x4 _MatrixView;
            float4x4 _MatrixProjection;
            float4x4 _MatrixViewProjection;
            float4x4 _MatrixInvView;
            float4x4 _MatrixInvProjection;
            float4x4 _MatrixInvViewProjection;
            float4 _Time;
            float4 _CameraPositionWS;

            LightData _LightData[MAX_LIGHT_COUNT];
            int _LightCount;
        };

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

        float3 GetDepthAndWorldPosition(float2 uv, out float depth)
        {
            float4 ndc = float4(uv.x, 1 - uv.y, 0, 1);
            ndc.xy = ndc.xy * 2 - 1;

            float4x4 ivp = _MatrixInvViewProjection;
            ndc.z = dot(ivp[1].xyw, ndc.xyw) / (-ivp[1].z);

            if (ndc.z < 0.0 || ndc.z > 1.0)
            {
                discard;
            }

            depth = ndc.z;

            float4 positionWS = mul(ivp, ndc);
            positionWS /= positionWS.w;
            return positionWS.xyz;
        }

        float4 GetGridColor(float3 positionWS, float level)
        {
            float gridWidth = pow(10, level);
            float2 scaledPos = positionWS.xz / gridWidth;
            float2 diff = fwidth(scaledPos); // 值越大，离得越远
            float2 gridEdge = abs(frac(scaledPos) - 0.5);

            float2 halfLineWidth = (1.0 + _Antialiasing) * diff; // 离得越远越粗
            float2 threshold = 0.5 - halfLineWidth;
            float2 intensity = smoothstep(threshold, 0.5, gridEdge); // 羽化边缘，减少锯齿

            float alpha = max(intensity.x, intensity.y);
            alpha *= pow(saturate(1 - max(diff.x, diff.y)), _FadeOut * 10); // 离得越远越淡

            float4 color;

            if (abs(scaledPos.x) < halfLineWidth.x)
            {
                color = _ZAxisColor;
            }
            else if (abs(scaledPos.y) < halfLineWidth.y)
            {
                color = _XAxisColor;
            }
            else
            {
                color = _LineColor;
            }

            return float4(color.rgb, color.a * alpha);
        }

        float4 frag(Varyings input, out float depth : SV_Depth) : SV_Target
        {
            float3 positionWS = GetDepthAndWorldPosition(input.uv, depth);

            // 划分等级
            // level:      0       1        2         3         ...
            // cameraY: 0 --- 100 --- 1000 --- 10000 --- 100000 --- ...
            float cameraY = abs(_CameraPositionWS.y);
            float level = max(0, floor(log10(cameraY) - 1));
            float pow10Level = pow(10, level);
            float nextHeight = pow10Level * 100;
            float prevHeight = level == 0 ? 0 : (pow10Level * 10);
            float progress = (cameraY - prevHeight) / (nextHeight - prevHeight);

            float4 c1 = GetGridColor(positionWS, level);
            float4 c2 = GetGridColor(positionWS, level + 1);
            return lerp(c1, c2, progress);
        }
        ENDHLSL
    }
}
