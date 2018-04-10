
#pragma pack_matrix( row_major )
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
struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float2 uv : UV;
};
struct VS_SHADOW_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float3 positionWS : POSITION;
};
VS_SHADOW_OUTPUT VShader(VS_INPUT input)
{
    VS_SHADOW_OUTPUT output;
	float4 pos = float4(input.position, 1.0f);
	float4 worldPos = mul(pos, modelMatrix);
    output.positionWS = worldPos.xyz;
    pos = mul(pos, mul(modelMatrix, mul(viewMatrix, projectionMatrix)));
	
	output.positionVS = pos;

	return output;
}