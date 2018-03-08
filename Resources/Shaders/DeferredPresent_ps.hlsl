#include "Defines.hlsl"

Texture2D diff : register(t0);
Texture2D norm : register(t1);
Texture2D F0Tex : register(t2);
Texture2D misc : register(t3);
Texture2D pos : register(t4);
Texture2D pbrData : register(t5);
Texture2D depth : register(t6);
Texture2D shadow : register(t7);
TextureCube SkyTex : register(t8);
TextureCube IBLTex : register(t9);

SamplerState SampleType[4];

float3 BRDF(float3 albedo, float3 F0v, float3 V, float3 normal, float3 lightDir, float3 lightCol, float roughness, float metallic);
float3 SpecBRDF(float3 normal, float3 V, float2 texCoord, float roughness, float metallic);
float4 PShader(VS_OUTPUT input) : SV_TARGET
{
    float4 finalResult = float4(0.0, 0.0, 0.0, 0);
    
    int2 texCoord = input.positionVS.xy;
    
    float4 albedo = diff.Load(int3(texCoord, 0));
    //float4 specular = spec.Load(int3(texCoord, 0));
    //float4 PBRData = pbrData.Load(int3(texCoord, 0));
	//x = Roughness, y = Metallic, z = EmissiveStrength, w = isEmissive ( non 0 = true)
    float4 miscData = misc.Load(int3(texCoord, 0));
    float3 F0v = F0Tex.Load(int3(texCoord, 0));
    float roughness = miscData.x;
    float metallic = miscData.y;
    float4 position = pos.Load(int3(texCoord, 0));
    float3 normal = normalize(norm.Load(int3(texCoord, 0)).xyz);
	
    if (albedo.w == 0.0)
    {
        return albedo.xyzw;
    }
   
    float3 V = normalize(cameraPosition.xyz - position.xyz);
	
	//attempt from: https://learnopengl.com/PBR/Theory
	
	//cook torrance
    float3 directBRDF = float3(0, 0, 0);
	
    float3 specBRDF = SpecBRDF(normal, V,texCoord, roughness, metallic);
    float3 irrMap = GammaCorrect(textureSphereLod(IBLTex,SampleType[0], normal, 0).rgb);

    float3 specDiffuse = irrMap * albedo.xyz / PI;
    float3 col = BlendMaterial(albedo.xyz, specDiffuse, specBRDF, metallic);

    for (int i = 0; i < _numDir;i++)
    {
        directBRDF += BRDF(albedo.xyz, F0v, V, normal, normalize(-_dirLights[i].direction.xyz), _dirLights[i].color.rgb, roughness, metallic);
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
    else if (_visualType == 6.0)
    {
        finalResult = float4(FresnelSchlick(dot(normal, V), F0v),0);
    }
    else
    {
        finalResult = float4(directBRDF, 1.0);
    }

    return finalResult;
}
float3 SpecBRDF(float3 normal, float3 V,float2 texCoord, float roughness, float metallic)
{
    float3 UpVector = abs(normal.z) < 0.9999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, normal));
    float3 TangentY = normalize(cross(normal, TangentX));

    const uint numSamples = 50;
    float3 sum = float3(0, 0, 0);

    //float3 H = normalize(normal + V);
    float NoV = max(dot(normal, V), 0.0);
    uint texWidth = 0;
    uint texHeight = 0;
    SkyTex.GetDimensions(texWidth, texHeight);
    for (uint i = 0; i < numSamples; ++i)
    {
        float2 Xi = Hammersley(i, numSamples);
        float3 Li = S(Xi, roughness, texCoord);
        float3 H = normalize(Li.x * TangentX + Li.y * TangentY + Li.z * normal);
        float3 L = normalize(-reflect(V, H));
		
		float NoL = max(dot(normal, L),0.0);
        float NoH = max(dot(normal, H),0.0);
        float VoH = max(dot(V, H),0.0);

        float lod = compute_lod(numSamples,texWidth,texHeight, NoH, roughness);

        float F_ = FSchlick(VoH, metallic);
        float G_ = GSmithSchlickBeckmann(NoL, NoV, NoH, VoH, roughness);
        float3 LColor = GammaCorrect(textureSphereLod(SkyTex, SampleType[0], L, lod).rgb);
       
        sum += F_ * G_ * LColor * VoH / (NoH * NoV);
    }
    return sum / (float) numSamples;
}
float3 BRDF(float3 albedo,float3 F0v, float3 V, float3 normal,float3 lightDir,float3 lightCol, float roughness, float metallic)
{
    float3 H = normalize(lightDir + V); //L V
    float NdotL = max(dot(lightDir, normal), 0.0); // L N cosTheta
   // cosTheta = max(cosTheta, 0.0);
    float NdotV = max(dot(normal, V), 0); // N V
   // float NdotL = dot(normal, lightDir); // N L cosTheta

    float D = DistributionGGX(normal, H, roughness);
	
    if (_visualType == 1.0)
    {
        return float3(D, D, D);
    }
    float3 F = FresnelSchlick(NdotL, F0v);
    if (_visualType == 2.0)
    {
        return F;
    }

    float G = GeometryGGX(NdotV, NdotL,roughness); //    GSmithSchlickBeckmann(NdotL, NdotV, max(dot(normal, H), 0.0), 0, roughness);
    if (_visualType == 3.0)
    {
        return float3(G, G, G);

    }
    float3 nominator = D * G * F;
    float denominator = 4 * NdotV * NdotL;
    float3 specular = nominator / max(denominator, 0.001);
        
	//direct diffuse part:
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;

    // add to outgoing radiance Lo
    float3 Lo = (kD * albedo / PI + specular) * lightCol * NdotL;
    return Lo;
}
