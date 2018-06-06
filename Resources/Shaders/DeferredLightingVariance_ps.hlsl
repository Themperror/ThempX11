#include "Defines.hlsl"
#include "Functions.hlsl"
#include "Structs.hlsl"


float4 PShader(VS_OUTPUT input) : SV_TARGET
{
    float4 finalResult = float4(0.5, 0.5, 0.5, 0);
    
    return finalResult;
}