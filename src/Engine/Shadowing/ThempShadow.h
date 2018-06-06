#pragma once
#include <DirectXMath.h>
#include <d3d11.h>

#define NUM_LIGHTS 3
#define ATLAS_RESOLUTION 2048  //Lights are atlassed, so if you want 3 directional lights on a 2048 atlas, they'll have to be 1024x1024 maximum in size
#define SHADOW_RESOLUTION 1024

namespace Themp
{
	using namespace DirectX;
	class Shadow
	{
	public:
		virtual void DrawShadow() = 0;
		virtual void DrawLight() = 0;
		virtual void PreDraw() = 0;
		virtual void SetMultiSample(int num) = 0;
		virtual void SetDirty() = 0;
	};
};