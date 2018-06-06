#include "Defines.hlsl"
#include "Functions.hlsl"
#include "Structs.hlsl"

//Texture2D shaderTexture[4];
Texture2D diff : register(t0);
Texture2D norm : register(t1);
Texture2D PBR : register(t2);
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

        float3x3 BTN = float3x3(normalize(input.tangentWS), normalize(input.bitangentWS), normalize(input.normalWS));
		//Transform normal from tangent space to view space.
        normal = mul(normal, BTN);

        output.normal = normalize(float4(normal, 0));
    }
    else
    {
        output.normal = float4(normalize(input.normalWS), 0);
    }


	//x = roughness, y = metallic, zw unused;
    float4 PBRData;
    float roughnessVal = 0.0;
    float metallicVal = 0.0;
    float AO = 1.0;
    if (_hasRoughness)
    {
        PBRData = PBR.Sample(normSampler, input.uv);
        roughnessVal = clamp(PBRData.x, 0.02, 0.98);
        metallicVal = PBRData.y;
        AO = PBRData.z > 0.01 ? PBRData.z : 1.0;

    }
    else
    {
        roughnessVal = clamp(_Roughness, 0.02, 0.98);
        metallicVal = _Metallic;
    }
    output.F0 = float4(lerp(float3(0.04, 0.04, 0.04), output.diffuse.xyz, metallicVal), AO);
    output.diffuse.xyz *= AO;
	//if(metallicVal > 0.0)
    //{
    //    output.F0 = output.diffuse;
    //}
	//else
    //{
    //    output.F0 = float4(float3(_F0, _F0, _F0), 1.0);
    //}
    output.misc = float4(roughnessVal, metallicVal, _EmissiveStrength, _isEmissive);

    return output;
}