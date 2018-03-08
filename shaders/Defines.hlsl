#define NUM_LIGHTS 3
#pragma pack_matrix( row_major )


#define PI   3.1415926535897932384626433832795
#define PI2  6.2831853071795864769252867665590
#define L2   0.6931471805599453094172321214581


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
	float4 F0 : SV_TARGET2;
	float4 misc : SV_TARGET3;
	float4 pos : SV_TARGET4;
    float4 pbrData : SV_TARGET5;
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
    float _globalAbsorption;
		  
    float _globalSpecular;
    float _globalRoughness;
    float _globalMetallic;
    float _globalIor;
		  
    float _globalRefraction;
    float _F0x;
    float _F0y;
    float _F0z;
		  
    float _d3;
    float _d4;
    float _d5;
    float _d6;
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
	float2 texCoord = screen.xy / float2(_screenWidth,_screenHeight);

	// Convert to clip space
	float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

	return ClipToView(clip);
}
float4 WorldToScreen(float3 world)
{
    float4 pos = float4(world, 1.0f);
    pos = mul(pos, viewMatrix);
    pos = mul(pos, projectionMatrix);

    return pos;
}


float3 GammaCorrect(float3 col)
{
    col = col / (col + float3(1.0, 1.0, 1.0));
    col = pow(col, float3(0.454545455f, 0.454545455f, 0.454545455f));
    return col;
}

// Hammersley function (return random low-discrepency points)
float2 Hammersley(uint i, uint N)
{
    return float2(
    float(i) / float(N),
    float(reversebits(i)) * 2.3283064365386963e-10);
}

// Random rotation based on the current pixel coord (Only use with Fullscreen quad!)
float randAngle(uint2 texCoord)
{
    uint x = texCoord.x;
    uint y = texCoord.y;
    return float(30u * x ^ y + 10u * x * y);
}
// Helper function (make skewed point into vec3 direction)
float3 MakeSample(float Theta, float Phi, uint2 texCoord)
{
    Phi += randAngle(texCoord);
    float SineTheta = sin(Theta);

    float x = cos(Phi) * SineTheta;
    float y = sin(Phi) * SineTheta;
    float z = cos(Theta);

    return float3(x, y, z);
}

//k = roughness
float DGgx(float NoH, float k)
{
  // Note: Generally sin2 + cos2 = 1
  // Also: Dgtr = c / (a * cos2 + sin2)
  // So...
    float Krough2 = k * k;
    float NoH2 = NoH * NoH;
    float denom = 1.0 + NoH2 * (Krough2 - 1.0);
    return Krough2 / (PI * denom * denom);
}

// Computes the exact mip-map to reference for the specular contribution.
// Accessing the proper mip-map allows us to approximate the integral for this
// angle of incidence on the current object.
//k = roughness
float compute_lod(uint NumSamples, float width, float height, float NoH, float k)
{
    float dist = DGgx(NoH, k); // Defined elsewhere as subroutine
    return 0.5 * (log2(float(width * height) / NumSamples) - log2(dist));
}


//GGxSample (S)
//E = output from Hammersley()
//k = roughness
float3 S(float2 E, float k, uint2 texCoord)
{
    float a = k * k;
    float Theta = atan(sqrt((a * E.x) / (1.0 - E.x)));
    float Phi = PI2 * E.y;
    return MakeSample(Theta, Phi, texCoord);
}
float FSchlick(float VoH, float metallic)
{
    float Kmetallic = metallic;
    return Kmetallic + (1.0 - Kmetallic) * pow(1.0 - VoH, 5.0);
}

//k = roughness
float GSmithSchlickBeckmann_(float NoV, float k)
{
    float r = k * k * 2.50662827f; // the constant is sqrt(PI2)
    return NoV / (NoV * (1.0 - r) + r);
}

float GSmithSchlickBeckmann(float NoL, float NoV, float NoH, float VoH, float k)
{
    return GSmithSchlickBeckmann_(NoL, k) * GSmithSchlickBeckmann_(NoV, k);
}
float2 SphereMap(float3 N)
{
    return float2(0.5 - atan2(N.y, N.x) / PI2, acos(N.z) / PI);
}
float4 textureSphereLod(TextureCube tex,SamplerState s, float3 N, float lod)
{
    return tex.SampleLevel(s, N, lod);
}
float3 BlendDielectric(float3 Kdiff, float3 Kspec, float3 Kbase)
{
    return Kdiff + Kspec;
}

float3 BlendMetal(float3 Kdiff, float3 Kspec, float3 Kbase)
{
    return Kspec * Kbase;
}

float3 BlendMaterial(float3 albedo, float3 Kdiff, float3 Kspec, float metallic)
{
    float3 Kbase = albedo;
    float scRange = smoothstep(0.2, 0.45, metallic);
    float3 dielectric = BlendDielectric(Kdiff, Kspec, Kbase);
    float3 metal = BlendMetal(Kdiff, Kspec, Kbase);
    return lerp(dielectric, metal, scRange);
}
float GeometryGGXSchlick(float val, float roughness)
{
    float nom = val;
    float denom = val * (1.0 - roughness) + roughness;
	
    return nom / denom;
}
float GeometryGGX(float maxNdotV, float maxNdotL, float roughness)
{
    float ggx2 = GeometryGGXSchlick(maxNdotV, roughness);
    float ggx1 = GeometryGGXSchlick(maxNdotL, roughness);
	
    return ggx1 * ggx2;
}


// GGX/Throwbridge-Reitz
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H),0);
    float NdotH2 = NdotH * NdotH;
	
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return nom / denom;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (float3(1.0, 1.0, 1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// ----------------------------------------------------------------------------
float3 FresnelSphericalGaussian(float3 F0, float cosTheta)
{
    return F0 + (float3(1.0, 1.0, 1.0) - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316 * cosTheta));
}