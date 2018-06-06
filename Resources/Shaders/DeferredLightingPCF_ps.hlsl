#include "Defines.hlsl"
#include "Functions.hlsl"
#include "Structs.hlsl"

Texture2D diff : register(t0);
Texture2D norm : register(t1);
Texture2D F0Tex : register(t2);
Texture2D misc : register(t3);
Texture2D pos : register(t4);
Texture2D depth : register(t5);

#ifdef MS //multisample
Texture2DMS<float> shadow : register(t6);
#else
Texture2D shadow : register(t6);
#endif

TextureCube SkyTex1 : register(t7);
TextureCube IBLTex1 : register(t8);
TextureCube SkyTex2 : register(t9);
TextureCube IBLTex2 : register(t10);
SamplerState SampleType[4];

float3 BRDF(float3 albedo, float3 F0v, float3 V, float3 normal, float3 lightDir, float4 lightCol, float roughness, float metallic);
float3 SpecBRDF(float3 normal, float3 V, float2 texCoord, float roughness, float metallic, float3 F0v);

float IsLitDirectional(float4 worldPixel, int i);
bool IsLitPoint(float4 worldPixel, int i, out float distStrength, out float shadowStrength);

float4 PShader(VS_OUTPUT input) : SV_TARGET
{
    float4 finalResult = float4(0.0, 0.0, 0.0, 0);
    
    int2 texCoord = input.positionVS.xy;

	float4 albedo = diff.Load(int3(texCoord, 0));
	//X = Roughness, Y = Metallic, Z = EmissiveStrength, W = isEmissive ( non 0 = true)
    float4 miscData = misc.Load(int3(texCoord, 0));

	//XYZ = F0 RGB W = AO
    float4 F0v = F0Tex.Load(int3(texCoord, 0));
    float roughness = miscData.x;
    float metallic = miscData.y;
    float emissiveStrength = miscData.z;
    bool isEmissive = miscData.w;
    float4 position = pos.Load(int3(texCoord, 0));
    float3 normal = normalize(norm.Load(int3(texCoord, 0)).xyz);
	//AO Value is stored in W value
    albedo = float4(albedo.xyz, albedo.w);

   // F0v = lerp(float3(0.04, 0.04, 0.04), F0v, metallic);
    
    float3 V = normalize(cameraPosition.xyz - position.xyz);
	if (albedo.w == 0.0)
    {
        return albedo.xyzw;
    }
	if (_visualType == 6.0)
    {
        return float4(metallic, metallic, metallic, 1.0);
    }
    else if (_visualType == 7.0)
    {
        return float4(roughness, roughness, roughness, 1.0);
    }
    else if (_visualType == 8.0)
    {
        return float4(albedo.xyz, 1.0);
    }
    else if (_visualType == 9.0)
    {
        return float4(normal.xyz, 1.0);
    }


	//attempt from: https://learnopengl.com/PBR/Theory
	//cook torrance
    float3 directBRDF = float3(0, 0, 0);
	
    float3 specBRDF = SpecBRDF(normal, V, texCoord, roughness, metallic, F0v.xyz, SkyTex1, SkyTex2, SampleType[0],_SkyboxLerp,_F0x);
    float3 irrMap = GammaCorrect(lerp(textureSphereLod(IBLTex1, SampleType[0], normal, 0).rgb, textureSphereLod(IBLTex2, SampleType[0], normal, 0).rgb, _SkyboxLerp), _F0y);
    float3 specDiffuse = irrMap * albedo.xyz;
    float3 col = BlendMaterial(albedo.xyz, specDiffuse, specBRDF, metallic);
	
    uint i = 0;
    for (i = 0; i < NUM_LIGHTS; i++)
    {
		if(_dirLights[i].enabled)
		{
			float litStr = IsLitDirectional(position, i);
			if (litStr > 0.0 || isEmissive)
			{
				float3 brdfResult = DebugBRDF(albedo.xyz, F0v.xyz, V, normal, normalize(-_dirLights[i].direction.xyz), _dirLights[i].color, roughness, metallic, _visualType, _F0z);
				float3 brdfLit = brdfResult * litStr;
				float3 brdfEmissive = brdfResult * emissiveStrength;
				directBRDF += GammaCorrect(brdfLit + brdfEmissive, _F0y);
			}
			else if (_visualType > 0.0)
			{
				directBRDF += GammaCorrect(DebugBRDF(albedo.xyz, F0v.xyz, V, normal, normalize(-_dirLights[i].direction.xyz), _dirLights[i].color, roughness, metallic, _visualType, _F0z), _F0y);
            }
        }
    }
    for (i = 0; i < NUM_LIGHTS; i++)
    {
        if (_pointLights[i].enabled)
        {
            float strength = 0.0;
            float shadowStr = 0.0;
		   //bool lit = IsLitPoint(position, i, strength, shadowStr);
		   //directBRDF += float3(lit, lit, lit);
		   //break;
            if (IsLitPoint(position, i, strength, shadowStr))
            {
                directBRDF += GammaCorrect(DebugBRDF(albedo.xyz, F0v.xyz, V, normal, -normalize(position.xyz - _pointLights[i].position.xyz), _pointLights[i].color, roughness, metallic, _visualType, _F0z) * strength * shadowStr, _F0y);
            }
            else if (_visualType > 0.0)
            {
                directBRDF += GammaCorrect(DebugBRDF(albedo.xyz, F0v.xyz, V, normal, -normalize(position.xyz - _pointLights[i].position.xyz), _pointLights[i].color, roughness, metallic, _visualType, _F0z) * strength, _F0y);
            }
        }
    }
    if (_visualType == 0.0)
    {
        finalResult = float4(directBRDF + col, 1.0);
    }
    else if (_visualType == 4.0)
    {
        finalResult = float4(specBRDF, 1.0);

    }
    else if (_visualType == 5.0)
    {
        finalResult = float4(directBRDF, 1.0);
    }
    else
    {
        finalResult = float4(directBRDF, 1.0);
    }

    return finalResult;
}


float3 offset_lookup(sampler2D map,float4 loc,float2 offset, float texmapscale)
{
    return tex2Dproj(map, float4(loc.xy + offset * texmapscale * loc.w, loc.z, loc.w));
}


float IsLitDirectional(float4 worldPixel, int i)
{
    float4x4 view = (_dirLights[i].lightviewmatrix);
    float4x4 proj = (_dirLights[i].lightprojmatrix);
    float4 viewPosition = worldPixel;
    viewPosition = mul(viewPosition, view);
    viewPosition = mul(viewPosition, proj);

    float3 projectTexCoord;
#ifdef MS
    float2 texOffset = _dirLights[i].texOffset.xy;
#else
    float2 texOffset = _dirLights[i].texOffset.xy / _shadow_atlas_size;
#endif
	
    projectTexCoord.x = viewPosition.x / viewPosition.w * 0.5 + 0.5f;
    projectTexCoord.y = -viewPosition.y / viewPosition.w * 0.5 + 0.5f;
    projectTexCoord.z = viewPosition.z / viewPosition.w;
     
    float visibility = 0.0;
    if ((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
    {
#ifdef MS
        projectTexCoord.xy *= (_dirLights[i].texOffset.zw);
        projectTexCoord.xy += texOffset.xy;
        float totalVis = 0.0;
        const int MSValue = clamp(_MSAAValue, 1, 8);
        for (int j = 0; j < MSValue; j++)
        {
            float shadowVal = shadow.Load(projectTexCoord.xy, j).r;
            if (shadowVal > viewPosition.z - 0.0005f)
            {
                totalVis += 1.0;
            }
        }
        visibility = totalVis / _MSAAValue;
        
#else
        projectTexCoord.xy /= (float2(_shadow_atlas_size, _shadow_atlas_size) / _dirLights[i].texOffset.zw);
        projectTexCoord.xy += texOffset.xy;
        float shadowVal = shadow.Sample(SampleType[0], projectTexCoord.xy).r;
		if (shadowVal > viewPosition.z - 0.0005f)
        {
            visibility = 1.0f;
        }
#endif
    }
    return visibility;
}
bool IsLitPoint(float4 worldPixel, int i, out float distStrength, out float shadowStrength)
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
				float shadowVal = shadow.Load(projectTexCoord.xy, k).r;
				if (shadowVal > projectTexCoord.z - 0.0005f)
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
            float shadowVal = shadow.Sample(SampleType[0], projectTexCoord.xy).r;
            if (shadowVal > projectTexCoord.z - 0.0005f)
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
//        float shadowVal = shadow.Sample(SampleType[0], projectTexCoord.xy).r;
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