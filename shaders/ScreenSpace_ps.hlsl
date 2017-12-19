#include "Defines.hlsl"

float4 PShader(VS_OUTPUT input) : SV_TARGET
{
	float4 textureColor = float4(0.0,1.0,0.0,0.0);// = shaderTexture[0].Sample(SampleType[0], uv);

    return textureColor;
}