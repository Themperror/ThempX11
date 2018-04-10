#include "ThempSystem.h"
#include "ThempShadowCascade.h"
#include "ThempD3D.h"
#include "ThempResources.h"
#include "ThempCamera.h"

#include <iostream>
using namespace DirectX;
namespace Themp
{
	ShadowCascade::ShadowCascade()
	{
		m_ShadowCamera = new Camera();
		m_ShadowCamera->SetProjection(Camera::CameraType::Orthographic);
		m_CascadedLightingMaterial = Resources::TRes->GetMaterial("CascadedLighting", "", "DeferredLightingCascade", true, true, false);
		m_DirectionalShadowMaterial = Resources::TRes->GetMaterial("ShadowDirectionalCascaded", "", "DeferredShadowCascade", true, true, true);
		if (D3D::s_D3D->SupportsVPArrayIndex)
		{
			m_ShadowMaterial = Resources::TRes->GetMaterial("DeferredShadow", "", "DeferredShadow", true, true, true);
		}
		else
		{
			m_ShadowMaterial = Resources::TRes->GetMaterial("DeferredShadowFallback", "", "DeferredShadowFallback", true, true, false);
		}
	}

	//Set the Cascade shadow map settings, this recalculates the projection matrix using the number of cascades and its near and far plane
	void ShadowCascade::SetCascade(int numCascades,float nearPlane, float farPlane)
	{
		numCascades = numCascades > MAX_CASCADES ? MAX_CASCADES : numCascades < 2 ? 2 : numCascades;
		m_NumCascades = numCascades;
		m_SplitFar.clear();
		m_SplitNear.clear();
		m_SplitFar.reserve(numCascades);
		m_SplitNear.reserve(numCascades);
		for (size_t i = 0; i < numCascades; i++)
		{
			m_SplitFar.push_back(farPlane);
			m_SplitNear.push_back(nearPlane);
		}
		//calculate the projection near and far plane for each cascade (edited from: https://promethe.io/2015/01/21/exponential-cascaded-shadow-mapping-with-webgl/ )
		float strength = 0.8f;
		float j = 1.0f;
		for (int i = 0; i < numCascades - 1; ++i, j += 1.0f)
		{
			m_SplitFar[i] = lerp(
				nearPlane + (j / (float)numCascades) * (farPlane - nearPlane),
				nearPlane * powf(farPlane / nearPlane, j / (float)numCascades),
				strength
			);
			m_SplitNear[i + 1] = m_SplitFar[i];
		}
	}
	ShadowCascade::~ShadowCascade()
	{
		delete m_ShadowCamera;
	}
	void ShadowCascade::DrawShadow()
	{
		for (size_t i = 0; i < m_CascadeLightBufferData.numDirs; i++)
		{
			DirectionalLight& l = m_CascadeLightBufferData._dirLights[i];
			m_ShadowCamera->SetPosition(l.m_LightPosition);
			m_ShadowCamera->SetLookTo(l.m_LightDirection, XMFLOAT3(0, 1, 0),XMFLOAT3(1,0,0));
			//m_CascadeLightBufferData._dirLights[i].m_ProjectionMatrix = 
		}
	}
	void ShadowCascade::DrawLight()
	{

	}
	void ShadowCascade::PreDraw()
	{

	}
	void ShadowCascade::SetDirty()
	{

	}
	void ShadowCascade::SetMultiSample(int num)
	{

	}
}