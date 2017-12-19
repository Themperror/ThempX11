#include "ThempSystem.h"
#include "ThempD3D.h"
#include "ThempObject3D.h"
#include "ThempGame.h"
#include "ThempResources.h"
#include "ThempMaterial.h"
#include "ThempMesh.h"
#include "ThempRenderTexture.h"
#include "ThempShadowMap.h"
#include "../Game/ThempCamera.h"
#include <iostream>
#include <fstream>


namespace Themp
{
	D3D11_INPUT_ELEMENT_DESC D3D::DefaultInputLayoutDesc[] =
	{
		{ "POSITION" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	uint32_t D3D::DefaultInputLayoutNumElements = 5;
	Material* D3D::DefaultMaterial;
	Material* D3D::DefaultMaterialShadow;
	Material* D3D::DefaultMaterialPresent;
	ID3D11SamplerState* D3D::DefaultTextureSampler;

	//0 object
	//1 camera
	//2 system
	ID3D11Buffer* D3D::ConstantBuffers[5];
	Object3D* m_FullScreenQuad = nullptr;
	bool D3D::Init()
	{
		//create swap chain
		DXGI_SWAP_CHAIN_DESC scd;

		ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));


		scd.BufferCount = 2;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		

		scd.Windowed = !static_cast<UINT>(Themp::System::tSys->m_SVars.find("Fullscreen")->second);
		if (scd.Windowed)
		{
			scd.BufferDesc.Width = static_cast<UINT>(Themp::System::tSys->m_SVars.find("WindowSizeX")->second);
			scd.BufferDesc.Height = static_cast<UINT>(Themp::System::tSys->m_SVars.find("WindowSizeY")->second);
			scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		}
		else
		{
			scd.BufferDesc.Width = GetSystemMetrics(SM_CXSCREEN);
			scd.BufferDesc.Height = GetSystemMetrics(SM_CYSCREEN);
			scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		}

		scd.OutputWindow = Themp::System::tSys->m_Window;
		scd.SampleDesc.Count = 1;
		scd.SampleDesc.Quality = 0;
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
		};
		HRESULT result;
#ifdef _DEBUG
		result = D3D11CreateDeviceAndSwapChain(NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL, D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT , featureLevels, 3,
			D3D11_SDK_VERSION,
			&scd,
			&m_Swapchain,
			&m_Device, NULL,
			&m_DevCon);
#else 
		result = D3D11CreateDeviceAndSwapChain(NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, 3,
			D3D11_SDK_VERSION,
			&scd,
			&m_Swapchain,
			&m_Device, NULL,
			&m_DevCon);
#endif 

		
		if (result != S_OK) { std::cout << "Could not create D3D11 Device and/or swapchain." << std::endl; return false; }
		int fl = m_Device->GetFeatureLevel();
		std::cout << "FeatureLevel:" << (fl == D3D_FEATURE_LEVEL_11_1 ? "11_1" : fl == D3D_FEATURE_LEVEL_11_0 ? "11_0" : "10_1") << std::endl;

#ifdef _DEBUG	
		result = m_Device->QueryInterface(&m_DebugInterface);
		result = m_DebugInterface->QueryInterface(&m_D3dInfoQueue);
		if (SUCCEEDED(result))
		{
			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_DEVICE_DRAW_SAMPLER_NOT_SET,
				// TODO: Add more message IDs here as needed 
			};
			D3D11_INFO_QUEUE_FILTER filter;
			memset(&filter, 0, sizeof(filter));
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			m_D3dInfoQueue->AddStorageFilterEntries(&filter);
		}
#endif

		int windowWidth = Themp::System::tSys->m_SVars.find("WindowSizeX")->second;
		int windowHeight = Themp::System::tSys->m_SVars.find("WindowSizeY")->second;


		if(!CreateRenderTextures(windowWidth, windowHeight) || !CreateBackBuffer(windowWidth, windowHeight) || !CreateDepthStencil(windowWidth, windowHeight))
		{
			printf("Could not initialise all required resources, shutting down");
			return false;
		}
		

		SetViewPort(0, 0, windowWidth, windowHeight);

		//create default material's and other data
		D3D11_SAMPLER_DESC texSamplerDesc;
		texSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		texSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		texSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		texSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		texSamplerDesc.MipLODBias = 0.0f;
		texSamplerDesc.MaxAnisotropy = static_cast<UINT>(Themp::System::tSys->m_SVars.find("Anisotropic_Filtering")->second);
		texSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		texSamplerDesc.BorderColor[0] = 0;
		texSamplerDesc.BorderColor[1] = 0;
		texSamplerDesc.BorderColor[2] = 1;
		texSamplerDesc.BorderColor[3] = 0;
		texSamplerDesc.MinLOD = 0;
		texSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		result = m_Device->CreateSamplerState(&texSamplerDesc, &D3D::DefaultTextureSampler);

		D3D11_RASTERIZER_DESC rDesc;
		memset(&rDesc, 0, sizeof(D3D11_RASTERIZER_DESC));
		rDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID; //change for Wireframe
		rDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK; //Backface culling yes/no/inverted
		rDesc.DepthClipEnable = true; //default true

		m_Device->CreateRasterizerState(&rDesc, &m_RasterizerState);
		
		m_DevCon->RSSetState(m_RasterizerState);
		std::vector<std::string> defaultTextures = {
			"DefaultDiffuse.dds",
			"",
			"",
			"",
		};
		std::vector<uint8_t> defaultTypes = { 1,((uint8_t)(-1)),((uint8_t)(-1)),((uint8_t)(-1)) };
		D3D::DefaultMaterial = Resources::TRes->LoadMaterial(defaultTextures, defaultTypes, "Deferred", true, true, false);

		defaultTypes[1] = 5;
		defaultTypes[2] = 7;
		defaultTypes[3] = 8;
		defaultTextures[1] = "DefaultNormal.dds";
		defaultTextures[2] = "DefaultSpecular.dds";
		defaultTextures[3] = "DefaultMisc.dds";
		D3D::DefaultMaterialShadow = Resources::TRes->LoadMaterial(defaultTextures, defaultTypes, "DeferredShadow", true, true, true);
		D3D::DefaultMaterialPresent = Resources::TRes->LoadMaterial(defaultTextures, defaultTypes, "DeferredPresent", true, true, false);

		std::cout << "DirectX11 Initialisation success!" << std::endl;

	
		

		m_FullScreenQuad = new Object3D();
		m_FullScreenQuad->CreateQuad("ScreenSpace", true, true, false);
		m_ShadowCamera = new Camera();
		m_ShadowCamera->SetFoV(90);
		m_ShadowCamera->SetAspectRatio(1.0);

		m_ShadowMap = new ShadowMap(8192);
		m_DevCon->ClearDepthStencilView(m_ShadowMap->m_DepthStencilView, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		memset(&m_LightConstantBufferData, 0, sizeof(LightConstantBuffer));

		///////////////Set up a Directional Light
		{
			m_LightConstantBufferData.numDir = 1;
			DirectionalLight* l = &m_LightConstantBufferData.dirLights[0];
			l->textureOffset = m_ShadowMap->ObtainTextureArea(4096);
			shadowMaps[0].l = l;
			shadowMaps[0].tex = new RenderTexture(4096, 4096, RenderTexture::DepthTex);
			shadowMaps[0].LightIsDirty = true;
			//sponza light dir
			//XMFLOAT3 dir = XMFLOAT3(0.621556, -0.783368, 0.001628);
			//elemental light dir
			XMFLOAT3 dir = XMFLOAT3(-0.343871, -0.762444, 0.548116);
			XMVECTOR xmVec = XMLoadFloat3(&dir);
			xmVec = XMVector3Normalize(xmVec);
			XMStoreFloat3(&dir, xmVec);
			l->position.x = 4.258440;
			l->position.y = 42.606735;
			l->position.z = -98.248665;
			l->direction = XMFLOAT4(dir.x, dir.y, dir.z, 1.0);
			l->color = XMFLOAT4(0.5, 0.5, 0.5, 1);
			RenderShadowsDirectionalLight(l);
			l->lightviewmatrix = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
			l->lightprojmatrix = m_ShadowCamera->m_CameraConstantBufferData.projectionMatrix;

		}
		///////////////

		///////////////Set up Point Light
		{
			m_LightConstantBufferData.numPoint = 1;
			PointLight* l = &m_LightConstantBufferData.pointLights[0];
			shadowMaps[1].l = l;
			shadowMaps[1].tex = new RenderTexture(1024, 1024, RenderTexture::CubeDepthTex);
			shadowMaps[1].LightIsDirty = true;
			for (size_t i = 0; i < 6; i++)
			{
				l->textureOffset[i] = m_ShadowMap->ObtainTextureArea(1024);
			}
			l->position.x = 0.649892;
			l->position.y = 5.834146;
			l->position.z = -39.419930;
			l->color = XMFLOAT4(0.5,0.5,0.5, 40);
			RenderShadowsPointLight(l);
			l->lightprojmatrix = m_ShadowCamera->m_CameraConstantBufferData.projectionMatrix;
		}
		///////////////

		D3D11_BUFFER_DESC cbDesc;
		cbDesc.ByteWidth = sizeof(LightConstantBuffer);
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = &m_LightConstantBufferData;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the buffer.
		m_Device->CreateBuffer(&cbDesc, &InitData, &m_LightBuffer);

		SetLightConstantBuffer(m_LightBuffer);
		return true;
	}

	void D3D::ResizeWindow(int newX, int newY)
	{
		int windowWidth = Themp::System::tSys->m_SVars.find("WindowSizeX")->second;
		int windowHeight = Themp::System::tSys->m_SVars.find("WindowSizeY")->second;
		if (newX == windowWidth && newY == windowHeight)
		{
			return;
		}
		if (m_Swapchain)
		{
			m_DevCon->OMSetRenderTargets(0, 0, 0);

			// Release all outstanding references to the swap chain's buffers.
			CLEAN(m_BackBuffer);
			for (int i = 0; i < NUM_RENDER_TEXTURES; i++)
			{
				CLEAN(m_RenderTextures[i]->m_RenderTarget);
				CLEAN(m_RenderTextures[i]->m_ShaderResourceView);
			}

			//Recreate back buffer
			HRESULT hr;
			// Preserve the existing buffer count and format.
			// Automatically choose the width and height to match the client rect for HWNDs.
			hr = m_Swapchain->ResizeBuffers(0, newX, newY, DXGI_FORMAT_UNKNOWN, 0);
			if (hr != S_OK)
			{
				printf("failed to resize buffers");
			}

			// Get buffer and create a render-target-view.
			ID3D11Texture2D* pBuffer;
			hr = m_Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
				(void**)&pBuffer);
			if (hr != S_OK)
			{
				printf("failed to obtain backbuffer texture");
			}

			hr = m_Device->CreateRenderTargetView(pBuffer, NULL, &m_BackBuffer);
			if (hr != S_OK)
			{
				printf("failed to create rendertarget view from backbuffer");
			}
			pBuffer->Release();

			for (int i = 0; i < NUM_RENDER_TEXTURES; i++)
			{
				ID3D11Texture2D* renderTex;
				D3D11_TEXTURE2D_DESC renderTexDesc;
				memset(&renderTexDesc, 0, sizeof(D3D11_TEXTURE2D_DESC));
				renderTexDesc.Usage = D3D11_USAGE_DEFAULT;
				renderTexDesc.ArraySize = 1;
				renderTexDesc.MipLevels = 1;
				renderTexDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
				renderTexDesc.CPUAccessFlags = 0;
				renderTexDesc.SampleDesc.Count = 1;
				renderTexDesc.SampleDesc.Quality = 0;
				renderTexDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
				renderTexDesc.Height = newY;
				renderTexDesc.Width = newX;
			
				hr = m_Device->CreateTexture2D(&renderTexDesc, nullptr, &renderTex);
			
				if (hr != S_OK) { printf("Could not CreateTexture2D %i", i); return; }
				// use the back buffer address to create the render target
				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
				memset(&srvDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
				srvDesc.Format = renderTexDesc.Format;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
			
				hr = m_Device->CreateShaderResourceView(renderTex, &srvDesc, &m_RenderTextures[i]->m_ShaderResourceView);
				if (hr != S_OK) { printf("Could not CreateShaderResourceView %i", i); return; }
			
				m_ShaderResourceViews[i] = m_RenderTextures[i]->m_ShaderResourceView;
			
				hr = m_Device->CreateRenderTargetView(renderTex, nullptr, &m_RenderTextures[i]->m_RenderTarget);
				if (hr != S_OK) { printf("Could not CreateRenderTargetView %i", i); return; }
			
				renderTex->Release();
				m_Rtvs[i] = m_RenderTextures[i]->m_RenderTarget;
				m_ShaderResourceViews[i] = m_RenderTextures[i]->m_ShaderResourceView;
			}

			CLEAN(m_DepthStencil);
			CLEAN(m_DepthStencilSRV);
			CLEAN(m_DepthStencilView);

			D3D11_TEXTURE2D_DESC depthBufferDesc;
			ZeroMemory(&depthBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));
			depthBufferDesc.SampleDesc.Count = 1;
			depthBufferDesc.SampleDesc.Quality = 0;

			depthBufferDesc.Width = newX;
			depthBufferDesc.Height = newY;
			depthBufferDesc.MipLevels = 1;
			depthBufferDesc.ArraySize = 1;
			depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // DXGI_FORMAT_D16_UNORM;
			depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			depthBufferDesc.CPUAccessFlags = 0;
			depthBufferDesc.MiscFlags = 0;
			hr = m_Device->CreateTexture2D(&depthBufferDesc, NULL, &m_DepthStencil);
			if (hr != S_OK)
			{
				printf("failed to create depth texture");
			}

			D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
			memset(&descDSV, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
			descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			descDSV.Texture2D.MipSlice = 0;


			// Create the depth stencil view
			hr = m_Device->CreateDepthStencilView(m_DepthStencil, // Depth stencil texture
				&descDSV, // Depth stencil desc
				&m_DepthStencilView);  // [out] Depth stencil view
			if (hr != S_OK)
			{
				printf("Could not create Depthstencil view");
				return;
			}
			D3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc;
			memset(&depthSRVDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			depthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			depthSRVDesc.Texture2D.MipLevels = -1;
			depthSRVDesc.Texture2D.MostDetailedMip = 0;
			depthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

			hr = m_Device->CreateShaderResourceView(m_DepthStencil, // Depth stencil texture
				&depthSRVDesc, // Depth stencil desc
				&m_DepthStencilSRV);  // [out] Depth stencil view
			if (hr != S_OK)
			{
				printf("Could not create Depthstencil shader resource view");
				return;
			}


			//m_DevCon->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

			// Set up the viewport.
			D3D11_VIEWPORT vp;
			vp.Width = newX;
			vp.Height = newY;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			m_DevCon->RSSetViewports(1, &vp);
			Themp::System::tSys->m_SVars["WindowSizeX"] = newX;
			Themp::System::tSys->m_SVars["WindowSizeY"] = newY;
			m_ConstantBufferData.screenHeight = windowHeight;
			m_ConstantBufferData.screenWidth = windowWidth;
			dirtySystemBuffer = true;

		}
	}
	void D3D::RenderShadowsPointLight(PointLight* l)
	{
		m_ShadowCamera->SetupCamera(XMFLOAT3(l->position.x, l->position.y, l->position.z), XMFLOAT3(1,0,0), XMFLOAT3(0, 1, 0));
		m_ShadowCamera->m_CamType = Camera::CameraType::Perspective;
		m_ShadowCamera->isDirty = true;

		m_ShadowCamera->m_LookDirection.x = 1;
		m_ShadowCamera->UpdateMatrices();
		m_ShadowCamera->isDirty = true;
		l->lightviewmatrix[0] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;


		m_ShadowCamera->m_LookDirection.x = -1;
		m_ShadowCamera->UpdateMatrices();
		m_ShadowCamera->isDirty = true;
		l->lightviewmatrix[1] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;


		m_ShadowCamera->m_LookDirection.x = 0;
		m_ShadowCamera->m_LookDirection.y = 1;
		m_ShadowCamera->SetUpVector(1, 0, 0);
		m_ShadowCamera->UpdateMatrices();
		m_ShadowCamera->isDirty = true;
		l->lightviewmatrix[2] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix; 


		m_ShadowCamera->m_LookDirection.y = -1;
		m_ShadowCamera->UpdateMatrices();
		m_ShadowCamera->isDirty = true;
		l->lightviewmatrix[3] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;


		m_ShadowCamera->SetUpVector(0, 1, 0);
		m_ShadowCamera->m_LookDirection.y = 0;
		m_ShadowCamera->m_LookDirection.z = 1;
		m_ShadowCamera->UpdateMatrices();
		m_ShadowCamera->isDirty = true;
		l->lightviewmatrix[4] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;


		m_ShadowCamera->m_LookDirection.z = -1;
		m_ShadowCamera->UpdateMatrices();
		l->lightviewmatrix[5] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
	}
	void D3D::RenderShadowsSpotLight(SpotLight* l)
	{

	}
	void D3D::RenderShadowsDirectionalLight(DirectionalLight* l)
	{
		m_ShadowCamera->SetupCamera(XMFLOAT3(l->position.x, l->position.y, l->position.z), XMFLOAT3(l->direction.x, l->direction.y, l->direction.z),XMFLOAT3(0,1,0));
		m_ShadowCamera->m_CamType = Camera::CameraType::Orthographic;
		m_ShadowCamera->m_OrthoWidth = 150;
		m_ShadowCamera->m_OrthoHeight = 150;
		m_ShadowCamera->isDirty = true;
		m_ShadowCamera->UpdateMatrices();
	}
	void D3D::DrawGBufferPass(Game & game)
	{
		m_DevCon->PSSetShaderResources(0, 4, D3D::DefaultMaterial->m_Views);
		m_DevCon->OMSetRenderTargets(NUM_RENDER_TEXTURES, m_Rtvs, m_DepthStencilView);

		m_DevCon->PSSetShader(D3D::DefaultMaterial->m_PixelShader, 0, 0);
		m_DevCon->VSSetShader(D3D::DefaultMaterial->m_VertexShader, 0, 0);
		m_DevCon->GSSetShader(D3D::DefaultMaterial->m_GeometryShader, 0, 0);

		m_DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_DevCon->IASetInputLayout(D3D::DefaultMaterial->m_InputLayout);
		Themp::D3D& _this = *static_cast<Themp::D3D*>(this);

		for (int i = 0; i < game.m_Objects3D.size(); ++i)
		{
			game.m_Objects3D[i]->Draw(_this,false);
		}
	}
	Camera* shadowCamera; 
	int frame = 0;
	void D3D::DrawShadowMaps(Themp::Game& game)
	{
		frame++;


		//PointLight* l = &m_LightConstantBufferData.pointLights[0];
		//l->position.x = sin(frame / 20.0f) * 10;
		//shadowMaps[1].LightIsDirty = true;

		m_DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_DevCon->IASetInputLayout(D3D::DefaultMaterial->m_InputLayout);

		//m_DevCon->ClearDepthStencilView(m_ShadowMap->m_DepthStencilView, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		m_DevCon->OMSetRenderTargets(0, nullptr, m_ShadowMap->m_DepthStencilView);
		//ConstantBuffers:
		//0 = SystemBuffer
		//1 = ObjectBuffer
		//2 = LightBuffer

		Themp::D3D& _this = *static_cast<Themp::D3D*>(this);
		D3D::ConstantBuffers[2] = m_LightBuffer;

		for (size_t i = 0; i < m_LightConstantBufferData.numDir; i++)
		{
			LightShadowMap* s = &shadowMaps[i];
			if (s->LightIsDirty)
			{
				//m_DevCon->ClearDepthStencilView(s->tex->m_DepthStencilView, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				//m_DevCon->OMSetRenderTargets(0, nullptr, s->tex->m_DepthStencilView);
				DirectionalLight* l = &m_LightConstantBufferData.dirLights[i];
				SetViewPort(l->textureOffset.x, l->textureOffset.y, s->tex->m_ResolutionX, s->tex->m_ResolutionY);

				m_DevCon->PSSetShader(D3D::DefaultMaterialShadow->m_PixelShader, 0, 0);
				m_DevCon->VSSetShader(D3D::DefaultMaterialShadow->m_VertexShader, 0, 0);
				m_DevCon->GSSetShader(nullptr, 0, 0);

				RenderShadowsDirectionalLight(l);
				l->lightviewmatrix = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
				l->lightprojmatrix = m_ShadowCamera->m_CameraConstantBufferData.projectionMatrix;
				for (int j = 0; j < game.m_Objects3D.size(); ++j)
				{
					game.m_Objects3D[j]->Draw(_this, true);
				}
				s->LightIsDirty = false;
			}
		}
		GSUploadConstantBuffersToGPU();
		for (size_t i = 0; i < m_LightConstantBufferData.numPoint; i++)
		{
			int shadowMapIndex = m_LightConstantBufferData.numDir + i;
			LightShadowMap* s = &shadowMaps[shadowMapIndex];
			if (s->LightIsDirty)
			{
				//m_DevCon->ClearDepthStencilView(s->tex->m_DepthStencilView, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				PointLight* l = &m_LightConstantBufferData.pointLights[i];
				D3D11_VIEWPORT vps[6];
				for (size_t j = 0; j < 6; j++)
				{
					vps[j].TopLeftX = l->textureOffset[j].x;
					vps[j].TopLeftY = l->textureOffset[j].y;
					vps[j].Width = l->textureOffset[j].z;
					vps[j].Height = l->textureOffset[j].w;
					vps[j].MinDepth = 0.0f;
					vps[j].MaxDepth = 1.0f;
				}
				m_DevCon->RSSetViewports(6, &vps[0]);
				m_DevCon->PSSetShader(D3D::DefaultMaterialShadow->m_PixelShader, 0, 0);
				m_DevCon->VSSetShader(D3D::DefaultMaterialShadow->m_VertexShader, 0, 0);
				m_DevCon->GSSetShader(D3D::DefaultMaterialShadow->m_GeometryShader, 0, 0);
				RenderShadowsPointLight(l);

				for (int j = 0; j < game.m_Objects3D.size(); ++j)
				{
					game.m_Objects3D[j]->Draw(_this, true);
				}
				s->LightIsDirty = false;
			}
		}
		GSUploadConstantBuffersToGPUNull();
		for (size_t i = 0; i < m_LightConstantBufferData.numSpot; i++)
		{
			int shadowMapIndex = m_LightConstantBufferData.numPoint + m_LightConstantBufferData.numDir + i;
			LightShadowMap* s = &shadowMaps[shadowMapIndex];
			if (s->LightIsDirty)
			{
				m_DevCon->PSSetShader(D3D::DefaultMaterialShadow->m_PixelShader, 0, 0);
				m_DevCon->VSSetShader(D3D::DefaultMaterialShadow->m_VertexShader, 0, 0);
				m_DevCon->GSSetShader(nullptr, 0, 0);
				SpotLight* l = &m_LightConstantBufferData.spotLights[i];
				RenderShadowsSpotLight(l);

				for (int j = 0; j < game.m_Objects3D.size(); ++j)
				{
					game.m_Objects3D[j]->Draw(_this, true);
				}
				s->LightIsDirty = false;
			}
		}
	}
	void D3D::DrawLightPass()
	{
		m_DevCon->PSSetShader(D3D::DefaultMaterialPresent->m_PixelShader, 0, 0);
		m_DevCon->VSSetShader(D3D::DefaultMaterialPresent->m_VertexShader, 0, 0);
		m_DevCon->GSSetShader(D3D::DefaultMaterialPresent->m_GeometryShader, 0, 0);

		SetViewPort(0, 0, m_ConstantBufferData.screenWidth, m_ConstantBufferData.screenHeight);
		//m_DevCon->OMSetRenderTargets(1, &m_BackBuffer, m_DepthStencilView);
		m_DevCon->OMSetRenderTargets(1, &m_BackBuffer, nullptr);
		m_DevCon->PSSetSamplers(0, 4, D3D::DefaultMaterialPresent->m_SamplerStates);

		for (size_t i = 0; i < 5; i++)
		{
			m_ShaderResourceViews[i] = m_RenderTextures[i]->m_ShaderResourceView;
		}
		//m_ShaderResourceViews[4] = m_RenderTextures[4]->m_ShaderResourceView;
		m_ShaderResourceViews[5] = m_DepthStencilSRV;
		m_ShaderResourceViews[6] = m_ShadowMap->m_ShaderResourceView;
		//m_ShaderResourceViews[6] = shadowMaps[0].tex->m_ShaderResourceView;

		m_DevCon->PSSetShaderResources(0, NUM_SHADER_RESOURCE_VIEWS, m_ShaderResourceViews);


		// draw fullscreen quad, manually
		uint32_t stride[] = { sizeof(Vertex) };
		uint32_t offset[] = { 0 };
		m_DevCon->IASetVertexBuffers(0, 1, &m_FullScreenQuad->m_Meshes[0]->m_VertexBuffer, stride, offset);
		m_DevCon->IASetIndexBuffer(m_FullScreenQuad->m_Meshes[0]->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		m_DevCon->IASetInputLayout(D3D::DefaultMaterialPresent->m_InputLayout);

		m_DevCon->PSSetSamplers(0, 4, D3D::DefaultMaterialPresent->m_SamplerStates);
		SetSystemConstantBuffer(m_CBuffer); 
		Themp::System::tSys->m_Game->m_Camera->UpdateMatrices();
		SetLightConstantBuffer(m_LightBuffer);
		PSUploadConstantBuffersToGPU();

		m_DevCon->DrawIndexed(m_FullScreenQuad->m_Meshes[0]->numIndices, 0, 0);
	}
	void D3D::Draw(Themp::Game& game)
	{
		static const float ClearColor[4] = { 0.0f, 0.2f, 0.4f, 0.0f };
		for (size_t i = 0; i < NUM_RENDER_TEXTURES; i++)
		{
			m_DevCon->ClearRenderTargetView(m_RenderTextures[i]->m_RenderTarget, ClearColor);
		}
		m_DevCon->ClearRenderTargetView(m_BackBuffer, ClearColor);
		m_DevCon->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		//Draw all geometry to the GBuffer //positions, normals, etc
		DrawGBufferPass(game);

		DrawShadowMaps(game);

		//Draw everything to a fullscreen quad, while calculating lighting 
		DrawLightPass();
		//Present to screen
		m_Swapchain->Present(0, 0);

		//set stuff back to prevent issues next draw
		VSUploadConstantBuffersToGPUNull();
		PSUploadConstantBuffersToGPUNull();
		GSUploadConstantBuffersToGPUNull();
		for (size_t i = 0; i < NUM_SHADER_RESOURCE_VIEWS; i++)
		{
			m_ShaderResourceViews[i] = nullptr;
		}
		m_DevCon->PSSetShaderResources(0, NUM_SHADER_RESOURCE_VIEWS, m_ShaderResourceViews);

		D3D11_MAPPED_SUBRESOURCE ms;
		m_DevCon->Map(m_LightBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
		memcpy(ms.pData, &m_LightConstantBufferData, sizeof(LightConstantBuffer));
		m_DevCon->Unmap(m_LightBuffer, NULL);
	}
	
	D3D::~D3D()
	{
		if (Themp::System::tSys->m_SVars.find("Fullscreen")->second == 1)
		{
			m_Swapchain->SetFullscreenState(FALSE, NULL);  // switch to windowed mode
		}
		CLEAN(m_CBuffer);
		CLEAN(m_LightBuffer);
		CLEAN(D3D::ConstantBuffers[0]);
		CLEAN(D3D::ConstantBuffers[1]);
		CLEAN(D3D::ConstantBuffers[2]);
		CLEAN(D3D::DefaultTextureSampler);

		for (size_t i = 0; i < 5; i++)
		{
			delete m_RenderTextures[i];
		}
		for (size_t i = 0; i < NUM_LIGHTS; i++)
		{
			delete shadowMaps[i].tex;
		}

		VSUploadConstantBuffersToGPUNull();
		PSUploadConstantBuffersToGPUNull();
		GSUploadConstantBuffersToGPUNull();

		delete m_ShadowCamera;
		delete m_ShadowMap;

		CLEAN(m_OMBlendState);
		CLEAN(m_DepthStencil);
		CLEAN(m_DepthStencilView);
		CLEAN(m_DeptStencilState);
		CLEAN(m_DepthStencilSRV);
		CLEAN(m_RasterizerState);
		CLEAN(m_InputLayout);
		CLEAN(m_Swapchain);
		CLEAN(m_BackBuffer);
		CLEAN(m_Device);
		CLEAN(m_DevCon);

#ifdef _DEBUG

		CLEAN(m_D3dInfoQueue);

#if ReportLiveObjects
		if (m_DebugInterface)
		{
			m_DebugInterface->ReportLiveDeviceObjects(D3D11_RLDO_FLAGS::D3D11_RLDO_DETAIL);
		}
#endif
		CLEAN(m_DebugInterface);
#endif
	}

	void D3D::SetViewPort(float xPos, float yPos, float width, float height)
	{
		D3D11_VIEWPORT viewport;
		ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

		viewport.TopLeftX = xPos;
		viewport.TopLeftY = yPos;
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1.0f;

		m_DevCon->RSSetViewports(1, &viewport);
	}
	void D3D::SetObject3DConstantBuffer(ID3D11Buffer* buf)
	{
		D3D::ConstantBuffers[0] = buf;
	}
	void D3D::SetCameraConstantBuffer(ID3D11Buffer* buf)
	{
		D3D::ConstantBuffers[1] = buf;
	}
	void D3D::SetSystemConstantBuffer(ID3D11Buffer* buf)
	{
		D3D::ConstantBuffers[2] = buf;
	}
	void D3D::SetMaterialConstantBuffer(ID3D11Buffer* buf)
	{
		D3D::ConstantBuffers[3] = buf;
	}
	void D3D::SetLightConstantBuffer(ID3D11Buffer* buf)
	{
		D3D::ConstantBuffers[4] = buf;
	}
	void D3D::VSUploadConstantBuffersToGPU()
	{
		if (D3D::ConstantBuffers[1] == nullptr) std::cout << "No Camera active in scene" << std::endl;
		m_DevCon->VSSetConstantBuffers(0, 2, D3D::ConstantBuffers);
	}
	void D3D::VSUploadConstantBuffersToGPUNull()
	{
		D3D::ConstantBuffers[0] = nullptr;
		D3D::ConstantBuffers[1] = nullptr;
		D3D::ConstantBuffers[2] = nullptr;
		D3D::ConstantBuffers[3] = nullptr;
		D3D::ConstantBuffers[4] = nullptr;
		m_DevCon->VSSetConstantBuffers(0, 5, D3D::ConstantBuffers);
	}
	void D3D::PSUploadConstantBuffersToGPUNull()
	{
		ID3D11Buffer* buffers[5] = { nullptr,nullptr,nullptr,nullptr,nullptr };
		m_DevCon->PSSetConstantBuffers(0, 5, buffers);
	}
	void D3D::GSUploadConstantBuffersToGPUNull()
	{
		ID3D11Buffer* buffers[5] = { nullptr,nullptr,nullptr,nullptr,nullptr };
		m_DevCon->GSSetConstantBuffers(0, 5, buffers);
	}
	void D3D::PSUploadConstantBuffersToGPU()
	{
		m_DevCon->PSSetConstantBuffers(0, 5, D3D::ConstantBuffers);
	}
	void D3D::GSUploadConstantBuffersToGPU()
	{
		m_DevCon->GSSetConstantBuffers(0, 5, D3D::ConstantBuffers);
	}
	bool D3D::CreateBackBuffer(int width, int height)
	{
		HRESULT result;
		////get back buffer
		ID3D11Texture2D *pBackBuffer;
		result = m_Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		if (result != S_OK) { std::cout << "Could not obtain BackBuffer" << std::endl; return false; }
		// use the back buffer address to create the render target
		result = m_Device->CreateRenderTargetView(pBackBuffer, NULL, &m_BackBuffer);
		if (result != S_OK) { std::cout << "Could not create Render target (BackBuffer)" << std::endl; return false; }
		pBackBuffer->Release();
		return true;
	}
	bool D3D::CreateRenderTextures(int width, int height)
	{
		HRESULT result;
		for (int i = 0; i < NUM_RENDER_TEXTURES; i++)
		{
			m_RenderTextures[i] = new RenderTexture(width,height,RenderTexture::RenderTex);
			m_ShaderResourceViews[i] = m_RenderTextures[i]->m_ShaderResourceView;
			m_Rtvs[i] = m_RenderTextures[i]->m_RenderTarget;
		}
		return true;
	}
	bool D3D::CreateDepthStencil(int width, int height)
	{
		HRESULT result;

		D3D11_TEXTURE2D_DESC depthBufferDesc;
		ZeroMemory(&depthBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));
		depthBufferDesc.SampleDesc.Count = 1;
		depthBufferDesc.SampleDesc.Quality = 0;

		depthBufferDesc.Width = width;
		depthBufferDesc.Height = height;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.ArraySize = 1;
		depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // DXGI_FORMAT_D16_UNORM;
		depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		depthBufferDesc.CPUAccessFlags = 0;
		depthBufferDesc.MiscFlags = 0;
		result = m_Device->CreateTexture2D(&depthBufferDesc, NULL, &m_DepthStencil);
		if (result != S_OK)
		{
			printf("Could not create Depthstencil texture");
			return false;
		}
		D3D11_DEPTH_STENCIL_DESC dsDesc;

		// Depth test parameters
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

		// Stencil test parameters
		dsDesc.StencilEnable = true;
		dsDesc.StencilReadMask = 0xFF;
		dsDesc.StencilWriteMask = 0xFF;

		// Stencil operations if pixel is front-facing
		dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
		dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Stencil operations if pixel is back-facing
		dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		// Create depth stencil state
		result = m_Device->CreateDepthStencilState(&dsDesc, &m_DeptStencilState);
		if (result != S_OK)
		{
			printf("Could not create Depthstencil state");
			return false;
		}
		m_DevCon->OMSetDepthStencilState(m_DeptStencilState, 1);

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		memset(&descDSV, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
		descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		descDSV.Texture2D.MipSlice = 0;


		// Create the depth stencil view
		result = m_Device->CreateDepthStencilView(m_DepthStencil, // Depth stencil texture
			&descDSV, // Depth stencil desc
			&m_DepthStencilView);  // [out] Depth stencil view
		if (result != S_OK)
		{
			printf("Could not create Depthstencil view");
			return false;
		}
		D3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc;
		memset(&depthSRVDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		depthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		depthSRVDesc.Texture2D.MipLevels = -1;
		depthSRVDesc.Texture2D.MostDetailedMip = 0;
		depthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

		result = m_Device->CreateShaderResourceView(m_DepthStencil, // Depth stencil texture
			&depthSRVDesc, // Depth stencil desc
			&m_DepthStencilSRV);  // [out] Depth stencil view
		if (result != S_OK)
		{
			printf("Could not create Depthstencil shader resource view");
			return false;
		}

		return true;
	}
	void D3D::PrepareSystemBuffer(Themp::Game& game)
	{
		SetSystemConstantBuffer(m_CBuffer);
		if (!dirtySystemBuffer)return;

		// Supply the vertex shader constant data.
		m_ConstantBufferData.screenWidth = Themp::System::tSys->m_SVars["WindowSizeX"];
		m_ConstantBufferData.screenHeight = Themp::System::tSys->m_SVars["WindowSizeY"];
		if (!m_CBuffer)
		{
			// Fill in a buffer description.
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.ByteWidth = sizeof(CONSTANT_BUFFER);
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = 0;

			// Fill in the subresource data.
			D3D11_SUBRESOURCE_DATA InitData;
			InitData.pSysMem = &m_ConstantBufferData;
			InitData.SysMemPitch = 0;
			InitData.SysMemSlicePitch = 0;

			// Create the buffer.
			m_Device->CreateBuffer(&cbDesc, &InitData, &m_CBuffer);
		}
		else
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			m_DevCon->Map(m_CBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
			memcpy(ms.pData, &m_ConstantBufferData, sizeof(CONSTANT_BUFFER));
			m_DevCon->Unmap(m_CBuffer, NULL);
		}
		dirtySystemBuffer = false;
		SetSystemConstantBuffer(m_CBuffer);
	}
};