#define NUM_LIGHTS 3
#pragma pack_matrix( row_major )


struct Light
{
    float4 position;
    float4 color;
    float4x4 lightprojmatrix;
};
struct DirectionalLight : Light
{
    float4 direction;
    float4x4 lightviewmatrix;
    float4 texOffset;
};
struct SpotLight : Light
{
    float4 direction;
    float angleMin, angleMax, range, dummyRW;
    float4x4 lightviewmatrix;
    float4 texOffset;
};
struct PointLight : Light
{
    float4x4 lightviewmatrix[6]; //128
    float4 texOffset[6];
};
cbuffer ObjectBuffer : register(b0)
{
    float4x4 modelMatrix; //64
};
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
cbuffer MeshBuffer : register(b3)
{
    bool _hasNormal; //4
    bool _hasRoughness; //8
    bool _hasMisc; //12
    bool _isEmissive; //16
    float _Metallic;
    float _Roughness;
    float _EmissiveStrength;
    float _F0;
};
cbuffer LightConstantBuffer : register(b4)
{
    uint _numDir, _numSpot, _numPoint, _dummy;
    DirectionalLight _dirLights[NUM_LIGHTS];
    PointLight _pointLights[NUM_LIGHTS];
    SpotLight _spotLights[NUM_LIGHTS];
};


struct VS_SHADOW_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float3 positionWS : POSITION;
    uint rti : SV_RenderTargetArrayIndex;
    //uint vpi : SV_ViewportArrayIndex;
};

[maxvertexcount(18)]
void GShader(triangle VS_SHADOW_OUTPUT input[3], inout TriangleStream<VS_SHADOW_OUTPUT> CubeMapStream)
{
    for (int i = 0; i < _numDir; i++)
    {
    // Compute screen coordinates
        VS_SHADOW_OUTPUT output;
	
        output.rti = i;
        //output.vpi = f;
        output.positionWS = float3(0, 0, 0);
        for (int v = 0; v < 3; v++)
        {
            float4 nPos = mul(float4(input[v].positionWS, 1), _dirLights[i].lightviewmatrix);
            output.positionVS = mul(nPos, _dirLights[i].lightprojmatrix);
            CubeMapStream.Append(output);
        }
        CubeMapStream.RestartStrip();
    }
}