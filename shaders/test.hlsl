cbuffer cbPerObject : register(b0)
{
    float4x4 _MatrixWorld;
};

cbuffer cbPerDraw : register(b1)
{
    float4x4 _MatrixView;
    float4x4 _MatrixProjection;
    float4x4 _MatrixViewProjection;
    float4x4 _MatrixInvView;
    float4x4 _MatrixInvProjection;
    float4x4 _MatrixInvViewProjection;
    float4 _Time;
};

struct Attributes
{
    float3 positionOS : POSITION;
    float4 color : COLOR;
};

struct Varyings
{
    float4 positionCS : SV_Position;
    float4 color : COLOR;
};

Varyings vert(Attributes input)
{
    Varyings output;
    float4 positionWS = mul(_MatrixWorld, float4(input.positionOS, 1.0));
    output.positionCS = mul(_MatrixViewProjection, positionWS);
    output.color = input.color;
    return output;
}

float4 frag(Varyings input) : SV_Target
{
    return input.color;
}
