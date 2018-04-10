#include "Defines.hlsl"

VS_SHADOW_OUTPUT VShader(VS_INPUT input)
{
    VS_SHADOW_OUTPUT output;
    float4 pos = float4(input.position, 1.0f);
    float4 worldPos = mul(pos, modelMatrix);
    //output.rti = 0;
    output.vpi = 0;
    output.positionWS = worldPos.xyz;
    pos = mul(pos, mul(modelMatrix, mul(viewMatrix, projectionMatrix)));
	
    output.positionVS = pos;

    return output;
}