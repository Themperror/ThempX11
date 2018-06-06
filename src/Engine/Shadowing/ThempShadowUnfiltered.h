#pragma once
#include "ThempShadow.h"
namespace Themp
{
	using namespace DirectX;
	class D3D;
	class Camera;
	class ShadowAtlas;
	class RenderTexture;
	class Material;
	class ShadowUnfiltered : Shadow
	{
		struct LightData
		{
			uint32_t enabled, dummy0, dummy1, dummy2;
			XMFLOAT4 position;
			XMFLOAT4 color;
			XMFLOAT4X4 lightprojmatrix;
		};
		struct DirectionalLight : public LightData
		{
			XMFLOAT4 direction;
			//float range, type, dummyRZ,dummyRW; //64
			XMFLOAT4X4 lightviewmatrix; //128
			XMFLOAT4 textureOffset;//x offset, y offset, width, height;
		};
		struct SpotLight : public LightData
		{
			XMFLOAT4 direction;
			float angleMin, angleMax, range, dummyRW; //64
			XMFLOAT4X4 lightviewmatrix; //128
			XMFLOAT4 textureOffset;//x offset, y offset, width, height;
		};
		struct PointLight : public LightData
		{
			//float range, type, dummyRZ,dummyRW; //64
			XMFLOAT4X4 lightviewmatrix[6]; //128
			XMFLOAT4 textureOffset[6];//x offset, y offset, width, height;
		};
		struct LightConstantBuffer
		{
			unsigned int numDir, numSpot, numPoint, dummy;
			DirectionalLight dirLights[NUM_LIGHTS];
			PointLight pointLights[NUM_LIGHTS];
			SpotLight spotLights[NUM_LIGHTS];
		};

		struct LightShadowMap
		{
			LightData* l;
			bool LightIsDirty;
		};

	public:
		ShadowUnfiltered();
		~ShadowUnfiltered();
		virtual void DrawShadow();
		virtual void DrawLight();
		virtual void PreDraw();
		virtual void SetMultiSample(int num);
		virtual void SetDirty();

		void SetLightDirty(int type, int index);
		void SetDirectionalLight(int index, bool enabled, XMFLOAT4 pos, XMFLOAT4 dir, XMFLOAT4 color);

		void DrawDirectionalShadow();
		void DrawPointShadow();
		void DrawSpotShadow();

		void RenderShadowsPointCamera(PointLight * l);
		void RenderShadowsSpotCamera(SpotLight * l);
		void RenderShadowsDirectionalCamera(DirectionalLight * l);

		bool m_DirtyLights = true;
		Camera* m_ShadowCamera = nullptr;
		ShadowAtlas* m_ShadowAtlas = nullptr;
		RenderTexture* m_ShadowTexture = nullptr;
		Material* m_ShadowMaterial = nullptr;
		Material* m_ShadowClearMaterial = nullptr;
		Material* m_UnfilteredLightingMaterial = nullptr;
		LightShadowMap m_ShadowMaps[NUM_LIGHTS*3];
		LightConstantBuffer m_LightConstantBufferData;
		ID3D11Buffer* m_LightBuffer = nullptr;
	};
};