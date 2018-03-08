#include "Defines.hlsl"

//Texture2D shaderTexture[4];
Texture2D diff : register(t0);
Texture2D norm : register(t1);
Texture2D rough : register(t2);
Texture2D misc : register(t3);

SamplerState diffSampler : register(s0);
SamplerState normSampler : register(s1);
SamplerState specSampler : register(s2);
SamplerState miscSampler : register(s3);

[earlydepthstencil]
PS_OUTPUT PShader(VS_OUTPUT input)
{
	PS_OUTPUT output;
	output.pos = float4(input.positionWS, 1.0f);
	output.diffuse = diff.Sample(diffSampler, input.uv);
	if (_hasNormal)
	{
		float3 normal = norm.Sample(normSampler, input.uv).xyz;
		normal = ExpandNormal(normal);

		float3x3 BTN = float3x3(normalize(input.tangentVS),
			normalize(input.bitangentVS),
			normalize(input.normalVS));
		// Transform normal from tangent space to view space.
		normal = mul(normal, BTN);

		output.normal = normalize(float4(normal, 0));
	}
	else
	{
		output.normal = float4(normalize(input.normalVS),0);
		//output.normal = float4(0,0,0,0);
	}


	//x = roughness, y = metallic, zw unused;
    float4 PBRData;
    float roughnessVal = 0.0;
    float metallicVal = 0.0;
	if (_hasRoughness)
	{
        PBRData = rough.Sample(normSampler, input.uv);
        roughnessVal = clamp(PBRData.x, 0.02, 0.98);
        metallicVal = PBRData.y;
    }
	else
	{
        roughnessVal = clamp(_Roughness, 0.02, 0.98);
        metallicVal = _Metallic;
    }
    output.F0 = float4(lerp(output.diffuse.xyz, float3(_F0, _F0, _F0), metallicVal),1.0);
	//if(metallicVal > 0.0)
    //{
    //    output.F0 = output.diffuse;
    //}
	//else
    //{
    //    output.F0 = float4(float3(_F0, _F0, _F0), 1.0);
    //}
    output.misc = float4(roughnessVal, metallicVal, _EmissiveStrength, _isEmissive);
    output.pbrData = float4(1.0, 1.0, 1.0, 1.0);

    return output;
}