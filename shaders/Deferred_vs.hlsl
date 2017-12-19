#include "Defines.hlsl"


VS_OUTPUT VShader(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 pos = float4(input.position, 1.0f);
    output.positionWS = mul(pos,modelMatrix);
    pos = mul(pos, mul(modelMatrix, mul(viewMatrix, projectionMatrix)));
	//pos = mul(pos, projectionMatrix);
    

    output.tangentVS = mul(input.tangent, (float3x3) modelMatrix);
    output.bitangentVS = mul(input.bitangent, (float3x3) modelMatrix);
    output.normalVS = mul(input.normal, (float3x3) modelMatrix);




	output.uv = input.uv;
	output.positionVS = pos;

	return output;
}