#pragma once
#include "ThempShadow.h"
#define MAX_CASCADES 6
namespace Themp
{
	using namespace DirectX;
	class D3D;
	class Camera;
	class Material;

	class ShadowCascade : Shadow
	{
		struct DirectionalLight
		{
			XMFLOAT4X4 m_ProjectionMatrix[MAX_CASCADES];
			XMFLOAT4X4 m_ViewMatrix;
			XMFLOAT4 m_LightDirection;
			XMFLOAT4 m_LightPosition;
			XMFLOAT4 m_LightColor;
		};
		struct CascadeLightBuffer
		{
			int numDirs = 0, d0, d1, d2;
			DirectionalLight _dirLights[3];
		};

	public:
		
		ShadowCascade();
		~ShadowCascade();
		virtual void DrawShadow();
		virtual void DrawLight();
		virtual void PreDraw();
		virtual void SetDirty();
		virtual void SetMultiSample(int num);

		void SetCascade(int numCascades, float nearPlane, float farPlane);

		std::vector<float> m_SplitFar;
		std::vector<float> m_SplitNear;
		Material* m_DirectionalShadowMaterial;
		Material* m_ShadowMaterial;
		Material* m_CascadedLightingMaterial;
		int m_NumCascades = 0;

		Camera* m_ShadowCamera = nullptr;

		CascadeLightBuffer m_CascadeLightBufferData;

	};
};