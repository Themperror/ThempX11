#include "Defines.hlsl"
#include "Structs.hlsl"

[maxvertexcount(18)]
void GShader(triangle VS_SHADOW_OUTPUT input[3], inout TriangleStream<VS_SHADOW_OUTPUT> CubeMapStream)
{
    for (int f = 0; f < 6; ++f)
    {
        VS_SHADOW_OUTPUT output;
        output.vpi = f;
        output.positionWS = float3(0, 0, 0);
		// Compute screen coordinates for each vertex
        for (int v = 0; v < 3; v++)
        {
            float4 nPos = mul(float4(input[v].positionWS, 1), _pointLights[0].lightviewmatrix[f]);
            output.positionVS = mul(nPos, _pointLights[0].lightprojmatrix);
            CubeMapStream.Append(output);
        }
        CubeMapStream.RestartStrip();
    }
}