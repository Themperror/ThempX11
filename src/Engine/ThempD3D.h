#pragma once

#include <d3d11.h>
#include <d3d10.h>
#include <DirectXMath.h>
#include "ThempMaterial.h"


#define NUM_RENDER_TEXTURES 5

//the +2 is for the depth texture and shadow map
#define NUM_SHADER_RESOURCE_VIEWS NUM_RENDER_TEXTURES+2


#define NUM_LIGHTS 3
namespace Themp
{ 
	using namespace DirectX;

	struct CONSTANT_BUFFER
	{
		float screenWidth;
		float screenHeight;
		float visualType;
		float dummy2;
	};
	struct Light
	{
		XMFLOAT4 position;
		XMFLOAT4 color;
		XMFLOAT4X4 lightprojmatrix;
	};
	struct DirectionalLight : public Light
	{
		XMFLOAT4 direction;
		//float range, type, dummyRZ,dummyRW; //64
		XMFLOAT4X4 lightviewmatrix; //128
		XMFLOAT4 textureOffset;//x offset, y offset, width, height;
	};
	struct SpotLight : public Light
	{
		XMFLOAT4 direction;
		float angleMin, angleMax, range,dummyRW; //64
		XMFLOAT4X4 lightviewmatrix; //128
		XMFLOAT4 textureOffset;//x offset, y offset, width, height;
	};
	struct PointLight : public Light
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
	class RenderTexture;
	class Camera;

	struct LightShadowMap
	{
		Light* l;
		bool LightIsDirty;
		RenderTexture* tex;
	};
	class Game;
	class ShadowMap;
	class D3D
	{
		
	public:
		D3D() {};
		~D3D();
		bool Init();
		void ResizeWindow(int newX, int newY);
		void RenderShadowsPointLight(PointLight * l);
		void RenderShadowsSpotLight(SpotLight * l);
		void RenderShadowsDirectionalLight(DirectionalLight * l);
		void PrepareSystemBuffer(Game & game);
		void Draw(Game& game);
		void DrawGBufferPass(Game& game);
		void DrawShadowMaps(Game& game);
		void DrawLightPass();

		void SetViewPort(float xPos, float yPos, float width, float height);
		void SetViewPorts(int* nViewports, float* xPos, float* yPos, float* width, float* height);
		void SetObject3DConstantBuffer(ID3D11Buffer* buf);
		void SetCameraConstantBuffer(ID3D11Buffer* buf);
		void SetSystemConstantBuffer(ID3D11Buffer* buf);
		void SetMaterialConstantBuffer(ID3D11Buffer* buf);
		void SetLightConstantBuffer(ID3D11Buffer* buf);
		void VSUploadConstantBuffersToGPU();
		void VSUploadConstantBuffersToGPUNull();
		void PSUploadConstantBuffersToGPUNull();
		void GSUploadConstantBuffersToGPUNull();
		void PSUploadConstantBuffersToGPU();
		void GSUploadConstantBuffersToGPU();

		bool CreateBackBuffer(int width, int height);
		bool CreateRenderTextures(int width, int height);
		bool CreateDepthStencil(int width, int height);

		ID3D10Blob * ReadToBlob(std::string path);

		CONSTANT_BUFFER m_ConstantBufferData;
		LightShadowMap shadowMaps[NUM_LIGHTS];
		LightConstantBuffer m_LightConstantBufferData;
		RenderTexture* m_RenderTextures[NUM_RENDER_TEXTURES];
		ID3D11RenderTargetView* m_Rtvs[NUM_RENDER_TEXTURES];
		ID3D11ShaderResourceView* m_ShaderResourceViews[NUM_SHADER_RESOURCE_VIEWS]; 

		ID3D11RenderTargetView* m_BackBuffer = nullptr;
		ID3D11BlendState* m_OMBlendState = nullptr;
		ID3D11Texture2D* m_DepthStencil = nullptr;
		ID3D11DepthStencilState * m_DeptStencilState = nullptr;
		ID3D11DepthStencilView* m_DepthStencilView = nullptr;
		ID3D11ShaderResourceView* m_DepthStencilSRV = nullptr;

		ID3D11RasterizerState* m_RasterizerState = nullptr;
		ID3D11Buffer* m_CBuffer = nullptr;      //System Constant Buffer
		ID3D11Buffer* m_LightBuffer = nullptr;      //Light Constant Buffer
		ID3D11Device* m_Device = nullptr;
#ifdef _DEBUG
		ID3D11Debug* m_DebugInterface;
		ID3D11InfoQueue* m_D3dInfoQueue;
#endif
		IDXGISwapChain* m_Swapchain = nullptr;             // the pointer to the swap chain interface
		ID3D11DeviceContext* m_DevCon = nullptr;           // the pointer to our Direct3D device context
		ID3D11InputLayout* m_InputLayout = nullptr;

		Camera* m_ShadowCamera;
		ShadowMap* m_ShadowMap;

		bool dirtySystemBuffer = true;
		static D3D11_INPUT_ELEMENT_DESC DefaultInputLayoutDesc[];
		static uint32_t DefaultInputLayoutNumElements;
		static Material* DefaultMaterial;
		static Material* DefaultMaterialShadow;
		static Material* DefaultMaterialPresent;
		static ID3D11SamplerState* DefaultTextureSampler;
		static ID3D11Buffer* ConstantBuffers[5];
	};
};