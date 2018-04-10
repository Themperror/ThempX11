#pragma pack_matrix( row_major )


struct VS_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float2 uv : UV0;
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
    output.uv = float2(input.uv.x, 1 - input.uv.y);
    output.positionVS = float4(input.position, 1.0f);
    return output;
}
