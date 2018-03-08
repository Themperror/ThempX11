#pragma pack_matrix( row_major )
struct VS_OUTPUT
{
    float4 positionVS : SV_POSITION;
    float3 normalVS : NORMAL0;
    float3 positionWS : POSITION;
    float3 uv : UV0;
};
struct PS_OUTPUT
{
    float4 diffuse : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 specular : SV_TARGET2;
    float4 misc : SV_TARGET3;
    float4 pos : SV_TARGET4;
    float4 pbrData : SV_TARGET5;
};

//Texture2D shaderTexture[4];

 
TextureCube diff : register(t0);
SamplerState diffSampler : register(s0);

PS_OUTPUT PShader(VS_OUTPUT input)
{
    PS_OUTPUT output;
    output.pos = float4(input.positionWS, 1.0f);

    float3 envColor = diff.SampleLevel(diffSampler, input.uv, 0).xyz;
	//gamma correction (linear to gamma space, since HDR's are Linear by default)
    envColor = envColor / (envColor + float3(1.0, 1.0, 1.0));
    envColor = pow(envColor, float3(0.454545455f, 0.454545455f, 0.454545455f));


    output.diffuse = float4(envColor, 0.0); //mark as sky so it doesn't get lighting pass


    //output.diffuse = float4(1,1,1, 1.0); //mark as sky so it doesn't get lighting pass
    output.normal = float4(normalize(input.normalVS), 4);
    output.specular = float4(0.0, 0.0, 0.0, 0.0);
    output.misc = float4(0.0, 0.0, 0.0, 0.0);
    output.pbrData = float4(0.0, 0.0, 0.0, 0.0);

    return output;
}