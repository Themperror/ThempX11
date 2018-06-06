#include "Defines.hlsl"

struct Light
{
    int enabled, ldummy0, ldummy1, ldummy2;
    float4 position;
    float4 color;
};
struct DirectionalLight : Light
{
    float4x4 lightprojmatrix[6];
    float4x4 lightviewmatrix[6];
    float4 direction;
    float4 texOffset;
};
struct SpotLight : Light
{
    float4 direction;
    float angleMin, angleMax, range, dummyRW;
    float4x4 lightprojmatrix;
    float4x4 lightviewmatrix;
    float4 texOffset;
};
struct PointLight : Light
{
    float4x4 lightprojmatrix;
    float4x4 lightviewmatrix[6]; //128
    float4 texOffset[6];
};
cbuffer ObjectBuffer : register(b0)
{
    float4x4 modelMatrix; //64
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

    float _numCascades;
    float _d5;
    float _d6;
    float _d7;
};
cbuffer LightConstantBuffer : register(b4)
{
    uint _numDir, _numSpot, _numPoint, _cascadeIndex;
    float4 splits0;
    float4 splits1;
    DirectionalLight _dirLights[NUM_LIGHTS];
    PointLight _pointLights[NUM_LIGHTS];
    SpotLight _spotLights[NUM_LIGHTS];
};


struct VS_SHADOW_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float3 positionWS : POSITION;
    //uint rti : SV_RenderTargetArrayIndex;
    uint vpi : SV_ViewportArrayIndex;
};

[maxvertexcount(18)]
void GShader(triangle VS_SHADOW_OUTPUT input[3], inout TriangleStream<VS_SHADOW_OUTPUT> CubeMapStream)
{
    for (int i = 0; i < NUM_LIGHTS; i++)
    {
		if(_dirLights[i].enabled)
        {
			VS_SHADOW_OUTPUT output;

			output.vpi = i;
			output.positionWS = float3(0, 0, 0);
			for (int v = 0; v < 3; v++)
			{
				float4 nPos = mul(float4(input[v].positionWS, 1), _dirLights[i].lightviewmatrix[_cascadeIndex]);
				output.positionVS = mul(nPos, _dirLights[i].lightprojmatrix[_cascadeIndex]);
				CubeMapStream.Append(output);
			}
            CubeMapStream.RestartStrip();
        }
    }
}