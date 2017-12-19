#include "Defines.hlsl"



VS_OUTPUT VShader(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 pos = float4(input.position, 1.0f);
    float4x4 viewProj = mul(viewMatrix, projectionMatrix);
    float4 worldPos = mul(pos,modelMatrix);
	output.positionWS = worldPos.xyz;
	//pos = mul(worldPos, viewMatrix);
    pos = mul(worldPos,viewProj);

	float4 norm = float4(input.normal, 1.0f);

	norm = mul(norm, modelMatrix);

	output.normalVS = normalize(norm.xyz);
	
	output.uv = input.uv;
	output.positionVS = pos;

	output.tangentVS = float3(0, 0, 0);
	output.bitangentVS = float3(0, 0, 0);

	return output;
}