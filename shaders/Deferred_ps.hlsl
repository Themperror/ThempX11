#include "Defines.hlsl"

//Texture2D shaderTexture[4];
Texture2D diff : register(t0);
Texture2D norm : register(t1);
Texture2D spec : register(t2);
Texture2D misc : register(t3);
Texture2D pos : register(t4);

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
	if (hasNormal)
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

	if (hasSpecular)
	{
		output.specular = spec.Sample(specSampler, input.uv);
	}
	else
	{
		output.specular = float4(0.5, 0.5, 0.5, 1.0f);
	}

	if (hasMisc)
	{
		output.misc = misc.Sample(miscSampler, input.uv);
	}
	else
	{
		output.misc = float4(0, 0, 0, 1);
	}

    return output;
}