#include "ThempSystem.h"
#include "ThempShadowVariance.h"
#include "ThempD3D.h"
#include "ThempResources.h"
#include <DirectXMath.h>

#include <iostream>
using namespace DirectX;
namespace Themp
{
	ShadowVariance::ShadowVariance()
	{

	}
	ShadowVariance::~ShadowVariance()
	{

	}
	void ShadowVariance::DrawShadow()
	{
	}
	void ShadowVariance::DrawLight()
	{
	}
	void ShadowVariance::PreDraw()
	{
	}
	void ShadowVariance::SetDirty()
	{
		//m_DirtyLights = true;
		for (size_t i = 0; i < NUM_LIGHTS * 3; i++)
		{
			//m_Lights[i].LightIsDirty = true;
		}
	}
	void ShadowVariance::SetMultiSample(int num)
	{
	}
}