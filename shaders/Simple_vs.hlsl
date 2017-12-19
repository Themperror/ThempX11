#include "Defines.hlsl"


VS_OUTPUT VShader(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 pos = float4(input.position, 1.0f);

	pos = mul(pos, modelMatrix);
	pos = mul(pos, viewMatrix);
	pos = mul(pos, projectionMatrix);
	
	float4 norm = float4(input.normal, 1.0f);

	norm = mul(norm, modelMatrix);

	output.normalVS = normalize(norm.xyz);
	
	output.uv = input.uv;
	output.positionVS = pos;
	output.positionWS = pos.xyz;

	output.tangentVS = float3(0, 0, 0);
	output.bitangentVS = float3(0, 0, 0);

	return output;
}