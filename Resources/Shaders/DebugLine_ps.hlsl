#include "Defines.hlsl"

struct VS_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float3 positionWS : POSITION;
    float3 color : COLOR;
};
struct PS_OUTPUT
{
    float4 diffuse : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 F0 : SV_TARGET2;
    float4 misc : SV_TARGET3;
    float4 pos : SV_TARGET4;
};

[earlydepthstencil]
PS_OUTPUT PShader(VS_OUTPUT input)
{
	PS_OUTPUT output;
	output.pos = float4(input.positionWS, 1.0f);
    output.diffuse = float4(input.color,0.0);
    output.normal = float4(normalize(float3(0, 1, 0)), 0);
    output.F0 = float4(input.color,0.0);
    output.misc = float4(0, 0, 1, 1);

    return output;
}