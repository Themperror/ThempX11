cbuffer GS_CONSTANT_BUFFER : register(b0)
{
	float camX;
	float camY;
    float tilesize;
    float sWidth;
    float sHeight;
    float zoom;
	uint tileTexSize;
	uint texWidth;
};

struct GS_INPUT
{
    float4 pos : SV_POSITION;
	uint texID : TEX_ID0;
};
struct GSPS_OUTPUT
{
    float4 pos : SV_POSITION;
	float2 uv : UV0;
};

[maxvertexcount(4)]
void GS( point  GS_INPUT input[1], inout TriangleStream<GSPS_OUTPUT> TriStream )
{
    GSPS_OUTPUT output;
    float cTileSizeX = (tilesize * (1.0f/sWidth) );
    float cTileSizeY = (tilesize * (1.0f/sHeight));
	
	uint numTilesWidth =  texWidth / tileTexSize;
	float numTilesWidthf =  texWidth / tileTexSize;
	float maxTiles = (numTilesWidthf*numTilesWidthf);
	float texS = (1.0 / numTilesWidthf);
	float pixS = texS / tileTexSize;
	
	float TexX =  input[0].texID % numTilesWidth;
	float TexY = (input[0].texID / numTilesWidth);
	
	//float texIDx = fmod((float)input[0].texID , numTilesWidth);
	//float texIDy = floor(input[0].texID / texWidth);
	
	float texX = TexX / numTilesWidthf;
	float texY = TexY / numTilesWidthf;
	
	float4 scaledPos = input[0].pos*float4(2.0f/sWidth,2.0f/sHeight,1,1);
	output.pos = scaledPos + float4(-cTileSizeX,cTileSizeY,0,0) + float4(camX,camY,0,0);
	output.uv = float2(texX,texY);
	TriStream.Append( output );
	
	output.pos = scaledPos + float4(cTileSizeX,cTileSizeY,0,0)+ float4(camX,camY,0,0);
	output.uv = float2(texX +texS,texY);
	TriStream.Append( output );
	
	output.pos = scaledPos + float4(-cTileSizeX,-cTileSizeY,0,0)+ float4(camX,camY,0,0);
	output.uv = float2(texX,texY+texS);
	TriStream.Append( output );

	output.pos = scaledPos + float4(cTileSizeX,-cTileSizeY,0,0)+ float4(camX,camY,0,0);
	output.uv = float2(texX+texS,texY+texS);
	TriStream.Append( output );
}