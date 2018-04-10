#pragma once
#include "ThempShadow.h"
namespace Themp
{
	using namespace DirectX;
	class D3D;

	class ShadowMoment : Shadow
	{
	public:
		
		ShadowMoment();
		~ShadowMoment();
		virtual void DrawShadow();
		virtual void DrawLight();
		virtual void PreDraw();
		virtual void SetDirty();
		virtual void SetMultiSample(int num);
	};
};