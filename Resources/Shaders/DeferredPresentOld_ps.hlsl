#include "Defines.hlsl"


Texture2D diff : register(t0);
Texture2D norm : register(t1);
Texture2D spec : register(t2);
Texture2D misc : register(t3);
Texture2D pos : register(t4);
Texture2D pbrData : register(t5);
Texture2D depth : register(t6);
Texture2D shadow : register(t7);
TextureCube SkyTex : register(t8);
TextureCube IBLTex : register(t9);

SamplerState SampleType[4];

#define SHADOW_BIAS 0.5

float DoPointLight		(float4 worldPixel, float3 worldNormal, float2 specular, int i);
float DoDirectionalLight(float4 worldPixel, float3 worldNormal, float2 specular, int i);
float DoSpotLight		(float4 worldPixel, float3 worldNormal, float2 specular, int i);

#define PI   3.1415926535897932384626433832795
#define PI2  6.2831853071795864769252867665590
#define L2   0.6931471805599453094172321214581

float Sqr(float x)
{
    return x * x;
}
float3 Sqr(float3 x)
{
    return x * x;
}

float4 textureSphereLod(TextureCube tex, float3 N, float lod);

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
    float scRange =  smoothstep(0.2, 0.45, metallic);
    float3 dielectric = BlendDielectric(Kdiff, Kspec, Kbase);
    float3 metal = BlendMetal(Kdiff, Kspec, Kbase);
    return lerp(dielectric, metal, scRange);
}

///////////////////////////////////////
////n = refractive index
//float getF0(float n)
//{
//    return pow((1 - n) / (1 + n), 2);
//}
////Schlick approx
//float FresnelTerm(float ior,float ldoth)
//{
//    float f0 = getF0(ior);
//    return f0 + (1 - f0) * pow(1 - ldoth, 5);
//
//}
///////////////////////////////////////
//
////Schlick / Smith
////v = lightdir or cameradir
//float G(float roughness, float3 normal, float3 v)
//{
//    float k = ((roughness + 1) * (roughness + 1)) / 8;
//    float NV = dot(normal, v);
//    return NV / (NV * (1 - k) + k);
//}
//
////testing, maybe correct
//float F(float metallic , float specular, float3 viewDir, float3 halfDir)
//{
//
//    float base = 1 - dot(viewDir, halfDir);
//    float exponential = pow(base, 5.0);
//    float fresnel = exponential + metallic * (1.0 - exponential);
//    return specular * fresnel;
//   //float VH = dot(viewDir, halfDir);
//   //float p = pow(2, (-5.55473 * VH - 6.98316) * VH);
//   //return shinyness + (1 - shinyness) * p;
//}


//
//
//// D()
////n = surface normal (presumed)
////h = "half vector" of lightdir + viewdir (norm(light+view))
////--deemed correct  
//float TrowbridgeReitzGGX(float roughness, float NdotH)
//{
//    NdotH = max(NdotH, 0.0);
//    float a = roughness * roughness;
//    float a2 = a * a;
//    float d = (NdotH * a2 - NdotH) * NdotH + 1; // 2 mad
//    return a2 / (PI * d * d);
//}
//
//// F()
////n = surface normal
////v = viewdir
//float3 FresnelSchlick(float specColor, float NdotV)
//{
//    float specColorSqrt = sqrt(clamp(specColor, 0, 0.99));
//    float n = (1 + specColorSqrt) / (1 - specColorSqrt);
//    float g = sqrt(n * n + NdotV * NdotV - 1);
//    return 0.5 * Sqr((g - NdotV) / (g + NdotV)) * (1 + Sqr(((g + NdotV) * NdotV - 1) / ((g - NdotV) * NdotV + 1)));
//
//  //  float p = max(NdotV, 0);
//    //return pow(p, 5.0);
//}
//
//// G()
////http://www.jordanstevenstechart.com/physically-based-rendering  
////GGX GSF  This is a refactor of the Walter etal. GSF algorithm by multiplying the GSF by NdotV/NdotV. 
//
////NdotL = Normal * lightdir
////NdotV = Normal * viewdir
//float SchlickGGX(float roughness, float NdotL, float NdotV)
//{
//    float roughnessSqr = roughness * roughness;
//    float NdotLSqr = NdotL * NdotL;
//    float NdotVSqr = NdotV * NdotV;
//
//
//    float SmithL = (2 * NdotL) / (NdotL + sqrt(roughnessSqr +
// (1 - roughnessSqr) * NdotLSqr));
//    float SmithV = (2 * NdotV) / (NdotV + sqrt(roughnessSqr +
//(1 - roughnessSqr) * NdotVSqr));
//
//
//    float Gs = (SmithL * SmithV);
//    return 1.0; 
//   //float k = (roughness * roughness) * 0.5;
//   //float SmithL = (NdotL) / (NdotL * (1 - k) + k);
//   //float SmithV = (NdotV) / (NdotV * (1 - k) + k);
//   //float Gs = (SmithL * SmithV);
//   //return Gs;
//}
//float3 SpecBRDF(int i,float3 worldNormal,float3 specular, float roughness , float NdotV,float NdotH, float NdotL)
//{
//    float3 reflectLDir = normalize(reflect(-dirLights[i].direction.xyz,worldNormal));
//    float3 viewReflectDir = normalize(reflect(cameraDir.xyz, worldNormal));
//   
//
//    float3 F = 0.0;//   FresnelSchlick(specular.x, NdotV);
//    float D = TrowbridgeReitzGGX(roughness, NdotH);
//    float G = SchlickGGX(clamp(roughness, 0.02, 1.0), NdotL, NdotV);
//			
//    float3 f = D * F * G / (4.0 * NdotL * NdotV);
//    return f;
//}

float3 DirectBRDFDirectional(float3 albedo, float3 normal, float metallic, float roughness, int light);
float3 radiance(float3 N, float3 V, float roughness, float metallic, uint2 texCoord);

float4 PShader(VS_OUTPUT input) : SV_TARGET
{
    float4 finalResult = float4(0.0, 0.0, 0.0, 0);
    
    int2 texCoord = input.positionVS.xy;
    
    float4 albedo = diff.Load(int3(texCoord, 0));
    float4 specular = spec.Load(int3(texCoord, 0));
    float4 PBRData = pbrData.Load(int3(texCoord, 0));
	//x = Roughness, y = Metallic, z = EmissiveStrength, w = isEmissive ( non 0 = true)
    float4 miscData = misc.Load(int3(texCoord, 0));

    float Roughness = miscData.x;
    float Metallic = miscData.y;
    float Specular = specular.w;

    float ior = PBRData.x;
    float absorption = PBRData.y;
    float refraction = PBRData.z;
    //float3 F0 = PBRData.xyz;
    float4 worldPixel = pos.Load(int3(texCoord, 0));
    float3 worldNormal = normalize(norm.Load(int3(texCoord, 0)).xyz);

    float3 ambientLight = float3(0.15, 0.15, 0.15);
    float3 lightColor = float3(0.0,0.0,0.0);
	
    if (albedo.w == 0.0)
    {
        return albedo.xyzw;
    }
   
    if (visualType == 4.0)
    {
        float epsSmall = 0.01;
        float epsBig = 0.2;

        float dX = distance(worldPixel.xyz, pos.Load(int3(texCoord.x - 1.0 / screenWidth, texCoord.y, 0)).xyz) * 0.75;
        float dY = distance(worldPixel.xyz, pos.Load(int3(texCoord.x, texCoord.y - 1.0 / screenHeight, 0)).xyz) * 0.75;
        float d = distance(worldPixel.xyz, cameraPosition.xyz);
        float SmallDistanceValueX = epsSmall / dX;
        float BigDistanceValueX = epsBig / dX;
        float SmallDistanceValueY = epsSmall / dY;
        float BigDistanceValueY = epsBig / dY;
     
        float closenessTX = lerp(SmallDistanceValueX, BigDistanceValueX, d * 0.1);
        float closenessTY = lerp(SmallDistanceValueY, BigDistanceValueY, d * 0.1);
        float isEdgeX = dX > closenessTX ? 1 : 0;
        float isEdgeY = dY > closenessTY ? 1 : 0;
		
        return float4(float3((worldNormal.xyz + float3(1, 1, 1)) / 2.0) * saturate(isEdgeX + isEdgeY), 1.0);

    }
    else if (visualType == 5.0)
    {
        if (albedo.a == 0.0)
        {
			//sky so don't do anything
            return float4(albedo.xyz, 0);
        }
        int i = 0;
		//float depthVal = depth.Load(int3(texCoord, 0)).r;
        float finalLight = 0.0;
		
        for (i = 0; i < numDir; i++)
        {
            float lightResult = DoDirectionalLight(worldPixel, worldNormal, specular, i);
           // return lightResult;
			//lightColor += dirLights[i].color.xyz * lightResult;
            finalLight += lightResult;
        }
        for (i = 0; i < numSpot; i++)
        {
            finalLight += DoSpotLight(worldPixel, worldNormal, specular, i);
        }
		
        for (i = 0; i < numPoint; i++)
        {
            float lightResult = DoPointLight(worldPixel, worldNormal, specular, i);
            finalLight += lightResult;
			//lightColor += pointLights[i].color.xyz * lightResult;
        }
      
        float3 col = albedo.xyz;
        col *= finalLight + ambientLight;
               
        finalResult += float4(col, 0.0f);
        return finalResult; 
    }

    float3 camDir = normalize(-cameraDir.xyz);
    float3 kSpec = radiance(worldNormal, camDir, Roughness, Metallic, texCoord);
    float3 irrMap = textureSphereLod(IBLTex, worldNormal,0).rgb;
	//gamma correction
    irrMap = irrMap / (irrMap + float3(1.0, 1.0, 1.0));
    irrMap = pow(irrMap, float3(0.454545455f, 0.454545455f, 0.454545455f));

    float3 Kdiff = irrMap * albedo.xyz / PI;
    float3 col = BlendMaterial(albedo.xyz,Kdiff, kSpec,Metallic);

    if(visualType == 6.0)
    {
        return float4(col, 1.0);
    }
    
	uint i = 0;
		
    float3 directLighting = float3(0, 0, 0);
    float NdotV = dot(worldNormal, camDir);
	
    for (i = 0; i < numDir; i++)
    {		
        float3 PBR = DirectBRDFDirectional(albedo.xyz, worldNormal, Metallic, Roughness, i);
		if(visualType == 0.0)
        {
            return float4(PBR + col, 1);
        }
		else
        {
            return float4(PBR, 1);
        }
		
		
		
		
		////float isShadow = DoDirectionalLight(worldPixel, worldNormal, specular, i);
		//       
		////lambertian
        //float3 fDiffuse = albedo.xyz / PI;
        //    
        //// miscData.x = clamp(miscData.x, 0.04, 0.96);
		////Microfacet Specular BRDF
        //float fD = D(Roughness, worldNormal, halfDir);
        ////float fD = TrowbridgeReitzGGX(Roughness, dot(worldNormal, normLDir));
        //// float fG = G(Roughness, worldNormal, cameraDir.xyz) * G(Roughness, worldNormal, dirLights[i].direction.xyz);
        //float fG = SchlickGGX(Roughness,NdotL,NdotV);
		//
        //float3 F0 = Metallic < 0.5f ? CalcF0(ior) : CalcF0(absorption, refraction);
        //float fF = FresnelTerm(ior, dot(normLDir, halfDir)); // Fresnel(Metallic, F0, albedo.xyz, NdotH); //F(Metallic,Specular, cameraDir.xyz,halfDir);
		//	
		//
        //float3 f = float3(0, 0, 0);
        //if (visualType == 0.0f)
        //{
        //    f = fD * fF * fG / (4.0 * (NdotL * NdotL));
        //    //return float4(f, 1.0);
        //    return float4(fDiffuse + specular.xyz * f, 1.0);
        //}
        //else if (visualType == 1.0f)
        //{
        //    return float4(fD, fD, fD, 1.0);
        //}
        //else if (visualType == 2.0f)
        //{
        //    return float4(fF, fF, fF, 1.0);
        //}
        //else if (visualType == 3.0f)
        //{
        //    return float4(fG, fG, fG, 1.0);
        //}
        //directLighting += fDiffuse + f;
           
	
		//directDiffuseFactor = DiffuseBRDF(directlightSource)
		//
        //directSpecularFactor = SpecularBRDF(directLightSource)
		//
        //directLighting += Albedo * directDiffuseFactor + SpecColor * directSpecularFactor
	
    }
    return float4(directLighting.xyz, 1);




	//if (visualType == 0.0)
    //{
    //    
    //   // finalResult += float4(col, 0.0f);
	//
    //}
    //else if (visualType == 1.0)
    //{
    //    if (albedo.a == 0.0)
    //    {
	//		//sky so don't do anything
    //        return float4(0, 0.2, 0.4, 0);
    //    }
    //    int i = 0;
	//	//float depthVal = depth.Load(int3(texCoord, 0)).r;
    //    float finalLight = 0.0;
	//	
    //    for (i = 0; i < numDir; i++)
    //    {
    //        float lightResult = DoDirectionalLight(worldPixel, worldNormal, specular, i);
    //       // return lightResult;
	//		//lightColor += dirLights[i].color.xyz * lightResult;
    //        finalLight += lightResult;
    //    }
    //    for (i = 0; i < numSpot; i++)
    //    {
    //        finalLight += DoSpotLight(worldPixel, worldNormal, specular, i);
    //    }
	//	
    //    for (i = 0; i < numPoint; i++)
    //    {
    //        float lightResult = DoPointLight(worldPixel, worldNormal, specular, i);
    //        finalLight += lightResult;
	//		//lightColor += pointLights[i].color.xyz * lightResult;
    //    }
    //  
    //    float3 col = albedo.xyz;
    //    col *= finalLight + ambientLight;
    //           
    //    finalResult += float4(col, 0.0f);
    //}
	//else if(visualType == 2.0)
    //{
    //    return float4(miscData.x, miscData.x, miscData.x, 1.0);
    //}
    //else if (visualType == 3.0)
    //{
    //    return float4(miscData.y, miscData.y, miscData.y, 1.0);
    //}
    //else if (visualType == 4.0)
    //{
    //    return float4(specular.x, specular.y, specular.z, 1.0);
    //}
    //else
    //{
    //    finalResult = float4(reflect(cross(ddx_fine(worldPixel).xyz, ddy_fine(worldPixel).xyz), worldNormal), 1);
    //    return finalResult;
    //}
    return finalResult;
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
float compute_lod(uint NumSamples, float NoH,float k)
{
    float dist = DGgx(NoH, k); // Defined elsewhere as subroutine
    return 0.5 * (log2(float(screenWidth * screenHeight) / NumSamples) - log2(dist));
	//Check correctness, might need to be the IBL's resolutions instead of the screen's
}


//GGxSample (S)
//E = output from Hammersley()
//k = roughness
float3 S(float2 E, float k, uint2 texCoord)
{
    float a = k * k;
    float Theta = atan(sqrt((a * E.x) / (1.0 - E.x)));
    float Phi = PI2 * E.y;
    return MakeSample(Theta, Phi,texCoord);
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
    return GSmithSchlickBeckmann_(NoL,k) * GSmithSchlickBeckmann_(NoV,k);
}
float2 SphereMap(float3 N)
{
    return float2(0.5 - atan2(N.y, N.x) / PI2, acos(N.z) / PI);
}
float4 textureSphereLod(TextureCube tex, float3 N, float lod)
{
    return tex.SampleLevel(SampleType[0],N, lod);
}

float GeometryGGXSchlick(float NdotV, float roughness)
{
    float nom = NdotV;
    float denom = NdotV * (1.0 - roughness) + roughness;
	
    return nom / denom;
}
float GeometryGGX(float NdotV, float NdotL, float roughness)
{
    float ggx2 = GeometryGGXSchlick(NdotV, roughness);
    float ggx1 = GeometryGGXSchlick(NdotL, roughness);
	
    return ggx1 * ggx2;
}

float3 radiance(float3 N, float3 V, float roughness,float metallic, uint2 texCoord)
{
  // Precalculate rotation for +Z Hemisphere to microfacet normal.
    float3 UpVector = abs(N.z) < 0.9999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = cross(N, TangentX);
 
    //float NoV = max(dot(N, V), 0.0);
    float NoV = abs(dot(N, V));
 
  // Approximate the integral for lighting contribution.
    float3 fColor = float3(0,0,0);
    const uint NumSamples = 200;
    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 Xi = Hammersley(i, NumSamples);
        float3 Li = S(Xi,roughness,texCoord); 
        float3 H = normalize(Li.x * TangentX + Li.y * TangentY + Li.z * N);
        float3 L = normalize(-reflect(V, H));
 
    // Calculate dot products for BRDF
        float NoL = max(dot(N, L), 0.0);
        //float NoH = max(dot(N, H), 0.0);
        //float VoH = max(dot(V, H), 0.0);
		
       // float NoL = abs(dot(N, L));
        float NoH = abs(dot(N, H));
        float VoH = abs(dot(V, H));

        float lod = compute_lod(NumSamples, NoH,roughness);
 
        float F_ = FSchlick(VoH,metallic); 
        float G_ = GSmithSchlickBeckmann(NoL, NoV, NoH, VoH,roughness);
        float3 LColor = textureSphereLod(SkyTex, L, lod).rgb;
        LColor = LColor / (LColor + float3(1.0, 1.0, 1.0));
        LColor = pow(LColor, float3(0.454545455f, 0.454545455f, 0.454545455f));
 
    // Since the sample is skewed towards the Distribution, we don't need
    // to evaluate all of the factors for the lighting equation. Also note
    // that this function is calculating the specular portion, so we absolutely
    // do not add any more albedo here.
        fColor += F_ * G_ * LColor * VoH / (NoH * NoV);
    }
 
  // Average the results
    return fColor / float(NumSamples);
}



//n = refractive index
float getF0(float n)
{
    return pow((1 - n) / (1 + n), 2);
}

// GGX/Throwbridge-Reitz
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = abs(dot(N, H));
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
float3 FresnelSphericalGaussian(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(2.0, (-5.55473 * cosTheta - 6.98316 * cosTheta));
}
float3 DirectBRDFDirectional(float3 albedo, float3 normal, float metallic, float roughness, int light)
{
	 // input
    float3 N = normalize(normal);
    float3 V = normalize(-cameraDir.xyz);
    float3 L = normalize(-dirLights[light].direction.xyz);
    float3 H = normalize(L + V);
    float NdotL = dot(N, L);
    

	//D
    float D = DistributionGGX(N, H, roughness);
	
	//F
   // float3 F0 = float3(F0x, F0y, F0z);
	//albedo == F0
    float3 F = FresnelSchlick(saturate(dot(V, H)), albedo); // Fresnel(metallic, F0, albedo, dot(V, H));
	

	//G
    float G = GeometryGGX(max(dot(N, H), 0.0), max((NdotL), 0.0), roughness);

    
    float3 nominator = D * G * F;
    float denominator = 4 * max(dot(N, V),0.0) * max(NdotL,0.0) + 0.001;
    float3 specular = nominator / denominator;
        
    // add to outgoing radiance Lo
    float3 Lo = (F - albedo / PI + specular) * dirLights[light].color.xyz * max(NdotL,0.0);
    
    if (visualType == 7.0f)
    {
        return Lo;
    }
    else if (visualType == 1.0f)
    {
        return float3(D, D, D);
    }
    else if (visualType == 2.0f)
    {
        return F;
    }
    else if (visualType == 3.0f)
    {
        return float3(G, G, G);
    }
	
    return Lo;
}




float DoDirectionalLight(float4 worldPixel, float3 worldNormal, float2 specular, int i)
{
   float lightStr = max(-dot(dirLights[i].direction.xyz, worldNormal.xyz), 0.0);
   float4x4 view = (dirLights[i].lightviewmatrix);
   float4x4 proj = (dirLights[i].lightprojmatrix);
   float4 viewPosition = worldPixel;
   viewPosition = mul(viewPosition, view);
   viewPosition = mul(viewPosition, proj);

   float3 projectTexCoord;
   float2 texOffset = dirLights[i].texOffset.xy / 8192.0;
   projectTexCoord.x = viewPosition.x / viewPosition.w * 0.5 + 0.5f;
   projectTexCoord.y = -viewPosition.y / viewPosition.w * 0.5 + 0.5f;
   projectTexCoord.z = viewPosition.z / viewPosition.w;
     
   float visibility = 0;
   if ((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
   {
       projectTexCoord.xy /= (8192.0 / dirLights[i].texOffset.zw);
       projectTexCoord.xy += texOffset.xy;
       float shadowVal = shadow.Sample(SampleType[0], projectTexCoord.xy).r;
       if (shadowVal > viewPosition.z - 0.001f)
       {
           visibility = 1.0;
       }
   }
    if (visualType == 0)
    {
        return visibility;
    }
	else
    {
        return visibility * lightStr;
    }
}
float DoPointLight(float4 worldPixel, float3 worldNormal, float2 specular, int i)
{
    float normalStrength = saturate(-dot(normalize(worldPixel.xyz - pointLights[i].position.xyz), worldNormal));
    float dist = distance(worldPixel.xyz, pointLights[i].position.xyz);
    float4x4 proj = (pointLights[i].lightprojmatrix);
    float totalVis = 0.0;
    for (int j = 0; j < 6; j++)
    {
        float4x4 view = (pointLights[i].lightviewmatrix[j]);
        float4 viewPosition = worldPixel;
        viewPosition = mul(viewPosition, view);
        viewPosition = mul(viewPosition, proj);
        float3 projectTexCoord;
        projectTexCoord.x = viewPosition.x / viewPosition.w * 0.5 + 0.5f;
        projectTexCoord.y = -viewPosition.y / viewPosition.w * 0.5 + 0.5f;
        projectTexCoord.z = viewPosition.z / viewPosition.w;
        float visibility = 0;
        if ((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
        {
            float2 texOffset = pointLights[i].texOffset[j].xy / 8192.0;
            projectTexCoord.xy /= (float2(8192.0, 8192.0) / pointLights[i].texOffset[j].zw);
            projectTexCoord.xy += texOffset.xy;
            float shadowVal = shadow.Sample(SampleType[0], projectTexCoord.xy).r;
            if (shadowVal > projectTexCoord.z - 0.001f)
            {
                visibility = 1.0;
            }
        }
				
        totalVis += visibility;
    }
    float distanceStrength = 1.0 / ((dist * dist) / pointLights[i].color.w);
    float lightResult = totalVis * normalStrength * distanceStrength;
    return lightResult;
}
float DoSpotLight(float4 worldPixel, float3 worldNormal, float2 specular, int i)
{
    float lightStr = max(-dot(spotLights[i].direction.xyz, worldNormal.xyz), 0.0);
    float4x4 view = (spotLights[i].lightviewmatrix);
    float4x4 proj = (spotLights[i].lightprojmatrix);
    float4 viewPosition = worldPixel;
    viewPosition = mul(viewPosition, view);
    viewPosition = mul(viewPosition, proj);

    float3 projectTexCoord;
    float2 texOffset = spotLights[i].texOffset.xy / 8192.0;
    projectTexCoord.x = viewPosition.x / viewPosition.w * 0.5 + 0.5f;
    projectTexCoord.y = -viewPosition.y / viewPosition.w * 0.5 + 0.5f;
    projectTexCoord.z = viewPosition.z / viewPosition.w; //* 0.5 + 0.5;
     
    float visibility = 0;
    if ((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))//  && viewPosition.z > 0.0)
    {
        projectTexCoord.xy /= (8192.0 / spotLights[i].texOffset.zw);
        projectTexCoord.xy += texOffset.xy;
        float shadowVal = shadow.Sample(SampleType[0], projectTexCoord.xy).r;
      //  return projectTexCoord.z;
       // return Remap(viewPosition.z, 0, 0.1, 0, 1.0);
        if (shadowVal > projectTexCoord.z - 0.001f)
        {
            visibility = 1.0;
        }
    }

    float3 lightToPixel = worldPixel.xyz - spotLights[i].position.xyz;
    float dist = length(lightToPixel);
    lightToPixel = normalize(lightToPixel);
    float cosDir = dot(lightToPixel, -spotLights[i].direction.xyz);
    float effect = smoothstep(spotLights[i].angleMax, spotLights[i].angleMin, cosDir);
    float att = smoothstep(spotLights[i].range, 0.0, dist);

    return (lightStr * visibility);
}