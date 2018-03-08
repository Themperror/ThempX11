#include "Defines.hlsl"

Texture2D shaderTexture[4];


SamplerState SampleType[4];
float4 PShader(VS_OUTPUT input) : SV_TARGET
{
	float4 textureColor = float4(0.0,0.0,0.0,0.0);// = shaderTexture[0].Sample(SampleType[0], uv);
	//textureColor *= shaderTexture[1].Sample(SampleType[1], uv);
	//textureColor *= shaderTexture[2].Sample(SampleType[2], uv);
	//textureColor *= shaderTexture[3].Sample(SampleType[3], uv);

	if (_visualType == 0.0) //default, proper rendering
	{
		//float lightRange = 0.03;
		//float lightStr = saturate(-dot(input.normal, normalize(input.worldPos - lightPos)));
		//float dist = distance(input.worldPos.xyz, lightPos.xyz)*lightRange;
		//
		//float b = -0.7; //base modifier
		//float d = -0.4; //distance modifier
		//
		//float distLight = (1 - saturate(b + exp(dist + d)));
		//float3 Lighting = float3(distLight, distLight, distLight) * lightStr;

		
		//textureColor = float4(diffuse.xyz * Lighting,1);
		float4 diffuse = shaderTexture[0].Sample(SampleType[0], input.uv);
		textureColor = diffuse;

	}
	else if(_visualType == 1.0) //only show diffuse
	{
		textureColor = shaderTexture[0].Sample(SampleType[0], input.uv);
	}
	else if (_visualType == 2.0) //only show normal maps
	{
		textureColor = shaderTexture[1].Sample(SampleType[1], input.uv);
	}
	else if (_visualType == 3.0)//only show specular maps
	{
		textureColor = shaderTexture[2].Sample(SampleType[2], input.uv);
	}
	else if (_visualType == 4.0)//only show misc / alpha maps
	{
		textureColor = shaderTexture[3].Sample(SampleType[3], input.uv);
	}
	else if (_visualType == 5.0)//only show UV's
	{
		textureColor.xyz = float3(fmod(input.uv, 1.0f), 0.0); //fmodded to show wrapping textures (keeping values in 0-1 range)
	}
	else if (_visualType == 6.0) //only show normals
	{
		textureColor.xyz = input.normalVS;
	}
	//textureColor.xyz = float3(visualType / 5.0, visualType / 5.0, visualType / 5.0);
	textureColor.w = 1.0f;
    return textureColor;
}