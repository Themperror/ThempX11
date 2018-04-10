#pragma once
#include <DirectXMath.h>
#include <d3d11.h>

#define NUM_LIGHTS 3

namespace Themp
{
	using namespace DirectX;
	class Shadow
	{
	public:
		virtual void SetDirty() = 0;
		virtual void DrawShadow() = 0;
		virtual void DrawLight() = 0;
		virtual void PreDraw() = 0;
		virtual void SetMultiSample(int num) = 0;
	};
};