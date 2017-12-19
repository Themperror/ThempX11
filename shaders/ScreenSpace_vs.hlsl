#include "Defines.hlsl"



VS_OUTPUT VShader(VS_INPUT input)
{
	VS_OUTPUT output;	
	output.normalVS = input.normal;
	output.uv = input.uv;
	output.positionVS = float4(input.position,1.0f);
	output.positionWS = float3(0, 0, 0);
	output.tangentVS= float3(0, 0, 0);
	output.bitangentVS= float3(0, 0, 0);

	return output;
}