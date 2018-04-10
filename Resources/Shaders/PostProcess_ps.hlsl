Texture2D g_Diffuse : register(t0);
Texture2D g_Normal : register(t1);
Texture2D g_F0Tex : register(t2);
Texture2D g_Misc : register(t3);
Texture2D g_Position : register(t4);
Texture2D g_Depth : register(t5);
Texture2D finalComp : register(t6);

SamplerState samplerState : register(s0);


cbuffer CameraBuffer : register(b1)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 invProjectionMatrix;
    float4x4 invViewMatrix;
    float4 cameraPosition;
    float4 cameraDir;
};
cbuffer ConstantBuffer : register(b2)
{
    float _screenWidth;
    float _screenHeight;
    float _visualType;
    float _SkyboxLerp;

    float _shadowType;
    float _globalRoughness;
    float _globalMetallic;
    float _MSAAValue;

    float _time;
    float _F0x;
    float _F0y;
    float _F0z;

    float _d4;
    float _d5;
    float _d6;
    float _d7;
};
struct VS_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float2 uv : UV0;
};
float Remap(float x, float input_min, float input_max, float output_min, float output_max)
{
    float n = (x - output_min) / (output_max - output_min);
    return input_min + ((input_max - input_min) * n);
}
float4 PShader(VS_OUTPUT input) : SV_Target
{
    //float2 uv = float2(
	//input.uv.x + sin(input.uv.y * 10.0 + _time) / 10.0,
	//input.uv.y
	//);
    //float depth = g_Depth.Sample(samplerState, input.uv).r;
    //depth = Remap(depth, 0.90, 1.0, 0, 1.0);
    //float4 finalColor = finalComp.Sample(samplerState, input.uv) * (depth);
    //
	//float4 finalColor = finalComp.Sample(samplerState, input.uv);
    return finalComp.Sample(samplerState, input.uv);
}
