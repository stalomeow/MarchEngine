#include "Lighting.hlsl"

cbuffer cbObject : register(b0)
{
    float4x4 _MatrixWorld;
};

cbuffer cbMaterial : register(b1)
{
    float4 _DiffuseAlbedo;
    float3 _FresnelR0;
    float _Roughness;
};

cbuffer cbPass : register(b2)
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

struct Attributes
{
    float3 positionOS : POSITION;
    float3 normalOS : NORMAL;
};

struct Varyings
{
    float4 positionCS : SV_Position;
    float3 normalWS : NORMAL;
    float3 positionWS : TEXCOORD0;
};

Varyings vert(Attributes input)
{
    float4 positionWS = mul(_MatrixWorld, float4(input.positionOS, 1.0));

    Varyings output;
    output.positionCS = mul(_MatrixViewProjection, positionWS);
    output.normalWS = mul(_MatrixWorld, float4(input.normalOS, 0.0)).xyz; // assume uniform scale
    output.positionWS = positionWS.xyz;
    return output;
}

float4 frag(Varyings input) : SV_Target
{
    Light lights[MAX_LIGHT_COUNT];
    GetLights(input.positionWS, _LightData, _LightCount, lights);

    float3 color = 0;
    float3 viewDirWS = normalize(_CameraPositionWS.xyz - input.positionWS);
    float shininess = 1 - _Roughness;

    for (int i = 0; i < _LightCount; i++)
    {
        color += BlinnPhong(lights[i], normalize(input.normalWS), viewDirWS, _DiffuseAlbedo.rgb, _FresnelR0, shininess);
    }

    return float4(color, _DiffuseAlbedo.a);
}
