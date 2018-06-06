#include "ThempSystem.h"
#include "ThempD3D.h"
#include "ThempObject3D.h"
#include "ThempGame.h"
#include "ThempResources.h"
#include "ThempMaterial.h"
#include "ThempMesh.h"
#include "ThempRenderTexture.h"
#include "ThempShadowAtlas.h"
#include "ThempGUI.h"
#include "ThempCamera.h"
#include "ThempDebugDraw.h"
#include "Shadowing\ThempShadowUnfiltered.h"
#include "Shadowing\ThempShadowPCF.h"
#include "Shadowing\ThempShadowCascade.h"
#include "Shadowing\ThempShadowVariance.h"
#include "Shadowing\ThempShadowMoment.h"
#include <imgui.h>
#include <iostream>
#include <fstream>

using namespace DirectX;

namespace Themp
{
	XMFLOAT3 Normalize(const XMFLOAT3& v)
	{
		XMVECTOR x = XMLoadFloat3(&v);
		x = XMVector3Normalize(x);
		XMFLOAT3 r;
		XMStoreFloat3(&r, x);
		return r;
	}
	XMFLOAT3 XMFLOAT3Add(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		return XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z);
	}
	D3D11_INPUT_ELEMENT_DESC D3D::DefaultInputLayoutDesc[] =
	{
		{ "POSITION" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3D* D3D::s_D3D = nullptr;

	uint32_t D3D::DefaultInputLayoutNumElements = 5;
	Material* D3D::DefaultMaterial;
	Material* D3D::DefaultMaterialSkybox;
	Material* D3D::DefaultPostProcess;
	ID3D11SamplerState* D3D::DefaultTextureSampler;

	//cannot place in the header due to heritance
	ShadowUnfiltered* m_ShadowUnfiltered = nullptr;
	ShadowPCF* m_ShadowPCF = nullptr;
	ShadowCascade* m_ShadowCascade = nullptr;
	ShadowVariance* m_ShadowVariance = nullptr;
	ShadowMoment* m_ShadowMoment = nullptr;


	//0 object
	//1 camera
	//2 system
	//3 material
	//4 light
	ID3D11Buffer* D3D::ConstantBuffers[5];
	bool D3D::Init()
	{
		s_D3D = this;
		//create swap chain
		DXGI_SWAP_CHAIN_DESC scd;

		ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));


		scd.BufferCount = 2;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

		RECT windowRect;
		GetClientRect(Themp::System::tSys->m_Window, &windowRect);

		int windowWidth = windowRect.right;
		int windowHeight = windowRect.bottom;

		scd.Windowed = !static_cast<UINT>(Themp::System::tSys->m_SVars.find("Fullscreen")->second);
		if (scd.Windowed)
		{
			scd.BufferDesc.Width = windowRect.right;
			scd.BufferDesc.Height = windowRect.bottom;
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
			NULL, D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, 3,
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


		if (result != S_OK) { System::Print("Could not create D3D11 Device and/or swapchain."); return false; }
		int fl = m_Device->GetFeatureLevel();
		System::Print("FeatureLevel: %s", (fl == D3D_FEATURE_LEVEL_11_1 ? "11_1" : fl == D3D_FEATURE_LEVEL_11_0 ? "11_0" : "10_1"));

		D3D11_FEATURE_DATA_D3D11_OPTIONS3 supportStruct;
		m_Device->CheckFeatureSupport(D3D11_FEATURE::D3D11_FEATURE_D3D11_OPTIONS3, &supportStruct, sizeof(D3D11_FEATURE_DATA_D3D11_OPTIONS3));
		if (!supportStruct.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer)
		{
			System::Print("This GPU does not support 'VP And RT ArrayIndex From Any Shader Feeding Rasterizer', will fall back to slower point light rendering");
		}
		SupportsVPArrayIndex = supportStruct.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer;


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

		int multisample = Themp::System::tSys->m_SVars["Multisample"];
		if (multisample == 0) multisample = 1;
		if (!CreateRenderTextures(windowWidth, windowHeight) || !CreateBackBuffer() || !CreateDepthStencil(windowWidth, windowHeight, multisample))
		{
			System::Print("Could not initialise all required resources, shutting down");
			return false;
		}


		SetViewPort(0.0f, 0.0f, (float)windowWidth, (float)windowHeight);

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

		rDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
		m_Device->CreateRasterizerState(&rDesc, &m_ShadowRasterizerState);

		rDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
		rDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
		m_Device->CreateRasterizerState(&rDesc, &m_WireframeRasterizerState);

		m_DevCon->RSSetState(m_RasterizerState);
		std::vector<std::string> defaultTextures = {
			"DefaultDiffuse.dds",
			"",
			"",
			"",
		};
		std::vector<uint8_t> defaultTypes = { 1,((uint8_t)(-1)),((uint8_t)(-1)),((uint8_t)(-1)) };
		D3D::DefaultMaterial = Resources::TRes->GetMaterial("G-Buffer", defaultTextures, defaultTypes, "Deferred",  false);

		defaultTypes[0] = Material::DIFFUSE;
		defaultTypes[1] = Material::NORMALS;
		defaultTypes[2] = Material::PBR;
		defaultTypes[3] = Material::UNKNOWN;
		defaultTextures[0] = "DefaultDiffuse.dds";
		defaultTextures[1] = "DefaultNormal.dds";
		defaultTextures[2] = "DefaultPBR.dds";
		defaultTextures[3] = "DefaultMisc.dds";
		D3D::DefaultPostProcess = Resources::TRes->GetMaterial("PostProcess", "", "PostProcess",  false);
		std::vector<std::string> skyboxTextures = { "../environmentmaps/Ice_Lake_Cube_Full.dds","../environmentmaps/Ice_Lake_Cube_IBL.dds","../environmentmaps/Tucker_Wreck.dds","../environmentmaps/Tucker_Wreck_IBL.dds" };

		D3D::DefaultMaterialSkybox = Resources::TRes->GetMaterial("Skybox", skyboxTextures, defaultTypes, "Skybox", false);
		System::Print("D3D11 Initialisation success!");

		m_FullScreenQuad = new Object3D();
		Resources::TRes->m_3DObjects.push_back(m_FullScreenQuad);
		m_FullScreenQuad->CreateQuad("ScreenSpace", false);
		m_Skybox = Resources::TRes->GetModel("Skysphere.bin", true);
		if (m_Skybox)
		{
			for (size_t i = 0; i < m_Skybox->m_Meshes.size(); i++)
			{
				m_Skybox->m_Meshes[i]->m_Material = D3D::DefaultMaterialSkybox;
			}
			m_Skybox->m_Scale = XMFLOAT3(1.0, 1.0, 1.0);
			m_Skybox->m_Position = XMFLOAT3(0.520057f, 7.266276f, -84.555794f);
			m_Skybox->isDirty = true;
		}
		else
		{
			System::Print("Skybox model not found!!");
		}


#ifdef _DEBUG
		DebugDraw::DefaultLineMaterial = Resources::TRes->GetMaterial("DebugLine", "", "DebugLine", false, DebugDraw::DefaultLineInputLayoutDesc, 2, false);
#endif // _DEBUG

		//m_ShadowUnfiltered = new ShadowUnfiltered();
		//m_ShadowPCF = new ShadowPCF();
		//m_ShadowCascade = new ShadowCascade(m_ConstantBufferData.num_cascades);
		//m_ShadowCascade->SetCascade(m_ConstantBufferData.num_cascades, 0.1, 500);
		//m_ShadowVariance = new ShadowVariance();
		//m_ShadowMoment = new ShadowMoment();

		SetShadowType(0);
		SetMultiSample(multisample);
		return true;
	}

	void D3D::ResizeWindow(int newX, int newY)
	{
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
			CLEAN(m_MainRender->m_RenderTarget);
			CLEAN(m_MainRender->m_ShaderResourceView);

			//Recreate back buffer
			HRESULT hr;
			// Preserve the existing buffer count and format.
			// Automatically choose the width and height to match the client rect for HWNDs.
			hr = m_Swapchain->ResizeBuffers(0, newX, newY, DXGI_FORMAT_UNKNOWN, 0);
			if (hr != S_OK)
			{
				System::Print("failed to resize buffers");
			}

			// Get buffer and create a render-target-view.
			ID3D11Texture2D* pBuffer;
			hr = m_Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
				(void**)&pBuffer);
			if (hr != S_OK)
			{
				System::Print("failed to obtain backbuffer texture");
			}

			hr = m_Device->CreateRenderTargetView(pBuffer, NULL, &m_BackBuffer);
			if (hr != S_OK)
			{
				System::Print("failed to create rendertarget view from backbuffer");
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

				if (hr != S_OK) { System::Print("Could not CreateTexture2D %i", i); return; }
				// use the back buffer address to create the render target
				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
				memset(&srvDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
				srvDesc.Format = renderTexDesc.Format;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

				hr = m_Device->CreateShaderResourceView(renderTex, &srvDesc, &m_RenderTextures[i]->m_ShaderResourceView);
				if (hr != S_OK) { System::Print("Could not CreateShaderResourceView %i", i); return; }

				m_ShaderResourceViews[i] = m_RenderTextures[i]->m_ShaderResourceView;

				hr = m_Device->CreateRenderTargetView(renderTex, nullptr, &m_RenderTextures[i]->m_RenderTarget);
				if (hr != S_OK) { System::Print("Could not CreateRenderTargetView %i", i); return; }

				renderTex->Release();
				m_Rtvs[i] = m_RenderTextures[i]->m_RenderTarget;
				m_ShaderResourceViews[i] = m_RenderTextures[i]->m_ShaderResourceView;
			}

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

			if (hr != S_OK) { System::Print("Could not CreateTexture2D for Main Render Target"); return; }
			// use the back buffer address to create the render target
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			memset(&srvDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			srvDesc.Format = renderTexDesc.Format;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

			hr = m_Device->CreateShaderResourceView(renderTex, &srvDesc, &m_MainRender->m_ShaderResourceView);
			if (hr != S_OK) { System::Print("Could not CreateShaderResourceView for Main Render Target"); return; }

			hr = m_Device->CreateRenderTargetView(renderTex, nullptr, &m_MainRender->m_RenderTarget);
			if (hr != S_OK) { System::Print("Could not CreateRenderTargetView for Main Render Target"); return; }
			renderTex->Release();

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
			depthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_D16_UNORM;
			depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			depthBufferDesc.CPUAccessFlags = 0;
			depthBufferDesc.MiscFlags = 0;
			hr = m_Device->CreateTexture2D(&depthBufferDesc, NULL, &m_DepthStencil);
			if (hr != S_OK)
			{
				System::Print("failed to create depth texture");
			}

			D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
			memset(&descDSV, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
			descDSV.Format = DXGI_FORMAT_D32_FLOAT;
			descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			descDSV.Texture2D.MipSlice = 0;

			// Create the depth stencil view
			hr = m_Device->CreateDepthStencilView(m_DepthStencil, // Depth stencil texture
				&descDSV, // Depth stencil desc
				&m_DepthStencilView);  // [out] Depth stencil view
			if (hr != S_OK)
			{
				System::Print("Could not create Depthstencil view");
				return;
			}
			D3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc;
			memset(&depthSRVDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			depthSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
			depthSRVDesc.Texture2D.MipLevels = (uint32_t)-1;
			depthSRVDesc.Texture2D.MostDetailedMip = 0;
			depthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

			hr = m_Device->CreateShaderResourceView(m_DepthStencil, // Depth stencil texture
				&depthSRVDesc, // Depth stencil desc
				&m_DepthStencilSRV);  // [out] Depth stencil view
			if (hr != S_OK)
			{
				System::Print("Could not create Depthstencil shader resource view");
				return;
			}
			//m_DevCon->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

			// Set up the viewport.
			D3D11_VIEWPORT vp;
			vp.Width = (float)newX;
			vp.Height = (float)newY;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0.0f;
			vp.TopLeftY = 0.0f;
			m_DevCon->RSSetViewports(1, &vp);
			Themp::System::tSys->m_SVars["WindowSizeX"] = vp.Width;
			Themp::System::tSys->m_SVars["WindowSizeY"] = vp.Height;
			m_ConstantBufferData.screenHeight = vp.Width;
			m_ConstantBufferData.screenWidth = vp.Height;
			m_ConstantBufferData.shadow_atlas_size = ATLAS_RESOLUTION;
			dirtySystemBuffer = true;
		}
	}

	//0: No Filtering
	//1: PCF
	//2: Cascaded
	//3: Variance
	//4: Moment
	void D3D::SetShadowType(int type)
	{
		m_ShadowType = type;

		if (m_ShadowUnfiltered)	delete m_ShadowUnfiltered;
		if (m_ShadowPCF)		delete m_ShadowPCF;
		if (m_ShadowCascade)	delete m_ShadowCascade;
		if (m_ShadowVariance)	delete m_ShadowVariance;
		if (m_ShadowMoment)		delete m_ShadowMoment;

		m_ShadowUnfiltered = nullptr;
		m_ShadowPCF = nullptr;
		m_ShadowCascade = nullptr;
		m_ShadowVariance = nullptr;
		m_ShadowMoment = nullptr;

		switch (m_ShadowType)
		{
		case 0:
			m_ShadowUnfiltered = new ShadowUnfiltered();
			break;
		case 1:
			m_ShadowPCF = new ShadowPCF();
			break;
		case 2:
			m_ShadowCascade = new ShadowCascade(m_ConstantBufferData.num_cascades);
			m_ShadowCascade->SetCascade(m_ConstantBufferData.num_cascades);
			break;
		case 3:
			m_ShadowVariance = new ShadowVariance();
			break;
		case 4:
			m_ShadowMoment = new ShadowMoment();
			break;
		}
	}
	bool D3D::SetMultiSample(int num)
	{
		if (num != 0 && num != 1 && num != 2 && num != 4 && num != 8)
		{
			System::Print("Multiplesample value (\"num\") has to be 0, 1, 2, 4 or 8, given: %i. Nothing has changed", num);
			return false;
		}
		if (num == 0)
		{
			System::Print("Multisample value (\"num\") == 0, this is allowed because it's set to 1, but no real MS value of 0 exists");
			num = 1;
		}
		if (m_Swapchain)
		{
			m_DevCon->OMSetRenderTargets(0, 0, 0);
			dirtySystemBuffer = true;
		}
		else
		{
			return false;
		}
		m_ConstantBufferData.MSAAValue = num;
		dirtySystemBuffer = true;

		if (m_ShadowUnfiltered)	m_ShadowUnfiltered->SetMultiSample(num);
		if (m_ShadowPCF)		m_ShadowPCF->SetMultiSample(num);
		if (m_ShadowCascade)	m_ShadowCascade->SetMultiSample(num);
		if (m_ShadowVariance)	m_ShadowVariance->SetMultiSample(num);
		if (m_ShadowMoment)		m_ShadowMoment->SetMultiSample(num);

		//mark all shadows dirty as they have to be recalculated
		if (m_ShadowUnfiltered)	m_ShadowUnfiltered->SetDirty();
		if (m_ShadowPCF)		m_ShadowPCF->SetDirty();
		if (m_ShadowCascade)	m_ShadowCascade->SetDirty();
		if (m_ShadowVariance)	m_ShadowVariance->SetDirty();
		if (m_ShadowMoment)		m_ShadowMoment->SetDirty();
		return true;
	}
	void D3D::SetLightDirty(LightType type, int index)
	{
		if (m_ShadowUnfiltered)	m_ShadowUnfiltered->SetLightDirty(type, index);
		if (m_ShadowPCF)		m_ShadowPCF->SetLightDirty(type, index);
		if (m_ShadowCascade)	m_ShadowCascade->SetLightDirty(type, index);
		//if (m_ShadowVariance)	m_ShadowVariance->SetLightDirty(type, index);
		//if (m_ShadowMoment)		m_ShadowMoment->SetLightDirty(type, index);
	}
	void D3D::SetDirectionalLight(int index, bool enabled, XMFLOAT4 pos , XMFLOAT4 dir, XMFLOAT4 color)
	{
		if (m_ShadowUnfiltered)	m_ShadowUnfiltered->SetDirectionalLight(index, enabled, pos, dir, color);
		if (m_ShadowPCF)		m_ShadowPCF->SetDirectionalLight(index, enabled, pos, dir, color);
		if (m_ShadowCascade)	m_ShadowCascade->SetDirectionalLight(index, enabled, pos, dir, color);
		//if (m_ShadowVariance)
		//if (m_ShadowMoment)
	}
	void D3D::SetNumberCascades(int numCascades)
	{
		if (m_ShadowCascade) m_ShadowCascade->SetCascade(numCascades);
	}
	void D3D::DrawGBufferPass(Game & game)
	{
		Themp::D3D& _this = *static_cast<Themp::D3D*>(this);
		if (m_Wireframe)
		{
			m_DevCon->RSSetState(m_WireframeRasterizerState);
		}
		else
		{
			m_DevCon->RSSetState(m_RasterizerState);
		}
		m_DevCon->OMSetRenderTargets(NUM_RENDER_TEXTURES, m_Rtvs, m_DepthStencilView);
		m_DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_DevCon->IASetInputLayout(D3D::DefaultMaterial->m_InputLayout); //skybox layout doesn't differ so this is fine

		//Skybox
		m_DevCon->OMSetDepthStencilState(m_SkyboxDepthStencilState, 1);
		m_DevCon->PSSetShaderResources(0, 2, D3D::DefaultMaterialSkybox->m_Views);
		m_DevCon->PSSetShader(D3D::DefaultMaterialSkybox->m_PixelShader, 0, 0);
		m_DevCon->VSSetShader(D3D::DefaultMaterialSkybox->m_VertexShader, 0, 0);
		m_DevCon->GSSetShader(D3D::DefaultMaterialSkybox->m_GeometryShader, 0, 0);

		m_Skybox->m_Position = game.m_Camera->GetPosition();
		m_Skybox->isDirty = true;
		m_Skybox->Draw(_this, Mesh::DrawPass::GBUFFER);

		//Models
		m_DevCon->OMSetDepthStencilState(m_DepthStencilState, 1);
		m_DevCon->PSSetShaderResources(0, 4, D3D::DefaultMaterial->m_Views);

		m_DevCon->PSSetShader(D3D::DefaultMaterial->m_PixelShader, 0, 0);
		m_DevCon->VSSetShader(D3D::DefaultMaterial->m_VertexShader, 0, 0);
		m_DevCon->GSSetShader(D3D::DefaultMaterial->m_GeometryShader, 0, 0);

		for (int i = 0; i < game.m_Objects3D.size(); ++i)
		{
			game.m_Objects3D[i]->Draw(_this, Mesh::DrawPass::GBUFFER);
		}

#ifdef _DEBUG
		DebugDraw::Draw(m_Device, m_DevCon);
#endif // _DEBUG

	}
	Camera* shadowCamera; 
	void D3D::DrawShadowMaps(Themp::Game& game)
	{
		switch (m_ShadowType)
		{
		case 0:
			m_ShadowUnfiltered->DrawShadow();
			break;
		case 1:
			m_ShadowPCF->DrawShadow();
			break;
		case 2:
			m_ShadowCascade->DrawShadow();
			break;
		case 3:
			m_ShadowVariance->DrawShadow();
			break;
		case 4:
			m_ShadowMoment->DrawShadow();
			break;
		}
	}
	void D3D::DrawLightPass()
	{
		switch (m_ShadowType)
		{
		case 0:
			m_ShadowUnfiltered->DrawLight();
			break;
		case 1:
			m_ShadowPCF->DrawLight();
			break;
		case 2:
			m_ShadowCascade->DrawLight();
			break;
		case 3:
			m_ShadowVariance->DrawLight();
			break;
		case 4:
			m_ShadowMoment->DrawLight();
			break;
		}
	}
	void D3D::DrawPostProcess()
	{
		m_DevCon->RSSetState(m_RasterizerState);
		m_DevCon->PSSetShader(D3D::DefaultPostProcess->m_PixelShader, 0, 0);
		m_DevCon->VSSetShader(D3D::DefaultPostProcess->m_VertexShader, 0, 0);
		m_DevCon->GSSetShader(D3D::DefaultPostProcess->m_GeometryShader, 0, 0);

		SetViewPort(0, 0, m_ConstantBufferData.screenWidth, m_ConstantBufferData.screenHeight);
		//m_DevCon->OMSetRenderTargets(1, &m_BackBuffer, m_DepthStencilView);
		m_DevCon->OMSetRenderTargets(1, &m_BackBuffer, nullptr);
		m_DevCon->PSSetSamplers(0, 4, D3D::DefaultPostProcess->m_SamplerStates);

		for (size_t i = 0; i < NUM_RENDER_TEXTURES; i++)
		{
			m_ShaderResourceViews[i] = m_RenderTextures[i]->m_ShaderResourceView;
		}
		m_ShaderResourceViews[NUM_RENDER_TEXTURES] = m_DepthStencilSRV;
		m_ShaderResourceViews[NUM_RENDER_TEXTURES+1] = m_MainRender->m_ShaderResourceView;

		m_DevCon->PSSetShaderResources(0, NUM_RENDER_TEXTURES + 2, m_ShaderResourceViews);


		// draw fullscreen quad, manually
		uint32_t stride[] = { sizeof(Vertex) };
		uint32_t offset[] = { 0 };
		m_DevCon->IASetVertexBuffers(0, 1, &m_FullScreenQuad->m_Meshes[0]->m_VertexBuffer, stride, offset);
		m_DevCon->IASetIndexBuffer(m_FullScreenQuad->m_Meshes[0]->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		m_DevCon->IASetInputLayout(D3D::DefaultPostProcess->m_InputLayout);

		SetSystemConstantBuffer(m_CBuffer);
		PSUploadConstantBuffersToGPU();

		m_DevCon->DrawIndexed(m_FullScreenQuad->m_Meshes[0]->m_NumIndices, 0, 0);
	}
	void D3D::Draw(Themp::Game& game)
	{
		static const float ClearColor[4] = { 0.0f, 0.2f, 0.4f, 0.0f };
		for (size_t i = 0; i < NUM_RENDER_TEXTURES; i++)
		{
			m_DevCon->ClearRenderTargetView(m_RenderTextures[i]->m_RenderTarget, ClearColor);
		}
		m_DevCon->ClearRenderTargetView(m_BackBuffer, ClearColor);
		m_DevCon->ClearRenderTargetView(m_MainRender->m_RenderTarget, ClearColor);
		m_DevCon->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.0f, 0);

		//Draw all geometry to the GBuffer //positions, normals, etc
		DrawGBufferPass(game);

		//If there are any dirty lights, render the shadow maps for those lights
		DrawShadowMaps(game);

		//Draw everything to a fullscreen quad, while calculating lighting 
		DrawLightPass();

		DrawPostProcess();

		//set stuff back to prevent issues next draw
		VSUploadConstantBuffersToGPUNull();
		PSUploadConstantBuffersToGPUNull();
		GSUploadConstantBuffersToGPUNull();
		for (size_t i = 0; i < 32; i++)
		{
			m_ShaderResourceViews[i] = nullptr;
		}
		m_DevCon->PSSetShaderResources(0, 32, m_ShaderResourceViews);
	}
	void D3D::DrawImGUI()
	{
		ImDrawData* drawData = ImGui::GetDrawData();
		GUI::gui->PrepareDraw(drawData);
		GUI::gui->Draw(drawData);
		GUI::gui->EndDraw();
		m_Swapchain->Present(0, 0);
	}
	D3D::~D3D()
	{
#if _DEBUG
		DebugDraw::Destroy();
#endif

		if (Themp::System::tSys->m_SVars.find("Fullscreen")->second == 1)
		{
			m_Swapchain->SetFullscreenState(FALSE, NULL);  // switch to windowed mode
		}
		CLEAN(m_CBuffer);
		CLEAN(D3D::ConstantBuffers[0]);
		CLEAN(D3D::ConstantBuffers[1]);
		CLEAN(D3D::ConstantBuffers[2]);
		CLEAN(D3D::DefaultTextureSampler);

		for (size_t i = 0; i < NUM_RENDER_TEXTURES; i++)
		{
			delete m_RenderTextures[i];
		}
		delete m_MainRender;
		VSUploadConstantBuffersToGPUNull();
		PSUploadConstantBuffersToGPUNull();
		GSUploadConstantBuffersToGPUNull();

		if (m_ShadowUnfiltered)	delete m_ShadowUnfiltered;
		if (m_ShadowPCF)		delete m_ShadowPCF;
		if (m_ShadowCascade)	delete m_ShadowCascade;
		if (m_ShadowVariance)	delete m_ShadowVariance;
		if (m_ShadowMoment)		delete m_ShadowMoment;

		CLEAN(m_OMBlendState);
		CLEAN(m_DepthStencil);
		CLEAN(m_DepthStencilView);
		CLEAN(m_DepthStencilState);
		CLEAN(m_SkyboxDepthStencilState);
		CLEAN(m_ShadowClearDepthStencilState);
		CLEAN(m_DepthStencilSRV);
		CLEAN(m_RasterizerState);
		CLEAN(m_ShadowRasterizerState);
		CLEAN(m_WireframeRasterizerState);
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
		if (D3D::ConstantBuffers[1] == nullptr) System::Print("No Camera active in scene");
		m_DevCon->VSSetConstantBuffers(0, 3, D3D::ConstantBuffers);
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
	bool D3D::CreateBackBuffer()
	{
		HRESULT result;
		////get back buffer
		ID3D11Texture2D *pBackBuffer;
		result = m_Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
		if (result != S_OK) { System::Print("Could not obtain BackBuffer"); return false; }
		// use the back buffer address to create the render target
		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		ZeroMemory(&RTVDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
		RTVDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		result = m_Device->CreateRenderTargetView(pBackBuffer, NULL, &m_BackBuffer);
		if (result != S_OK) { System::Print("Could not create Render target (BackBuffer)"); return false; }
		pBackBuffer->Release();
		return true;
	}
	bool D3D::CreateRenderTextures(int width, int height)
	{
		for (int i = 0; i < NUM_RENDER_TEXTURES; i++)
		{
			m_RenderTextures[i] = new RenderTexture(width,height,RenderTexture::RenderTex, 1);
			m_ShaderResourceViews[i] = m_RenderTextures[i]->m_ShaderResourceView;
			m_Rtvs[i] = m_RenderTextures[i]->m_RenderTarget;
		}
		m_MainRender = new RenderTexture(width, height, RenderTexture::RenderTex, 1);
		return true;
	}
	bool D3D::CreateDepthStencil(int width, int height,int multisample)
	{
		HRESULT result;
		SetMultiSample(multisample);
		
		D3D11_DEPTH_STENCIL_DESC dsDesc;

		// Depth test parameters
		dsDesc.DepthEnable = true;
		dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

		// Stencil test parameters
		dsDesc.StencilEnable = false;
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
		result = m_Device->CreateDepthStencilState(&dsDesc, &m_DepthStencilState);
		if (result != S_OK)
		{
			System::Print("Could not create Depthstencil state");
			return false;
		}
		m_DevCon->OMSetDepthStencilState(m_DepthStencilState, 1);

		
		D3D11_DEPTH_STENCIL_DESC dssDesc;
		ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		dssDesc.DepthEnable = true;
		dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		result = m_Device->CreateDepthStencilState(&dssDesc, &m_SkyboxDepthStencilState);
		if (result != S_OK)
		{
			System::Print("Could not create Skybox Depthstencil state");
			return false;
		}

		ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
		dssDesc.DepthEnable = true;
		dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dssDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		dssDesc.StencilEnable = false;
		dssDesc.StencilReadMask = 0xFF;
		dssDesc.StencilWriteMask = 0xFF;

		result = m_Device->CreateDepthStencilState(&dssDesc, &m_ShadowClearDepthStencilState);
		if (result != S_OK)
		{
			System::Print("Could not create ShadowClear Depthstencil state");
			return false;
		}
		return true;
	}
	void D3D::PrepareSystemBuffer()
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