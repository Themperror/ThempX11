#include "Defines.hlsl"
#include "Functions.hlsl"

struct LightData
{
    int enabled, ldummy0, ldummy1, ldummy2;
    float4 position;
    float4 color;
};
struct DirectionalLight : LightData
{
    float4x4 lightprojmatrix[NUM_CASCADES];
    float4x4 lightviewmatrix[NUM_CASCADES];
    float4 direction;
    float4 texOffset; //x offset, y offset, width, height;
};
struct PointLight : LightData
{
	//float range, type, dummyRZ,dummyRW; //64
    float4x4 lightprojmatrix;
    float4x4 lightviewmatrix[6]; //128
    float4 texOffset[6]; //x offset, y offset, width, height;
};
struct SpotLight : LightData
{
    float4 direction;
    float angleMin, angleMax, range, dummyRW; //64
    float4x4 lightprojmatrix;
    float4x4 lightviewmatrix; //128
    float4 texOffset; //x offset, y offset, width, height;
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

    float _numCascades;
    float _shadow_atlas_size;
    float _d6;
    float _d7;
};

cbuffer CascadeLightBuffer : register(b4)
{
    uint _numDir, _numSpot, _numPoint, _cascadeIndex ;
    float4 splits0;
    float4 splits1;
    DirectionalLight _dirLights[NUM_LIGHTS];
    PointLight _pointLights[NUM_LIGHTS];
    SpotLight _spotLights[NUM_LIGHTS];
};
struct VS_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float3 normalWS : NORMAL0;
    float3 tangentWS : TANGENT;
    float3 bitangentWS : BITANGENT;
    float3 positionWS : POSITION;
    float2 uv : UV0;
};

Texture2D diff : register(t0);
Texture2D norm : register(t1);
Texture2D F0Tex : register(t2);
Texture2D misc : register(t3);
Texture2D pos : register(t4);
Texture2D depth : register(t5);

#ifdef MS
Texture2DMS<float> nonDirshadow : register(t6);
//Texture2DMS<float> cascadeShadow[6] : register(t7);
Texture2DMS<float> cascadeShadow0 : register(t7);
Texture2DMS<float> cascadeShadow1 : register(t8);
Texture2DMS<float> cascadeShadow2 : register(t9);
Texture2DMS<float> cascadeShadow3 : register(t10);
Texture2DMS<float> cascadeShadow4 : register(t11);
Texture2DMS<float> cascadeShadow5 : register(t12);
#else
Texture2D nonDirshadow : register(t6);
//Texture2D cascadeShadow[6] : register(t7);
Texture2D cascadeShadow0 : register(t7);
Texture2D cascadeShadow1 : register(t8);
Texture2D cascadeShadow2 : register(t9);
Texture2D cascadeShadow3 : register(t10);
Texture2D cascadeShadow4 : register(t11);
Texture2D cascadeShadow5 : register(t12);
#endif

TextureCube SkyTex1 : register(t13);
TextureCube IBLTex1 : register(t14);
TextureCube SkyTex2 : register(t15);
TextureCube IBLTex2 : register(t16);
SamplerState SampleType[4];


float IsLitDirectional(const float4 worldPixel, const int i, const float pixelDepth, const float pixelWorldDepth);
bool IsLitPoint(float4 worldPixel, const int i, out float distStrength, out float shadowStrength);

float4 PShader(VS_OUTPUT input) : SV_TARGET
{
    float4 finalResult = float4(0.0, 0.0, 0.0, 0);
    
    int2 texCoord = input.positionVS.xy;

	float4 albedo = diff.Load(int3(texCoord, 0));
    if (albedo.w == 0.0)
    {
        return albedo.xyzw;
    }
	//X = Roughness, Y = Metallic, Z = EmissiveStrength, W = isEmissive ( non 0 = true)
    float4 miscData = misc.Load(int3(texCoord, 0));

	//XYZ = F0 RGB W = AO
    float4 F0v = F0Tex.Load(int3(texCoord, 0));
    float roughness = miscData.x;
    float metallic = miscData.y;
    float4 position = pos.Load(int3(texCoord, 0));
    float pixelDepth = depth.Load(int3(texCoord, 0)).r;
    //float pixelWorldDepth = distance(cameraPosition, position);

    float4 viewPos = mul(position, mul(viewMatrix, projectionMatrix));
    float pixelWorldDepth = viewPos.z;
    float3 normal = normalize(norm.Load(int3(texCoord, 0)).xyz);
	//AO Value is stored in W value
    albedo = float4(albedo.xyz, albedo.w);

    // F0v = lerp(float3(0.04, 0.04, 0.04), F0v, metallic);
    float3 V = normalize(cameraPosition.xyz - position.xyz);
	

	//attempt from: https://learnopengl.com/PBR/Theory
	//cook torrance
    float3 directBRDF = float3(0, 0, 0);
	
    float3 specBRDF = SpecBRDF(normal, V,texCoord, roughness, metallic,F0v.xyz,SkyTex1,SkyTex2,SampleType[0],_SkyboxLerp,_F0x);
    float3 irrMap = GammaCorrect(lerp(textureSphereLod(IBLTex1, SampleType[0], normal, 0).rgb, textureSphereLod(IBLTex2, SampleType[0], normal, 0).rgb, _SkyboxLerp), _F0y);
    float3 specDiffuse = irrMap * albedo.xyz;

    [unroll(3)]
    for (uint i = 0; i < _numDir; i++)
    {
        float litStr = IsLitDirectional(position, i, pixelDepth, pixelWorldDepth);
        if (_visualType == 1)
        {
            if (litStr >= 5.0)
            {
                return float4(1, 0, 0, 1);
            }
            if (litStr >= 4.0)
            {
                return float4(0, 1, 0, 1);
            }
            if (litStr >= 3.0)
            {
                return float4(0, 0, 1, 1);
            }
            if (litStr >= 2.0)
            {
                return float4(1, 0, 1, 0);
            }
            if (litStr >= 1.0)
            {
                return float4(0.5, 1, 0.5, 1);
            }
            if (litStr >= 0.0)
            {
                return float4(0, 0.5, 1, 1);
            }
        }
		
        //return float4(litStr, litStr, litStr, litStr);
        if (litStr > 0.0)
        {
            directBRDF += GammaCorrect(BRDF(albedo.xyz, F0v.xyz, V, normal, normalize(-_dirLights[i].direction.xyz), _dirLights[i].color, roughness, metallic, _visualType, _F0z) * litStr, _F0y);
        }
    }
	
    [unroll(3)]
    for (uint j = 0; j < _numPoint; j++)
    {
        float strength = 0.0;
        float shadowStr= 0.0;
        if (IsLitPoint(position, j, strength,shadowStr))
        {
            directBRDF += GammaCorrect(BRDF(albedo.xyz, F0v.xyz, V, normal, -normalize(position.xyz - _pointLights[j].position.xyz), _pointLights[j].color, roughness, metallic, _visualType, _F0z) * strength * shadowStr, _F0y);
        }
    }

    float3 col = BlendMaterial(albedo.xyz, specDiffuse, specBRDF, metallic);
    finalResult = float4(directBRDF + col, 1.0);

    return finalResult;
}


float CalcShadowFactor(const float4 lightSpacePos, const int cascadeIndex)
{
}

float IsLitDirectional(const float4 worldPixel, const int i, const float pixelDepth, const float pixelWorldDepth)
{
    float4 viewPosition = worldPixel;
    int index = _numCascades-1;

	// find the appropriate depth map to look up in based on the depth of this pixel
	//we have a max of 6 cascade splits so this handles them all

	//from DirectX Samples
    float4 pixelDepth4 = float4(pixelWorldDepth, pixelWorldDepth, pixelWorldDepth, pixelWorldDepth);
    //float4 pixelDepth4 = float4(viewPosition.z, viewPosition.z, viewPosition.z, viewPosition.z);
    float4 fComparison = (pixelDepth4 > splits0);
    float4 fComparison2 = (pixelDepth4 > splits1);
    float fIndex = dot(
                            float4(_numCascades > 0,
                                    _numCascades > 1,
                                    _numCascades > 2,
                                    _numCascades > 3)
                            , fComparison)
                         + dot(
                            float4(
                                    _numCascades > 4,
                                    _numCascades > 5,
                                    _numCascades > 6,
                                    _numCascades > 7)
                            , fComparison2);
                                    
    fIndex = min(fIndex, _numCascades - 1);
    index = (int) floor(fIndex);
    if (_visualType == 1)
    {
        return fIndex;
    }
    //return (float)index / _numCascades;

    viewPosition = mul(viewPosition, mul(_dirLights[i].lightviewmatrix[index], _dirLights[i].lightprojmatrix[index]));

    float3 pt;
#ifdef MS
    float2 texOffset = _dirLights[i].texOffset.xy;
#else
    float2 texOffset = _dirLights[i].texOffset.xy / _shadow_atlas_size;
#endif
	
    pt.x = viewPosition.x / viewPosition.w * 0.5 + 0.5f;
    pt.y = -viewPosition.y / viewPosition.w * 0.5 + 0.5f;
    pt.z = viewPosition.z / viewPosition.w;
     
    float visibility = 0.0;
    if ((saturate(pt.x) == pt.x) && (saturate(pt.y) == pt.y))
    {
#ifdef MS
        pt.xy *= (_dirLights[i].texOffset.zw);
        pt.xy += texOffset.xy;
        float totalVis = 0.0;
        const int MSValue = clamp(_MSAAValue, 1, 8);
        for (int j = 0; j < MSValue; j++)
        {
            float shadowVal = 
			index == 0 ? cascadeShadow0.Load(float3(pt.xy,0), j).r : 
			index == 1 ? cascadeShadow1.Load(float3(pt.xy,0), j).r : 
			index == 2 ? cascadeShadow2.Load(float3(pt.xy,0), j).r : 
			index == 3 ? cascadeShadow3.Load(float3(pt.xy,0), j).r : 
			index == 4 ? cascadeShadow4.Load(float3(pt.xy,0), j).r : 
			cascadeShadow5.Load(float3(pt.xy,0), j).r;
            if (shadowVal > viewPosition.z - 0.001f)
            {
                totalVis += 1.0;
            }
        }
        visibility = totalVis / _MSAAValue;
        
#else
        pt.xy /= (float2(_shadow_atlas_size, _shadow_atlas_size) / _dirLights[i].texOffset.zw);
        pt.xy += texOffset.xy;

        float shadowVal = (
		index == 0 ? cascadeShadow0.Sample(SampleType[0], pt.xy).r :
		index == 1 ? cascadeShadow1.Sample(SampleType[0], pt.xy).r :
		index == 2 ? cascadeShadow2.Sample(SampleType[0], pt.xy).r :
		index == 3 ? cascadeShadow3.Sample(SampleType[0], pt.xy).r :
		index == 4 ? cascadeShadow4.Sample(SampleType[0], pt.xy).r :
					 cascadeShadow5.Sample(SampleType[0], pt.xy).r);

        //float shadowVal = cascadeShadow[index].Sample(SampleType[0], pt.xy).r;
		if (shadowVal > viewPosition.z - 0.0005f)
        {
            visibility = 1.0f;
        }
#endif
    }
	
    return visibility;
}
bool IsLitPoint(float4 worldPixel,const int i, out float distStrength, out float shadowStrength)
{
    float dist = distance(worldPixel.xyz, _pointLights[i].position.xyz);
    float4x4 proj = (_pointLights[i].lightprojmatrix);
    float visibility = 0.0;
    bool isLit = false;
    for (int j = 0; j < 6; j++)
    {
        float4x4 view = (_pointLights[i].lightviewmatrix[j]);
        float4 viewPosition = worldPixel;
        viewPosition = mul(viewPosition, view);
        viewPosition = mul(viewPosition, proj);
        float3 projectTexCoord;
#ifdef MS
        float2 texOffset = _pointLights[i].texOffset[j].xy; //offset is 4096 on X
#else
        float2 texOffset = _pointLights[i].texOffset[j].xy / _shadow_atlas_size;
#endif
        projectTexCoord.x = viewPosition.x / viewPosition.w * 0.5 + 0.5f;
        projectTexCoord.y = -viewPosition.y / viewPosition.w * 0.5 + 0.5f;
        projectTexCoord.z = viewPosition.z / viewPosition.w;
        if ((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
        {
#ifdef MS
			//0.0 to 1.0    * 1024 (map size) = 0  to 1024
			projectTexCoord.xy *= _pointLights[i].texOffset[j].zw;
			//0  to 1024 + 4096
			projectTexCoord.xy += texOffset.xy;

			float totalVis = 0.0;
			const int MSValue = clamp(_MSAAValue, 1, 8);
			for (int k = 0; k < MSValue; k++)
			{
				float shadowVal = nonDirshadow.Load(float3(projectTexCoord.xy,0), k).r;
				if (shadowVal > projectTexCoord.z - 0.0001f)
				{
					totalVis += 1.0;
				}
			}
			visibility = totalVis / _MSAAValue;
			if(visibility > 0.0)
			{
				isLit = true;
				break;
			}
#else
            projectTexCoord.xy /= (float2(_shadow_atlas_size, _shadow_atlas_size) / _pointLights[i].texOffset[j].zw);
            projectTexCoord.xy += texOffset.xy;
            float shadowVal = nonDirshadow.Sample(SampleType[0], projectTexCoord.xy).r;
            if (shadowVal > projectTexCoord.z - 0.0001f)
            {
                visibility = 1.0f;
                isLit = true;
                break;
            }
#endif
        }
    }
    distStrength = 1.0 / ((dist * dist) / _F0z);
    shadowStrength = visibility;
    return isLit;
}
//float DoSpotLight(float4 worldPixel, float3 worldNormal, float2 specular, int i)
//{
//    float lightStr = max(-dot(spotLights[i].direction.xyz, worldNormal.xyz), 0.0);
//    float4x4 view = (spotLights[i].lightviewmatrix);
//    float4x4 proj = (spotLights[i].lightprojmatrix);
//    float4 viewPosition = worldPixel;
//    viewPosition = mul(viewPosition, view);
//    viewPosition = mul(viewPosition, proj);
//
//    float3 projectTexCoord;
//    float2 texOffset = spotLights[i].texOffset.xy / SHADOW_MAP_SIZE;
//    projectTexCoord.x = viewPosition.x / viewPosition.w * 0.5 + 0.5f;
//    projectTexCoord.y = -viewPosition.y / viewPosition.w * 0.5 + 0.5f;
//    projectTexCoord.z = viewPosition.z / viewPosition.w; //* 0.5 + 0.5;
//     
//    float visibility = 0;
//    if ((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))//  && viewPosition.z > 0.0)
//    {
//        projectTexCoord.xy /= (SHADOW_MAP_SIZE / spotLights[i].texOffset.zw);
//        projectTexCoord.xy += texOffset.xy;
//        float shadowVal = cascadeShadow.Sample(SampleType[0], projectTexCoord.xy).r;
//      //  return projectTexCoord.z;
//       // return Remap(viewPosition.z, 0, 0.1, 0, 1.0);
//        if (shadowVal > projectTexCoord.z - 0.001f)
//        {
//            visibility = 1.0;
//        }
//    }
//
//    float3 lightToPixel = worldPixel.xyz - spotLights[i].position.xyz;
//    float dist = length(lightToPixel);
//    lightToPixel = normalize(lightToPixel);
//    float cosDir = dot(lightToPixel, -spotLights[i].direction.xyz);
//    float effect = smoothstep(spotLights[i].angleMax, spotLights[i].angleMin, cosDir);
//    float att = smoothstep(spotLights[i].range, 0.0, dist);
//
//    return (lightStr * visibility);
//}