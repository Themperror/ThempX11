#include "Defines.hlsl"


Texture2D diff : register(t0);
Texture2D norm : register(t1);
Texture2D spec : register(t2);
Texture2D misc : register(t3);
Texture2D pos : register(t4);
Texture2D depth : register(t5);
Texture2D shadow : register(t6);

SamplerState SampleType[4];

#define SHADOW_BIAS 0.5

float DoPointLight(float4 worldPixel,float3 N, int i);
float DoDirectionalLight(float4 worldPixel, float3 N, int i);
float DoSpotLight();


float4 PShader(VS_OUTPUT input) : SV_TARGET
{
    float4 finalResult = float4(0.0, 0.0, 0.0, 0);
    
    float4 diffuse = diff.Sample(SampleType[0], input.uv);
    int2 texCoord = input.positionVS.xy;
    float4 worldPixel = pos.Load(int3(texCoord, 0));

    float3 N = norm.Load(int3(texCoord, 0)).xyz;

    float3 ambientLight = float3(0.15, 0.15, 0.15);
    float3 lightColor = float3(1.0,1.0,1.0);
	
	if (visualType == 0.0)
    {
        if (diffuse.a == 0.0)
        {
			//sky so don't do anything
            return float4(0, 0.2, 0.4, 0);
        }
        int i = 0;
		float depthVal = depth.Load(int3(texCoord, 0)).r;
        float finalLight = 0.0;
		
        for (i = 0; i < numDir; i++)
        {
            float lightResult = DoDirectionalLight(worldPixel, N, i);
            lightColor += dirLights[i].color.xyz * lightResult;
            finalLight += lightResult;
        }
        for (i = 0; i < numSpot; i++)
        {
            finalLight += DoSpotLight();
        }
		
        for (i = 0; i < numPoint; i++)
        {
            float lightResult = DoPointLight(worldPixel, N, i);
            finalLight += lightResult;
            lightColor += pointLights[i].color.xyz * lightResult;
        }
        
        float3 col = diffuse.xyz * lightColor;
        col *= finalLight + ambientLight;
               
        finalResult += float4(col, 1.0);
    }
    else if (visualType == 1.0)
    {
        float2 screenUV = float2(input.positionVS.x / screenWidth, input.positionVS.y / screenHeight);

        if (input.positionVS.x < screenWidth / 2 && input.positionVS.y < screenHeight / 2)
        {
            finalResult = diff.Sample(SampleType[0], screenUV * 2);
        }
        else if (input.positionVS.x >= screenWidth / 2 && input.positionVS.y < screenHeight / 2)
        {
            finalResult = norm.Sample(SampleType[1], screenUV * 2);
        }
        else if (input.positionVS.x >= screenWidth / 2 && input.positionVS.y >= screenHeight / 2)
        {
            float depthv = depth.Sample(SampleType[2], screenUV * 2).r;
            depthv = Remap(depthv, 0.0, 1.0, 0.94, 1.0);
            finalResult = float4(depthv, depthv, depthv, 1.0f);
        }
        else if (input.positionVS.x < screenWidth / 2 && input.positionVS.y >= screenHeight / 2)
        {
            float depthv = shadow.Sample(SampleType[3], screenUV*2).r;
           // depthv = Remap(depthv, 0.0, 1.0, 0.9, 1.0);
            finalResult = float4(depthv, depthv, depthv, 1.0f);
        }
    }
	else if(visualType == 2.0)
    {
       // finalResult = diff.Sample(SampleType[0], input.uv);
        finalResult = float4(reflect(cross(ddx_fine(worldPixel).xyz, ddy_fine(worldPixel).xyz), N), 1);

    }
    else
    {
        return diffuse;
    }
    return finalResult;
}

float DoDirectionalLight(float4 worldPixel,float3 N, int i)
{
   //float4x4 view = (dirLights[i].lightviewmatrix);
   //float4x4 proj = (dirLights[i].lightprojmatrix);
   //float4 viewPosition = worldPixel;
   //viewPosition = mul(viewPosition, view);
   //viewPosition = mul(viewPosition, proj);
   //float3 projectTexCoord;
   //projectTexCoord.x = viewPosition.x / viewPosition.w * 0.5 + 0.5f;
   //projectTexCoord.y = -viewPosition.y / viewPosition.w * 0.5 + 0.5f;
   //projectTexCoord.z = viewPosition.z / viewPosition.w;
   //        
   //float3 lightDirection = -dirLights[i].direction;
   //float2 texOffset = dirLights[i].texOffset.xy / 8192.0;
   //projectTexCoord.xy /= (8192.0 / dirLights[i].texOffset.zw);
   //projectTexCoord.xy += texOffset.xy;
   //        //bias based on angle
   //float d = dot(lightDirection, N.xyz);
   //float bias = 0.005 * tan(acos(d));
   //bias = clamp(bias, 0, 0.01);
   //        
   //float fd = projectTexCoord.z - bias;
   //float2 texelSize = 1.0 / 8192; //8192 = shadowmap size
   //float factor = 0.0;
   //        
   //if ((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
   //{
   //    //"multi" sampling the texture, smoothens edges
   //    for (float y = -1.0; y < 1.01; y++)
   //    {
   //        for (float x = -1.0; x < 1.01; x++)
   //        {
   //            float3 uvc = float3(projectTexCoord.xy + float2(x, y) * texelSize, fd);
   //            float comp = 1.0;
   //            if (shadow.Sample(SampleType[0], uvc.xy).r < uvc.z)
   //            {
   //                comp = 0;
   //            }
   //            factor += comp;
   //        
   //        }
   //    }
   //}
   //else
   //{
   //    factor = 0.0;
   //}
   //        
   //factor /= 9.0;
   //        
   //eturn saturate(factor * max(d, 0.0));


	///////////////   
   float lightStr = max(-dot(dirLights[i].direction.xyz, N.xyz), 0.0);
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
   return visibility * lightStr;
}
float DoPointLight(float4 worldPixel,float3 N, int i)
{
    float normalStrength = saturate(-dot(normalize(worldPixel.xyz - pointLights[i].position.xyz), N));
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
float DoSpotLight()
{
    return 0.0;
}