#include "Structs.hlsl"

VS_OUTPUT VShader(VS_INPUT input)
{
	VS_OUTPUT output;	
	output.normalWS = input.normal;
	output.uv = float2(input.uv.x,1-input.uv.y);
	output.positionVS = float4(input.position,1.0f);
	output.positionWS = float3(0, 0, 0);
    output.bitangentWS = float3(0, 0, 0);
    output.tangentWS = float3(0, 0, 0);
	return output;
}