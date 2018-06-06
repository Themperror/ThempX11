#pragma once
#include "ThempShadow.h"
#define MAX_CASCADES 6
#undef far
#undef near

namespace Themp
{
	using namespace DirectX;
	class D3D;
	class Camera;
	class Material;
	class ShadowAtlas;
	class RenderTexture;

	class ShadowCascade : Shadow
	{
		struct Frustum
		{
			XMFLOAT3 corners[8];
			float RightSlope;           // Positive X slope (X/Z).
			float LeftSlope;            // Negative X slope.
			float TopSlope;             // Positive Y slope (Y/Z).
			float BottomSlope;          // Negative Y slope.
			float Near, Far;            // Z of the near plane and far plane.
		};
		//struct Frustum
		//{
		//	//view info
		//	float near, far,fov, ratio;
		//
		//	//bounding box corners
		//	XMFLOAT4 corners[8];
		//
		//	//proj info
		//	float r,l; //left right
		//	float b,t; //bottom top
		//	float n,f; //z-near z-far
		//};
		struct LightData
		{
			uint32_t enabled, dummy0, dummy1, dummy2;
			XMFLOAT4 position;
			XMFLOAT4 color;
		};
		struct DirectionalLight : public LightData
		{
			XMFLOAT4X4 lightProjectionMatrix[MAX_CASCADES];
			XMFLOAT4X4 lightViewMatrix[MAX_CASCADES];
			XMFLOAT4 direction;
			XMFLOAT4 textureOffset;//x offset, y offset, width, height;
		}; 
		struct PointLight : public LightData
		{
			//float range, type, dummyRZ,dummyRW; //64
			XMFLOAT4X4 lightProjMatrix;
			XMFLOAT4X4 lightViewMatrix[6]; //128
			XMFLOAT4 textureOffset[6];//x offset, y offset, width, height;
		};
		struct SpotLight : public LightData
		{
			XMFLOAT4 direction;
			float angleMin, angleMax, range, dummyRW; //64
			XMFLOAT4X4 lightProjMatrix;
			XMFLOAT4X4 lightViewMatrix; //128
			XMFLOAT4 textureOffset;//x offset, y offset, width, height;
		};
		
		struct CascadeLightBuffer
		{
			uint32_t numDir=0, numSpot=0, numPoint=0, cascadeIndex =0;
			float split0, split1, split2, split3;
			float split4, split5, split6, split7;
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
		
		ShadowCascade(int num_Cascades);
		~ShadowCascade();
		virtual void DrawShadow();
		virtual void DrawLight();
		virtual void PreDraw();
		virtual void SetMultiSample(int num);
		virtual void SetDirty();
		void SetLightDirty(int type, int index);
		void SetDirectionalLight(int index,bool enabled, XMFLOAT4 pos, XMFLOAT4 dir, XMFLOAT4 color);

		void CalculateSplits(float nearPlane, float farPlane);
		void SetCascade(int numCascades);
		void DrawDirectionalShadow();

		void DrawFrustum(Frustum & f, XMMATRIX projv, XMMATRIX view, XMMATRIX camViewProj, int n);

		void ComputeFrustumFromProjection(Frustum* pOut, XMMATRIX* pProjection);
		void CreateFrustumPointsFromCascadeInterval(float fnear, float ffar, XMMATRIX &vProjection, Frustum& outFrustum, XMVECTOR* pvCornerPointsWorld);
		void CalculateFrustumPoints(Camera* cam, Frustum* outFrustum);

		std::vector<float> m_SplitFar;
		std::vector<float> m_SplitNear;
		Material* m_DirectionalShadowMaterial = nullptr;
		Material* m_DirectionalShadowMultiSampleMaterial = nullptr;
		Material* m_ShadowMaterial = nullptr;
		Material* m_CascadedLightingMaterial = nullptr;
		Material* m_CascadedLightingMultiSampleMaterial = nullptr;
		Material* m_ShadowClearMaterial = nullptr;
		ShadowAtlas* m_DirShadowAtlas = nullptr;
		ShadowAtlas* m_MiscShadowAtlas = nullptr;
		LightShadowMap m_Lights[NUM_LIGHTS * 3];

		int m_NumCascades = 0;

		RenderTexture* m_MiscShadows = nullptr;
		RenderTexture* m_CascadeShadows[MAX_CASCADES];
		Frustum m_Frustums[NUM_LIGHTS][MAX_CASCADES];
		int m_Multisample = 1;
		bool m_DirtyLights = true;

		Camera* m_ShadowCamera = nullptr;

		CascadeLightBuffer m_CascadeLightBufferData;
		ID3D11Buffer* m_LightBuffer = nullptr;
		static float CascadeBounds[MAX_CASCADES];
		static float CascadeNearPlane;
		static float CascadeFarPlane;
	};
};