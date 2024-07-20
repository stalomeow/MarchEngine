cbuffer cbPerObject : register(b0)
{
    float4x4 _MatrixMVP;
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
    output.positionCS = mul(_MatrixMVP, float4(input.positionOS, 1.0));
    output.color = input.color;
    return output;
}

float4 frag(Varyings input) : SV_Target
{
    return input.color;
}
