#include "Defines.hlsl"

struct VS_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float3 normalWS : NORMAL0;
    float3 positionWS : POSITION;
    float3 uv : UV0;
};
struct PS_OUTPUT
{
    float4 diffuse : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 specular : SV_TARGET2;
    float4 misc : SV_TARGET3;
    float4 pos : SV_TARGET4;
};

cbuffer ConstantBuffer : register(b2)
{
    float _screenWidth;
    float _screenHeight;
    float _visualType;
    float _SkyboxLerp;

    float _d1;
    float _globalRoughness;
    float _globalMetallic;
    float _d2;

    float _d3;
    float _F0x;
    float _F0y;
    float _F0z;

    float _d4;
    float _d5;
    float _d6;
    float _d7;
};

//Texture2D shaderTexture[4];

 
TextureCube sky1Tex : register(t0);
TextureCube sky1IBL : register(t1);
TextureCube sky2Tex : register(t2);
TextureCube sky2IBL : register(t3);
SamplerState diffSampler : register(s0);

PS_OUTPUT PShader(VS_OUTPUT input)
{
    PS_OUTPUT output;
    output.pos = float4(input.positionWS, 1.0f);

    float3 envColor = lerp(sky1Tex.SampleLevel(diffSampler, input.uv, 0).xyz * _F0x, sky2Tex.SampleLevel(diffSampler, input.uv, 0).xyz * _F0x, _SkyboxLerp);
	//gamma correction (linear to gamma space, since HDR's are Linear by default)
    envColor = envColor / (envColor + float3(1.0, 1.0, 1.0));
   // envColor = pow(envColor, float3(0.454545455f, 0.454545455f, 0.454545455f));
    envColor = pow(abs(envColor), float3(_F0x, _F0x, _F0x));


    output.diffuse = float4(envColor, 0.0); //mark as sky so it doesn't get lighting pass


    //output.diffuse = float4(1,1,1, 1.0); //mark as sky so it doesn't get lighting pass
    output.normal = float4(normalize(input.normalWS), 4);
    output.specular = float4(0.0, 0.0, 0.0, 0.0);
    output.misc = float4(0.0, 0.0, 0.0, 0.0);

    return output;
}