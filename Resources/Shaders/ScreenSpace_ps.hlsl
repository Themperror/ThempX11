#include "Defines.hlsl"

struct VS_OUTPUT
{
    float4 positionVS : SV_POSITION;
};
struct PS_OUTPUT
{
    float depth : SV_Depth;
};
PS_OUTPUT PShader(VS_OUTPUT input)
{
    PS_OUTPUT output;
    //tput.col = float4(0, 0, 0, 0);
    output.depth = 1.0;
    return output;
}
