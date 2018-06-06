#pragma once
#include "ThempShadow.h"
namespace Themp
{
	using namespace DirectX;
	class D3D;

	class ShadowVariance : Shadow
	{
	public:
		
		ShadowVariance();
		~ShadowVariance();
		virtual void DrawShadow();
		virtual void DrawLight();
		virtual void PreDraw();
		virtual void SetMultiSample(int num);
		virtual void SetDirty();
	};
};