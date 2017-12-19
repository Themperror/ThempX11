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


struct VS_SHADOW_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float3 positionWS : POSITION;
    //uint rti : SV_RenderTargetArrayIndex;
    uint vpi : SV_ViewportArrayIndex;
};


struct VS_OUTPUT
{
	float4 positionVS : SV_POSITION;
	float3 normalVS : NORMAL0;
	float3 tangentVS : TANGENT;
	float3 bitangentVS : BITANGENT;
	float3 positionWS : POSITION;
	float2 uv : UV0;
};
struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
	float2 uv : UV;
};
struct PS_OUTPUT
{
	float4 diffuse : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 specular : SV_TARGET2;
	float4 misc : SV_TARGET3;
	float4 pos : SV_TARGET4;
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
};
cbuffer ConstantBuffer : register(b2)
{
	float screenWidth;
	float screenHeight;
	float visualType; // 0 = default, 1 = diffuse, 2 = normal, 3 = specular, 4 = opacity, 5 = UV's
	float c_dummy;
};
cbuffer MeshBuffer : register(b3)
{
	bool hasNormal; //4
	bool hasSpecular; //8
	bool hasMisc; //12
	bool m_dummy; //16
};
cbuffer LightConstantBuffer : register(b4)
{
    uint numDir, numSpot, numPoint, dummy;
    DirectionalLight dirLights[NUM_LIGHTS];
    PointLight pointLights[NUM_LIGHTS];
    SpotLight spotLights[NUM_LIGHTS];
};

float3 ExpandNormal(float3 n)
{
	return n * 2.0f - 1.0f;
};

float Remap(float x, float input_min, float input_max, float output_min, float output_max)
{
	float n = (x - output_min) / (output_max - output_min);
	return input_min + ((input_max - input_min)*n);
}
float3 Remap(float3 x, float3 input_min, float3 input_max, float3 output_min, float3 output_max)
{
	return float3(
		Remap(x.x, input_min.x, input_max.x, output_min.x, output_max.x),
		Remap(x.y, input_min.y, input_max.y, output_min.y, output_max.y),
		Remap(x.z, input_min.z, input_max.z, output_min.z, output_max.z)
		);
}

float4 ClipToView(float4 clip)
{
	// View space position.
	float4 view = mul(invProjectionMatrix, clip);
	// Perspective projection.
	view = view / view.w;

	return view;
}

// Convert screen space coordinates to view space.
float4 ScreenToView(float4 screen)
{
	// Convert to normalized texture coordinates
	float2 texCoord = screen.xy / float2(screenWidth,screenHeight);

	// Convert to clip space
	float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

	return ClipToView(clip);
}
float3 ScreenToWorld(float2 screenPos, float depthV)
{
	double x = 2.0 * screenPos / screenWidth - 1;
	double y = - 2.0 * screenPos / screenHeight + 1;
	float4x4 invViewProj = mul(invProjectionMatrix, invViewMatrix);

	float3 P = float3(x, y, 0);
	return mul(P,invViewProj);

}
float4 WorldToScreen(float3 world)
{
    float4 pos = float4(world, 1.0f);
    pos = mul(pos, viewMatrix);
    pos = mul(pos, projectionMatrix);

    return pos;
}