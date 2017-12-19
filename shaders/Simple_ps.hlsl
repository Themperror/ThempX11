#include "Defines.hlsl"


Texture2D shaderTexture;
SamplerState SampleType;
float4 PShader(VS_OUTPUT input) : SV_TARGET
{
	float4 textureColor = shaderTexture.Sample(SampleType, input.uv).zyxw;
    return textureColor;
}