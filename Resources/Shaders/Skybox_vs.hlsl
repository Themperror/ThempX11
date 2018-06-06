#include "Defines.hlsl"

cbuffer ObjectBuffer : register(b0)
{
    float4x4 modelMatrix; //64
};
cbuffer CameraBuffer : register(b1)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 invProjectionMatrix;
    float4x4 invViewMatrix;
    float4 cameraPosition;
    float4 cameraDir;
};
struct VS_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float3 normalWS : NORMAL0;
    float3 positionWS : POSITION;
    float3 uv : UV0;
};
struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float2 uv : UV;
};

VS_OUTPUT VShader(VS_INPUT input)
{
	VS_OUTPUT output;
	float4 pos = float4(input.position, 1.0f);
    output.positionWS = mul(pos,modelMatrix).xyz;
    pos = mul(pos, mul(modelMatrix, mul(viewMatrix, projectionMatrix))).xyww; //XYWW instead of XYZW so that its always at max depth
	//pos = mul(pos, projectionMatrix);
	
    output.normalWS = mul(input.normal, (float3x3) modelMatrix);

	output.uv = input.position;
	output.positionVS = pos;

	return output;
}