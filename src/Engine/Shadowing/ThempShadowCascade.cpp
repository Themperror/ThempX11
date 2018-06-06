#include "ThempSystem.h"
#include "ThempShadowCascade.h"
#include "ThempD3D.h"
#include "ThempResources.h"
#include "ThempMesh.h"
#include "ThempCamera.h"
#include "ThempShadowAtlas.h"
#include "ThempRenderTexture.h"
#include "ThempMaterial.h"
#include "ThempObject3D.h"
#include "ThempFunctions.h"
#include "ThempDebugDraw.h"
#include "../../Game/ThempGame.h"

#include <iostream>
#include <limits>
using namespace DirectX;

namespace Themp
{

	float ShadowCascade::CascadeBounds[MAX_CASCADES] = { 20 , 40 , 60, 80 , 100 , 120,};
	float ShadowCascade::CascadeNearPlane = 0.1f;
	float ShadowCascade::CascadeFarPlane = 1000.0f;

	ShadowCascade::ShadowCascade(int num_Cascades)
	{
		for (size_t i = 0; i < MAX_CASCADES; i++)
		{
			m_CascadeShadows[i] = nullptr;
		}
		m_NumCascades = num_Cascades;
		m_Multisample = Themp::System::tSys->m_SVars["Multisample"];
		m_ShadowCamera = new Camera();
		m_ShadowCamera->SetOrtho(400, 400);
		m_DirShadowAtlas = new ShadowAtlas(ATLAS_RESOLUTION);
		m_MiscShadowAtlas = new ShadowAtlas(ATLAS_RESOLUTION);
		m_ShadowCamera->SetProjection(Camera::CameraType::Orthographic);
		m_CascadedLightingMaterial = Resources::TRes->GetMaterial("CascadedLighting", "", "DeferredLightingCascade", false);
		m_CascadedLightingMultiSampleMaterial = Resources::TRes->GetMaterial("CascadedLightingMS", "", "DeferredLightingCascade",false, 0, 0, true);
		m_DirectionalShadowMaterial = Resources::TRes->GetMaterial("ShadowDirectionalCascaded", "", "DeferredShadowCascade",true);
		m_DirectionalShadowMultiSampleMaterial = Resources::TRes->GetMaterial("ShadowDirectionalCascadedMS", "", "DeferredShadowCascade",true,0,0,true);
		m_ShadowClearMaterial = Resources::TRes->GetMaterial("ScreenSpace", "", "ScreenSpace",false);
		
		if (D3D::s_D3D->SupportsVPArrayIndex)
		{
			m_ShadowMaterial = Resources::TRes->GetMaterial("DeferredShadow", "", "DeferredShadow", true);
		}
		else
		{
			m_ShadowMaterial = Resources::TRes->GetMaterial("DeferredShadowFallback", "", "DeferredShadowFallback",  false);
		}
		
		D3D11_BUFFER_DESC cbDesc;
		cbDesc.ByteWidth = sizeof(CascadeLightBuffer);
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = &m_CascadeLightBufferData;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the buffer.
		D3D::s_D3D->m_Device->CreateBuffer(&cbDesc, &InitData, &m_LightBuffer);
		m_CascadeLightBufferData.numDir = NUM_LIGHTS;
		for (size_t i = 0; i < NUM_LIGHTS; i++)
		{
			m_Lights[i].l = &m_CascadeLightBufferData.dirLights[i];
			m_Lights[i].LightIsDirty = true;
			m_CascadeLightBufferData.dirLights[i].enabled = false;
			m_CascadeLightBufferData.dirLights[i].position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
			m_CascadeLightBufferData.dirLights[i].direction = XMFLOAT4(1.0f, 0.0f, 0.0f,0.0f);
			m_CascadeLightBufferData.dirLights[i].color = XMFLOAT4(1.0f,1.0f,1.0f,1.0f);
			m_CascadeLightBufferData.dirLights[i].textureOffset = m_DirShadowAtlas->ObtainTextureArea(SHADOW_RESOLUTION);
			m_DirtyLights = true;
		}
	}
	void ShadowCascade::CalculateSplits(float nearPlane, float farPlane)
	{
		m_SplitFar.clear();
		m_SplitNear.clear();
		m_SplitFar.reserve(m_NumCascades);
		m_SplitNear.reserve(m_NumCascades);
		for (size_t i = 0; i < m_NumCascades; i++)
		{
			m_SplitFar.push_back(farPlane);
			m_SplitNear.push_back(nearPlane);
		}

		//calculate the projection near and far plane for each cascade (edited from: https://promethe.io/2015/01/21/exponential-cascaded-shadow-mapping-with-webgl/ )
		float strength = 0.8f;
		float j = 1.0f;
		float ratio = farPlane / nearPlane;
		for (int i = 0; i < m_NumCascades - 1; ++i, j += 1.0f)
		{
			m_SplitFar[i] = lerp(
				nearPlane + (j / (float)m_NumCascades) * (farPlane - nearPlane),
				nearPlane * powf(ratio, j / (float)m_NumCascades),
				strength
			);
			m_SplitNear[i + 1] = m_SplitFar[i];
		}
		
		//set all the splits in the buffer
		//memsetting floats is bad, so using a forloop
		for (size_t i = 0; i < 8; i++)
		{
			(&m_CascadeLightBufferData.split0)[i] = 0.0f;
		}
		m_CascadeLightBufferData.split0 = 0.0f;
		float* splits = &m_CascadeLightBufferData.split1;
		for (size_t i = 0; i < m_SplitFar.size(); i++)
		{
			splits[i] = m_SplitFar[i];
		}


	}
	//Set the Cascade shadow map settings, this calculates cascades and its near and far planes
	void ShadowCascade::SetCascade(int numCascades)
	{
		numCascades = numCascades > MAX_CASCADES ? MAX_CASCADES : numCascades < 2 ? 2 : numCascades;
		m_NumCascades = numCascades;
		
		
		for (size_t i = 0; i < MAX_CASCADES; i++)
		{
			if (m_CascadeShadows[i])
			{
				delete m_CascadeShadows[i];
				m_CascadeShadows[i] = nullptr;
			}
		}
		for (size_t i = 0; i < m_NumCascades; i++)
		{
			m_CascadeShadows[i] = new RenderTexture(ATLAS_RESOLUTION, ATLAS_RESOLUTION, RenderTexture::TextureType::DepthTex, m_Multisample);
		}

		if (m_MiscShadows)
		{
			delete m_MiscShadows;
		}
		m_MiscShadows = new RenderTexture(ATLAS_RESOLUTION, ATLAS_RESOLUTION, RenderTexture::TextureType::DepthTex, m_Multisample);


		m_DirtyLights = true;
		for (size_t i = 0; i < NUM_LIGHTS*3; i++)
		{
			m_Lights[i].LightIsDirty = true;
		}
		CalculateSplits(System::tSys->m_Game->m_Camera->GetNear(), System::tSys->m_Game->m_Camera->GetFar());
	}
	ShadowCascade::~ShadowCascade()
	{
		for (size_t i = 0; i < MAX_CASCADES; i++)
		{
			if (m_CascadeShadows[i])
			{
				delete m_CascadeShadows[i];
				m_CascadeShadows[i] = nullptr;
			}
		}
		if (m_MiscShadows)
		{
			delete m_MiscShadows;
			m_MiscShadows = nullptr;
		}
		if (m_DirShadowAtlas)
		{
			delete m_DirShadowAtlas;
			m_DirShadowAtlas = nullptr;
		}
		if (m_MiscShadowAtlas)
		{
			delete m_MiscShadowAtlas;
			m_MiscShadowAtlas = nullptr;
		}
		if (m_ShadowCamera)
		{
			delete m_ShadowCamera;
			m_ShadowCamera = nullptr;
		}
		if (m_LightBuffer)
		{
			m_LightBuffer->Release();
			m_LightBuffer = nullptr;
		}
	}
	Material* usedShadowMaterial = nullptr;
	void ShadowCascade::DrawShadow()
	{
		D3D* l_D3D = D3D::s_D3D;
		PreDraw();
		if (m_Multisample == 1)
		{
			usedShadowMaterial = m_DirectionalShadowMaterial;
		}
		else
		{
			usedShadowMaterial = m_DirectionalShadowMultiSampleMaterial;
		}

		l_D3D->m_DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		l_D3D->m_DevCon->IASetInputLayout(m_ShadowMaterial->m_InputLayout);

		l_D3D->m_DevCon->RSSetState(l_D3D->m_RasterizerState);

		DrawDirectionalShadow();
		usedShadowMaterial = m_ShadowMaterial;
		//DrawPointShadow();
	}
	void ShadowCascade::DrawDirectionalShadow()
	{
		D3D* l_D3D = D3D::s_D3D;
		Game* l_Game = Themp::System::tSys->m_Game;
		if (m_DirtyLights)
		{
			l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_ShadowClearDepthStencilState, 1);
			//clear the shadow atlas
			for (size_t i = 0; i < m_CascadeLightBufferData.numDir; i++)
			{
				DirectionalLight* l = &m_CascadeLightBufferData.dirLights[i];
				l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_ShadowClearDepthStencilState, 1);
				l_D3D->m_DevCon->PSSetShader(m_ShadowClearMaterial->m_PixelShader, 0, 0);
				l_D3D->m_DevCon->VSSetShader(m_ShadowClearMaterial->m_VertexShader, 0, 0);
				l_D3D->m_DevCon->GSSetShader(nullptr, 0, 0);
				l_D3D->SetViewPort(l->textureOffset.x, l->textureOffset.y, l->textureOffset.z, l->textureOffset.w);
				for (size_t j = 0; j < m_NumCascades; j++)
				{
					l_D3D->m_DevCon->OMSetRenderTargets(0, nullptr, m_CascadeShadows[j]->m_DepthStencilView);
					l_D3D->m_FullScreenQuad->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
				}
			}

			l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_DepthStencilState, 1);
			l_D3D->m_DevCon->RSSetState(l_D3D->m_ShadowRasterizerState);

			//set up viewports
			D3D11_VIEWPORT viewports[NUM_LIGHTS];
			
			for (size_t j = 0; j < m_CascadeLightBufferData.numDir; j++)
			{
				viewports[j].TopLeftX = m_CascadeLightBufferData.dirLights[j].textureOffset.x;
				viewports[j].TopLeftY = m_CascadeLightBufferData.dirLights[j].textureOffset.y;
				viewports[j].MinDepth = 0;
				viewports[j].MaxDepth = 1.0;
				viewports[j].Width = m_CascadeLightBufferData.dirLights[j].textureOffset.z;
				viewports[j].Height = m_CascadeLightBufferData.dirLights[j].textureOffset.w;
			}
			l_D3D->m_DevCon->RSSetViewports(m_CascadeLightBufferData.numDir, viewports);

			l_D3D->m_DevCon->PSSetShader(usedShadowMaterial->m_PixelShader, 0, 0);
			l_D3D->m_DevCon->VSSetShader(usedShadowMaterial->m_VertexShader, 0, 0);
			l_D3D->m_DevCon->GSSetShader(usedShadowMaterial->m_GeometryShader, 0, 0);
			l_D3D->SetLightConstantBuffer(m_LightBuffer);
			l_D3D->GSUploadConstantBuffersToGPU();

			//draw the shadow maps for every cascade
			for (size_t i = 0; i < m_NumCascades; i++)
			{
				m_CascadeLightBufferData.cascadeIndex = i;

				D3D11_MAPPED_SUBRESOURCE ms;
				l_D3D->m_DevCon->Map(m_LightBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
				memcpy(ms.pData, &m_CascadeLightBufferData, sizeof(CascadeLightBuffer));
				l_D3D->m_DevCon->Unmap(m_LightBuffer, NULL);

				l_D3D->m_DevCon->OMSetRenderTargets(0, nullptr, m_CascadeShadows[i]->m_DepthStencilView);
				for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
				{
					l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
				}
			}
			l_D3D->m_DevCon->GSSetShader(nullptr, 0, 0);
			m_DirtyLights = false;
		}
		l_D3D->GSUploadConstantBuffersToGPUNull();
	}
	void ShadowCascade::DrawLight()
	{
		D3D* l_D3D = D3D::s_D3D;
		l_D3D->m_DevCon->RSSetState(l_D3D->m_RasterizerState);
		Material* usedLightMaterial = m_Multisample == 1 ? m_CascadedLightingMaterial : m_CascadedLightingMultiSampleMaterial;
		l_D3D->m_DevCon->PSSetShader(usedLightMaterial->m_PixelShader, 0, 0);
		l_D3D->m_DevCon->VSSetShader(usedLightMaterial->m_VertexShader, 0, 0);
		l_D3D->m_DevCon->GSSetShader(nullptr, 0, 0);

		l_D3D->SetViewPort(0, 0, l_D3D->m_ConstantBufferData.screenWidth, l_D3D->m_ConstantBufferData.screenHeight);
		//m_DevCon->OMSetRenderTargets(1, &m_BackBuffer, m_DepthStencilView);
		l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_DepthStencilState,0);
		l_D3D->m_DevCon->OMSetRenderTargets(1, &l_D3D->m_MainRender->m_RenderTarget, nullptr);
		l_D3D->m_DevCon->PSSetSamplers(0, 4, usedLightMaterial->m_SamplerStates);

		for (size_t i = 0; i < NUM_RENDER_TEXTURES; i++)
		{
			l_D3D->m_ShaderResourceViews[i] = l_D3D->m_RenderTextures[i]->m_ShaderResourceView;
		}
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES] = l_D3D->m_DepthStencilSRV;
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 1] = m_MiscShadows->m_ShaderResourceView;
		for (size_t i = 0; i < MAX_CASCADES; i++)
		{
			if (m_CascadeShadows[i])
			{
				l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 2 + i] = m_CascadeShadows[i]->m_ShaderResourceView;
			}
			else
			{
				l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 2 + i] = nullptr;
			}
		}
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 2 + MAX_CASCADES] = l_D3D->DefaultMaterialSkybox->m_Textures[0]->m_View;
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 3 + MAX_CASCADES] = l_D3D->DefaultMaterialSkybox->m_Textures[1]->m_View;
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 4 + MAX_CASCADES] = l_D3D->DefaultMaterialSkybox->m_Textures[2]->m_View;
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 5 + MAX_CASCADES] = l_D3D->DefaultMaterialSkybox->m_Textures[3]->m_View;

		//every cascade shadow + 1 for regular lights
		l_D3D->m_DevCon->PSSetShaderResources(0, NUM_SHADER_RESOURCE_VIEWS + MAX_CASCADES + 1, l_D3D->m_ShaderResourceViews);


		// draw fullscreen quad, manually
		uint32_t stride[] = { sizeof(Vertex) };
		uint32_t offset[] = { 0 };
		l_D3D->m_DevCon->IASetVertexBuffers(0, 1, &l_D3D->m_FullScreenQuad->m_Meshes[0]->m_VertexBuffer, stride, offset);
		l_D3D->m_DevCon->IASetIndexBuffer(l_D3D->m_FullScreenQuad->m_Meshes[0]->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		l_D3D->m_DevCon->IASetInputLayout(usedLightMaterial->m_InputLayout);

		l_D3D->m_DevCon->PSSetSamplers(0, 4, usedLightMaterial->m_SamplerStates);
		l_D3D->SetSystemConstantBuffer(l_D3D->m_CBuffer);
		Themp::System::tSys->m_Game->m_Camera->UpdateMatrices();
		l_D3D->SetLightConstantBuffer(m_LightBuffer);
		l_D3D->PSUploadConstantBuffersToGPU();

		l_D3D->m_DevCon->DrawIndexed(l_D3D->m_FullScreenQuad->m_Meshes[0]->m_NumIndices, 0, 0);
	}
	void ShadowCascade::DrawFrustum(Frustum& f, XMMATRIX lightProj, XMMATRIX lightView,XMMATRIX camViewProj, int n)
	{
		XMMATRIX temp, inv;
		temp = lightView * lightProj;
		inv = XMMatrixInverse(nullptr,temp); 
		//camViewProj = XMMatrixInverse(nullptr,camViewProj); 

		XMFLOAT4 fr[8] =
		{
			// near
			{ -1, -1, -1, 1 },{ 1, -1, -1, 1 },{ 1,  1, -1, 1 },{ -1,  1, -1, 1 },
			// far
			{ -1, -1, 1, 1 },{ 1, -1, 1, 1 },{ 1,  1, 1, 1 },{ -1,  1, 1, 1 }
		};//unused atm, was trying something else..


		XMVECTOR tfr[8];
		for (int i = 0; i<8; i++)
		{
			fr[i] = ToXMFLOAT4(f.corners[i], 1.0);
			tfr[i] = XMLoadFloat4(&fr[i]);
			//tfr[i] = XMVector4Transform(tfr[i], inv);
			//tfr[i] = XMVector4Transform(tfr[i], camViewProj);
			XMStoreFloat4(&fr[i], tfr[i]);

			//divide by W value
			XMFLOAT4 wDivT = XMFLOAT4(fr[i].w, fr[i].w, fr[i].w, fr[i].w);
			tfr[i] = XMVectorDivide(tfr[i], XMLoadFloat4(&wDivT));
			XMStoreFloat4(&fr[i], tfr[i]);
			
			fr[i].w = 1.0;
		}
		for (int i = 0; i < 8; i++)
		{
			//fr[i] = ToXMFLOAT4(f.corners[i],1.0);
		}
		XMFLOAT3 color[6] =
		{
			XMFLOAT3(1,1,1),
			XMFLOAT3(1,0,0),
			XMFLOAT3(0,1,0),
			XMFLOAT3(0,0,1),
			XMFLOAT3(1,0,1),
			XMFLOAT3(1,1,0),
		};

		//this seems correct, its just a matter of getting the matrices right
		DebugDraw::Line(ToXMFLOAT3(fr[0]), ToXMFLOAT3(fr[1]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[1]), ToXMFLOAT3(fr[2]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[2]), ToXMFLOAT3(fr[3]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[3]), ToXMFLOAT3(fr[0]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[4]), ToXMFLOAT3(fr[5]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[5]), ToXMFLOAT3(fr[6]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[6]), ToXMFLOAT3(fr[7]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[7]), ToXMFLOAT3(fr[4]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[0]), ToXMFLOAT3(fr[4]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[1]), ToXMFLOAT3(fr[5]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[2]), ToXMFLOAT3(fr[6]),5.0,color[n]);
		DebugDraw::Line(ToXMFLOAT3(fr[3]), ToXMFLOAT3(fr[7]),5.0,color[n]);
		//0 - 1, 1 - 2, 2 - 3, 3 - 0,
		//4 - 5, 5 - 6, 6 - 7, 7 - 4,
		//0 - 4, 1 - 5, 2 - 6, 3 - 7
	}
	void ShadowCascade::PreDraw()
	{
		Camera* cam = Themp::System::tSys->m_Game->m_Camera;
		XMMATRIX cameraViewMatrix  = XMLoadFloat4x4(&cam->m_CameraConstantBufferData.viewMatrix);
		XMMATRIX cameraProjectionMatrix =  XMLoadFloat4x4(&cam->m_CameraConstantBufferData.projectionMatrix);
		float oldNear = cam->GetNear(), oldFar = cam->GetFar();
		XMMATRIX camInvView = XMMatrixInverse(nullptr, cameraViewMatrix);
		for (size_t i = 0; i < m_CascadeLightBufferData.numDir; i++)
		{
			if (!m_Lights[i].l->enabled)
			{
				continue;
			}
			XMFLOAT4X4 lightViewM;
			m_DirtyLights = true;
			DirectionalLight& l = m_CascadeLightBufferData.dirLights[i];
			m_ShadowCamera->SetLookTo(l.direction, XMFLOAT3(0, 1, 0), XMFLOAT3(1, 0, 0));
			m_ShadowCamera->SetProjection(Camera::CameraType::Orthographic);
			m_ShadowCamera->SetNear(0.1f);
			m_ShadowCamera->SetFar(1000.0f);
			for (int j = 0; j < m_NumCascades; j++)
			{
				cam->SetNear(m_SplitNear[j]);
				cam->SetFar(m_SplitFar[j]);

				XMFLOAT3 p1 = cam->GetPosition() + (cam->GetForward() * m_SplitNear[j]);
				XMFLOAT3 p2 = cam->GetPosition() + (cam->GetForward() * m_SplitFar[j]);
				XMVECTOR lerpVal = XMVectorLerp(XMLoadFloat3(&p1), XMLoadFloat3(&p2),0.5f);
				XMStoreFloat3(&p1, lerpVal);

				XMFLOAT4 cameraCenterViewFrustum = ToXMFLOAT4(p1, 1.0); //get the center of the current cascade fragment
				cameraCenterViewFrustum = cameraCenterViewFrustum - (l.direction * (m_ShadowCamera->GetFar()*0.5f));
				
				//float worldUnitsPerTexel = CascadeBounds[j] / (float)SHADOW_RESOLUTION;
				//XMVECTOR worldUnitsPerTexelV = XMVectorSet(worldUnitsPerTexel, worldUnitsPerTexel, worldUnitsPerTexel, 1.0f);
				//XMVECTOR cameraCenterViewFrustumV = XMLoadFloat4(&cameraCenterViewFrustum);
				//cameraCenterViewFrustumV /= worldUnitsPerTexelV;
				//cameraCenterViewFrustumV = XMVectorFloor(cameraCenterViewFrustumV);
				//cameraCenterViewFrustumV = XMVectorMultiply(cameraCenterViewFrustumV, worldUnitsPerTexelV);
				//XMStoreFloat4(&cameraCenterViewFrustum, cameraCenterViewFrustumV);

				m_ShadowCamera->SetPosition(cameraCenterViewFrustum); //focus light position on that point with offset
				lightViewM = m_ShadowCamera->GetViewMatrix();

				XMMATRIX lightViewMatrix = XMLoadFloat4x4(&lightViewM);
				XMFLOAT4X4 camProj = cam->GetPerspectiveProjectionMatrix();
				XMMATRIX camProjM = XMLoadFloat4x4(&camProj);
				//ComputeFrustumFromProjection(&m_Frustums[i][j], &camProjM);
				XMVECTOR vFrustumPoints[8];
				CreateFrustumPointsFromCascadeInterval(m_SplitNear[j], m_SplitFar[j], camProjM, m_Frustums[i][j], &vFrustumPoints[0]);

				Frustum& frustum = m_Frustums[i][j];
				//CalculateFrustumPoints(cam,&frustum);

				
				XMFLOAT4 minBounds;
				XMFLOAT4 maxBounds;
				minBounds.x = std::numeric_limits<float>::max();
				maxBounds.x = std::numeric_limits<float>::lowest();
				minBounds.y = std::numeric_limits<float>::max();
				maxBounds.y = std::numeric_limits<float>::lowest();
				minBounds.z = std::numeric_limits<float>::max();
				maxBounds.z = std::numeric_limits<float>::lowest();
				minBounds.w = 1.0f;
				maxBounds.w = 1.0f;

				for (int k = 0; k < 8; k++)
				{
					//XMFLOAT4 c4 = ToXMFLOAT4(frustum.corners[k], 1.0f);
					//XMVECTOR corner = XMLoadFloat4(&c4);
					XMVECTOR vW = vFrustumPoints[k];

					// Transform the frustum coordinate from view to world space
					vW = XMVector4Transform(vW,camInvView);

					// Transform the frustum coordinate from world to light space
					vW = XMVector4Transform(vW, lightViewMatrix);
					XMFLOAT3 tC;
					XMStoreFloat3(&tC, vW);

					minBounds.x = std::min(minBounds.x, tC.x);
					maxBounds.x = std::max(maxBounds.x, tC.x);
					minBounds.y = std::min(minBounds.y, tC.y);
					maxBounds.y = std::max(maxBounds.y, tC.y);
					minBounds.z = std::min(minBounds.z, tC.z);
					maxBounds.z = std::max(maxBounds.z, tC.z);
				}


				float biggestCascadeBound = 0;
				biggestCascadeBound = abs(minBounds.x) + abs(maxBounds.x) > biggestCascadeBound ? abs(minBounds.x) + abs(maxBounds.x) : biggestCascadeBound;
				biggestCascadeBound = abs(minBounds.y) + abs(maxBounds.y) > biggestCascadeBound ? abs(minBounds.y) + abs(maxBounds.y) : biggestCascadeBound;
				biggestCascadeBound = m_SplitNear[j] + m_SplitFar[j] > biggestCascadeBound ? m_SplitNear[j] + m_SplitFar[j] : biggestCascadeBound;

				frustum.RightSlope = maxBounds.x;
				frustum.LeftSlope = minBounds.x;
				frustum.BottomSlope = minBounds.y;
				frustum.TopSlope = maxBounds.y;
				frustum.Near = m_ShadowCamera->GetNear();
				frustum.Far = m_ShadowCamera->GetFar();
				//CreateFrustumPointsFromCascadeInterval(m_SplitNear[j], m_SplitFar[j], cameraProjectionMatrix, vFrustumPoints);
				
				XMMATRIX proj = XMMatrixOrthographicOffCenterLH(frustum.LeftSlope,frustum.RightSlope, frustum.BottomSlope, frustum.TopSlope, frustum.Near, frustum.Far);
				//XMMATRIX proj = XMMatrixOrthographicLH(biggestCascadeBound, biggestCascadeBound, 0.1f, 1000.0f);


				((float*)&m_CascadeLightBufferData.split0)[j] = m_SplitFar[j];
				XMStoreFloat4x4(&m_CascadeLightBufferData.dirLights[i].lightProjectionMatrix[j], proj);

				m_CascadeLightBufferData.dirLights[i].lightViewMatrix[j] = lightViewM;

				//DrawFrustum(frustum, proj, lightViewMatrix, camProjView,j);
			}
		}
		cam->SetNear(oldNear);
		cam->SetFar(oldFar);
	}
	void ShadowCascade::SetMultiSample(int num)
	{
		//have to limit it to x2 because it takes more than 8GB of VRAM to allocate a x4 multisampled texture;
		//num = num > 2 ? 2 : num;
		m_Multisample = num;
		for (size_t i = 0; i < MAX_CASCADES; i++)
		{
			if (m_CascadeShadows[i])
			{
				delete m_CascadeShadows[i];
				m_CascadeShadows[i] = nullptr;
			}
		}
		if (m_MiscShadows)
		{
			delete m_MiscShadows;
			m_MiscShadows = nullptr;
		}
		for (size_t i = 0; i < m_NumCascades; i++)
		{
			m_CascadeShadows[i] = new RenderTexture(ATLAS_RESOLUTION, ATLAS_RESOLUTION, RenderTexture::TextureType::DepthTex, m_Multisample);
		}
		m_MiscShadows = new RenderTexture(ATLAS_RESOLUTION, ATLAS_RESOLUTION, RenderTexture::TextureType::DepthTex, m_Multisample);
		if (m_Multisample == 1)
		{
			usedShadowMaterial = m_DirectionalShadowMaterial;
		}
		else
		{
			usedShadowMaterial = m_DirectionalShadowMultiSampleMaterial;
		}
	}

	void ShadowCascade::SetDirty()
	{
		m_DirtyLights = true;
		for (size_t i = 0; i < NUM_LIGHTS*3; i++)
		{
			m_Lights[i].LightIsDirty = true;
		}
	}

	void ShadowCascade::SetLightDirty(int type, int index)
	{
		m_DirtyLights = true;
		switch (type)
		{
		case 0: //directional light
			m_Lights[index].LightIsDirty = true;
			break;
		case 1: //point light
			m_Lights[index + 3].LightIsDirty = true;
			break;
		case 2: //spot light
			m_Lights[index + 6].LightIsDirty = true;
			break;
		}
	}

	void ShadowCascade::SetDirectionalLight(int index,bool enabled, XMFLOAT4 pos, XMFLOAT4 dir, XMFLOAT4 color)
	{
		m_DirtyLights = true;
		m_Lights[index].LightIsDirty = true;
		m_Lights[index].l->enabled = enabled;
		m_Lights[index].l->position = pos;
		((DirectionalLight*)m_Lights[index].l)->direction = dir;
		m_Lights[index].l->color = color;
	}

	//From XNA COLLISION MATH (SOURCE: DIRECTX11 SAMPLES (JUNE SDK) )

	//-----------------------------------------------------------------------------
	// Build a frustum from a persepective projection matrix.  The matrix may only
	// contain a projection; any rotation, translation or scale will cause the
	// constructed frustum to be incorrect.
	//-----------------------------------------------------------------------------
	void ShadowCascade::ComputeFrustumFromProjection(Frustum* pOut, XMMATRIX* pProjection)
	{
		// Corners of the projection frustum in homogenous space.
		static XMVECTOR HomogenousPoints[6] =
		{
			{ 1.0f,  0.0f, 1.0f, 1.0f },   // right (at far plane)
			{ -1.0f,  0.0f, 1.0f, 1.0f },   // left
			{ 0.0f,  1.0f, 1.0f, 1.0f },   // top
			{ 0.0f, -1.0f, 1.0f, 1.0f },   // bottom

			{ 0.0f, 0.0f, 0.0f, 1.0f },     // near
			{ 0.0f, 0.0f, 1.0f, 1.0f }      // far
		};

		XMVECTOR Determinant;
		XMMATRIX matInverse = XMMatrixInverse(&Determinant, *pProjection);

		// Compute the frustum corners in world space.
		XMVECTOR Points[6];

		for (INT i = 0; i < 6; i++)
		{
			// Transform point.
			Points[i] = XMVector4Transform(HomogenousPoints[i], matInverse);
		}
		
		// Compute the slopes.
		Points[0] = XMVectorMultiply(Points[0],XMVectorReciprocal(XMVectorSplatZ(Points[0])));
		Points[1] = XMVectorMultiply(Points[1],XMVectorReciprocal(XMVectorSplatZ(Points[1])));
		Points[2] = XMVectorMultiply(Points[2],XMVectorReciprocal(XMVectorSplatZ(Points[2])));
		Points[3] = XMVectorMultiply(Points[3],XMVectorReciprocal(XMVectorSplatZ(Points[3])));

		pOut->RightSlope = XMVectorGetX(Points[0]);
		pOut->LeftSlope = XMVectorGetX(Points[1]);
		pOut->TopSlope = XMVectorGetY(Points[2]);
		pOut->BottomSlope = XMVectorGetY(Points[3]);

		// Compute near and far.
		Points[4] =XMVectorMultiply(Points[4],XMVectorReciprocal(XMVectorSplatW(Points[4])));
		Points[5] =XMVectorMultiply(Points[5],XMVectorReciprocal(XMVectorSplatW(Points[5])));

		pOut->Near = XMVectorGetZ(Points[4]);
		pOut->Far = XMVectorGetZ(Points[5]);

		return;
	}

	//FROM DIRECTX11 SAMPLES (JUNE SDK)

	//--------------------------------------------------------------------------------------
	// This function takes the camera's projection matrix and returns the 8
	// points that make up a view frustum.
	// The frustum is scaled to fit within the Begin and End interval paramaters.
	//--------------------------------------------------------------------------------------
	void ShadowCascade::CreateFrustumPointsFromCascadeInterval(float fnear,float ffar,XMMATRIX &vProjection,Frustum& outFrustum, XMVECTOR* pvCornerPointsWorld)
	{
		ComputeFrustumFromProjection(&outFrustum, &vProjection);
		outFrustum.Near = fnear;
		outFrustum.Far = ffar;

		static const XMVECTORU32 vGrabY = { 0x00000000,0xFFFFFFFF,0x00000000,0x00000000 };
		static const XMVECTORU32 vGrabX = { 0xFFFFFFFF,0x00000000,0x00000000,0x00000000 };

		XMVECTORF32 vRightTop = { outFrustum.RightSlope,outFrustum.TopSlope,1.0f,1.0f };
		XMVECTORF32 vLeftBottom = { outFrustum.LeftSlope,outFrustum.BottomSlope,1.0f,1.0f };
		XMVECTORF32 vNear = { outFrustum.Near,outFrustum.Near,outFrustum.Near,1.0f };
		XMVECTORF32 vFar = { outFrustum.Far,outFrustum.Far,outFrustum.Far,1.0f };
		XMVECTOR vRightTopNear = XMVectorMultiply(vRightTop, vNear);
		XMVECTOR vRightTopFar = XMVectorMultiply(vRightTop, vFar);
		XMVECTOR vLeftBottomNear = XMVectorMultiply(vLeftBottom, vNear);
		XMVECTOR vLeftBottomFar = XMVectorMultiply(vLeftBottom, vFar);

		pvCornerPointsWorld[0] = vRightTopNear;
		pvCornerPointsWorld[1] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabX);
		pvCornerPointsWorld[2] = vLeftBottomNear;
		pvCornerPointsWorld[3] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabY);

		pvCornerPointsWorld[4] = vRightTopFar;
		pvCornerPointsWorld[5] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabX);
		pvCornerPointsWorld[6] = vLeftBottomFar;
		pvCornerPointsWorld[7] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabY);
	}
	void ShadowCascade::CalculateFrustumPoints(Camera* cam, Frustum* outFrustum)
	{
		//https://gamedev.stackexchange.com/questions/19774/determine-corners-of-a-specific-plane-in-the-frustum
		XMFLOAT3 v = cam->GetForward();
		XMFLOAT3 right = cam->GetRight();
		XMFLOAT3 P = cam->GetPosition();
		XMFLOAT3 up = cam->GetUp();
		float fov = cam->GetFovRadians();
		float nDis = cam->GetNear();
		float fDis = cam->GetFar();
		float ar = cam->GetAspectRatio();

		float Hnear = 2.0f * tan(fov / 2.0f) * nDis;
		float Wnear = Hnear * ar;
		float Hfar = 2.0f * tan(fov / 2.0f) * fDis;
		float Wfar = Hfar * ar;
		XMFLOAT3 Cnear = P + v * nDis;
		XMFLOAT3 Cfar = P + v * fDis;
		
		outFrustum->corners[0] = Cnear + (up * (Hnear / 2.0f)) - (right * (Wnear / 2.0f));//Near Top Left		
		outFrustum->corners[1] = Cnear + (up * (Hnear / 2.0f)) + (right * (Wnear / 2.0f));//Near Top Right		
		outFrustum->corners[2] = Cnear - (up * (Hnear / 2.0f)) - (right * (Wnear / 2.0f));//Near Bottom Left	
		outFrustum->corners[3] = Cnear + (up * (Hnear / 2.0f)) + (right * (Wnear / 2.0f));//Near Bottom Right	
		outFrustum->corners[4] = Cfar + (up * (Hfar / 2.0f)) - (right * Wfar / 2.0f);//Far Top Left		
		outFrustum->corners[5] = Cfar + (up * (Hfar / 2.0f)) + (right * Wfar / 2.0f);//Far Top Right		
		outFrustum->corners[6] = Cfar - (up * (Hfar / 2.0f)) - (right * Wfar / 2.0f);//Far Bottom Left		
		outFrustum->corners[7] = Cfar - (up * (Hfar / 2.0f)) + (right * Wfar / 2.0f);//Far Bottom Right	
	}
}