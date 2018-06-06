#include "ThempSystem.h"
#include "ThempShadowUnfiltered.h"
#include "ThempD3D.h"
#include "ThempGame.h"
#include "ThempObject3D.h"
#include "ThempMesh.h"
#include "ThempMaterial.h"
#include "ThempResources.h"
#include "ThempRenderTexture.h"
#include "ThempShadowAtlas.h"
#include "../ThempCamera.h"

#include <DirectXMath.h>
#include <iostream>

using namespace DirectX;
namespace Themp
{
	ShadowUnfiltered::ShadowUnfiltered()
	{
		m_ShadowCamera = new Camera();
		m_ShadowCamera->SetOrtho(400, 400);
		m_ShadowAtlas = new ShadowAtlas(ATLAS_RESOLUTION);
		SetMultiSample(1);
		D3D::s_D3D->m_DevCon->ClearDepthStencilView(m_ShadowTexture->m_DepthStencilView, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.0f, 0);

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
		D3D::s_D3D->m_Device->CreateBuffer(&cbDesc, &InitData, &m_LightBuffer);

		m_UnfilteredLightingMaterial = Resources::TRes->GetMaterial("DeferredLightingUnfiltered", "", "DeferredLightingPCF", false);
		if (D3D::s_D3D->SupportsVPArrayIndex)
		{
			m_ShadowMaterial = Resources::TRes->GetMaterial("DeferredShadow", "", "DeferredShadow", true);
		}
		else
		{
			m_ShadowMaterial = Resources::TRes->GetMaterial("DeferredShadowFallback", "", "DeferredShadowFallback", false);
		}
		m_ShadowClearMaterial = Resources::TRes->GetMaterial("ScreenSpace", "", "ScreenSpace", false);
		m_LightConstantBufferData.numDir = NUM_LIGHTS;
		for (size_t i = 0; i < NUM_LIGHTS; i++)
		{
			m_ShadowMaps[i].l = &m_LightConstantBufferData.dirLights[i];
			m_ShadowMaps[i].LightIsDirty = true;
			m_DirtyLights = true;
			m_LightConstantBufferData.dirLights[i].enabled = false;
			m_LightConstantBufferData.dirLights[i].textureOffset = m_ShadowAtlas->ObtainTextureArea(SHADOW_RESOLUTION);
		}
	}
	ShadowUnfiltered::~ShadowUnfiltered()
	{
		if (m_LightBuffer)
		{
			m_LightBuffer->Release();
			m_LightBuffer = nullptr;
		}
		delete m_ShadowAtlas;
		delete m_ShadowTexture;
		delete m_ShadowCamera;
	}
	void ShadowUnfiltered::PreDraw()
	{
		if (m_DirtyLights)
		{
			D3D* l_D3D = D3D::s_D3D;
			D3D11_MAPPED_SUBRESOURCE ms;
			l_D3D->m_DevCon->Map(m_LightBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
			memcpy(ms.pData, &m_LightConstantBufferData, sizeof(LightConstantBuffer));
			l_D3D->m_DevCon->Unmap(m_LightBuffer, NULL);
		}
	}
	void ShadowUnfiltered::SetDirty()
	{
		m_DirtyLights = true;
		for (size_t i = 0; i < NUM_LIGHTS * 3; i++)
		{
			m_ShadowMaps[i].LightIsDirty = true;
		}
	}
	void ShadowUnfiltered::SetMultiSample(int num)
	{
		num = 1;
		if (m_ShadowTexture)
		{
			delete m_ShadowTexture;
			m_ShadowTexture = nullptr;
		}
		m_ShadowTexture = new RenderTexture(ATLAS_RESOLUTION, ATLAS_RESOLUTION, RenderTexture::TextureType::DepthTex, num);
	}

	void ShadowUnfiltered::SetLightDirty(int type, int index)
	{
		m_DirtyLights = true;
		switch (type)
		{
		case 0: //directional light
			m_ShadowMaps[index].LightIsDirty = true;
			break;
		case 1: //point light
			m_ShadowMaps[index+3].LightIsDirty = true;
			break;
		case 2: //spot light
			m_ShadowMaps[index+6].LightIsDirty = true;
			break;
		}
	}
	void ShadowUnfiltered::SetDirectionalLight(int index, bool enabled, XMFLOAT4 pos, XMFLOAT4 dir, XMFLOAT4 color)
	{
		m_DirtyLights = true;
		m_ShadowMaps[index].LightIsDirty = true;
		m_LightConstantBufferData.dirLights[index].enabled = enabled;
		m_LightConstantBufferData.dirLights[index].position = pos;
		m_LightConstantBufferData.dirLights[index].direction = dir;
		m_LightConstantBufferData.dirLights[index].color = color;
	}
	void ShadowUnfiltered::DrawShadow()
	{
		D3D* l_D3D = D3D::s_D3D;

		PreDraw();

		l_D3D->m_DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		l_D3D->m_DevCon->IASetInputLayout(D3D::DefaultMaterial->m_InputLayout);

		l_D3D->m_DevCon->RSSetState(l_D3D->m_RasterizerState);
		l_D3D->m_DevCon->OMSetRenderTargets(0, nullptr, m_ShadowTexture->m_DepthStencilView);
		DrawDirectionalShadow();
		DrawPointShadow();
		DrawSpotShadow();
	}
	void ShadowUnfiltered::DrawLight()
	{
		D3D* l_D3D = D3D::s_D3D;
		l_D3D->m_DevCon->RSSetState(l_D3D->m_RasterizerState);
		l_D3D->m_DevCon->PSSetShader(m_UnfilteredLightingMaterial->m_PixelShader, 0, 0);
		l_D3D->m_DevCon->VSSetShader(m_UnfilteredLightingMaterial->m_VertexShader, 0, 0);
		l_D3D->m_DevCon->GSSetShader(m_UnfilteredLightingMaterial->m_GeometryShader, 0, 0);

		l_D3D->SetViewPort(0, 0, l_D3D->m_ConstantBufferData.screenWidth, l_D3D->m_ConstantBufferData.screenHeight);
		//m_DevCon->OMSetRenderTargets(1, &m_BackBuffer, m_DepthStencilView);
		l_D3D->m_DevCon->OMSetRenderTargets(1, &l_D3D->m_MainRender->m_RenderTarget, nullptr);
		l_D3D->m_DevCon->PSSetSamplers(0, 4, m_UnfilteredLightingMaterial->m_SamplerStates);

		for (size_t i = 0; i < NUM_RENDER_TEXTURES; i++)
		{
			l_D3D->m_ShaderResourceViews[i] = l_D3D->m_RenderTextures[i]->m_ShaderResourceView;
		}
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES] = l_D3D->m_DepthStencilSRV;
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 1] = m_ShadowTexture->m_ShaderResourceView;
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 2] = l_D3D->DefaultMaterialSkybox->m_Textures[0]->m_View;
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 3] = l_D3D->DefaultMaterialSkybox->m_Textures[1]->m_View;
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 4] = l_D3D->DefaultMaterialSkybox->m_Textures[2]->m_View;
		l_D3D->m_ShaderResourceViews[NUM_RENDER_TEXTURES + 5] = l_D3D->DefaultMaterialSkybox->m_Textures[3]->m_View;

		l_D3D->m_DevCon->PSSetShaderResources(0, NUM_SHADER_RESOURCE_VIEWS, l_D3D->m_ShaderResourceViews);


		// draw fullscreen quad, manually
		uint32_t stride[] = { sizeof(Vertex) };
		uint32_t offset[] = { 0 };
		l_D3D->m_DevCon->IASetVertexBuffers(0, 1, &l_D3D->m_FullScreenQuad->m_Meshes[0]->m_VertexBuffer, stride, offset);
		l_D3D->m_DevCon->IASetIndexBuffer(l_D3D->m_FullScreenQuad->m_Meshes[0]->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		l_D3D->m_DevCon->IASetInputLayout(m_UnfilteredLightingMaterial->m_InputLayout);

		l_D3D->m_DevCon->PSSetSamplers(0, 4, m_UnfilteredLightingMaterial->m_SamplerStates);
		l_D3D->SetSystemConstantBuffer(l_D3D->m_CBuffer);
		Themp::System::tSys->m_Game->m_Camera->UpdateMatrices();
		l_D3D->SetLightConstantBuffer(m_LightBuffer);
		l_D3D->PSUploadConstantBuffersToGPU();

		l_D3D->m_DevCon->DrawIndexed(l_D3D->m_FullScreenQuad->m_Meshes[0]->m_NumIndices, 0, 0);
	}
	void ShadowUnfiltered::DrawDirectionalShadow()
	{
		D3D* l_D3D = D3D::s_D3D;
		Game* l_Game = Themp::System::tSys->m_Game;
		//ConstantBuffers:
		//0 = SystemBuffer
		//1 = ObjectBuffer
		//2 = LightBuffer
		D3D::ConstantBuffers[2] = m_LightBuffer;

		for (size_t i = 0; i < m_LightConstantBufferData.numDir; i++)
		{
			LightShadowMap* s = &m_ShadowMaps[i];
			if (s->LightIsDirty && s->l->enabled)
			{
				DirectionalLight* l = &m_LightConstantBufferData.dirLights[i];

				//clear light depth buffer data
				l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_ShadowClearDepthStencilState, 1);
				l_D3D->m_DevCon->PSSetShader(m_ShadowClearMaterial->m_PixelShader, 0, 0);
				l_D3D->m_DevCon->VSSetShader(m_ShadowClearMaterial->m_VertexShader, 0, 0);
				l_D3D->m_DevCon->GSSetShader(nullptr, 0, 0);
				l_D3D->SetViewPort(l->textureOffset.x, l->textureOffset.y, l->textureOffset.z, l->textureOffset.w);
				l_D3D->m_FullScreenQuad->Draw(*l_D3D, Mesh::DrawPass::SHADOW);

				//restore shader
				l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_DepthStencilState, 1);
				l_D3D->m_DevCon->RSSetState(l_D3D->m_ShadowRasterizerState);

				l_D3D->m_DevCon->PSSetShader(m_ShadowMaterial->m_PixelShader, 0, 0);
				l_D3D->m_DevCon->VSSetShader(m_ShadowMaterial->m_VertexShader, 0, 0);
				l_D3D->m_DevCon->GSSetShader(nullptr, 0, 0);

				RenderShadowsDirectionalCamera(l);

				for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
				{
					l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
				}
				s->LightIsDirty = false;
			}
		}
	}
	void ShadowUnfiltered::DrawPointShadow()
	{
		D3D* l_D3D = D3D::s_D3D;
		Game* l_Game = Themp::System::tSys->m_Game;
		l_D3D->m_DevCon->RSSetState(l_D3D->m_RasterizerState);
		//clear point lights if needed
		for (int i = 0; i < m_LightConstantBufferData.numPoint; i++)
		{
			int shadowMapIndex = m_LightConstantBufferData.numDir + i;
			LightShadowMap* s = &m_ShadowMaps[shadowMapIndex];
			if (s->LightIsDirty && s->l->enabled)
			{
				PointLight* l = &m_LightConstantBufferData.pointLights[i];
				D3D11_VIEWPORT vps[6];

				//clear depth buffer setup
				l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_ShadowClearDepthStencilState, 1);
				l_D3D->m_DevCon->PSSetShader(m_ShadowClearMaterial->m_PixelShader, 0, 0);
				l_D3D->m_DevCon->VSSetShader(m_ShadowClearMaterial->m_VertexShader, 0, 0);
				l_D3D->m_DevCon->GSSetShader(nullptr, 0, 0);
				for (int j = 0; j < 6; j++)
				{
					//set viewports
					vps[j].TopLeftX = l->textureOffset[j].x;
					vps[j].TopLeftY = l->textureOffset[j].y;
					vps[j].Width = l->textureOffset[j].z;
					vps[j].Height = l->textureOffset[j].w;
					vps[j].MinDepth = 0.0f;
					vps[j].MaxDepth = 1.0f;

					//clear current viewport
					l_D3D->SetViewPort(l->textureOffset[j].x, l->textureOffset[j].y, l->textureOffset[j].z, l->textureOffset[j].w);
					l_D3D->m_FullScreenQuad->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
				}
			}
		}

		//restore proper shadow shader and depthstencilstate
		l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_DepthStencilState, 1);
		l_D3D->m_DevCon->RSSetState(l_D3D->m_ShadowRasterizerState);

		//we use the geometry shader which has the light buffer bound to its regular slot so we use this instead
		l_D3D->SetLightConstantBuffer(m_LightBuffer);
		l_D3D->PSUploadConstantBuffersToGPU();
		if (l_D3D->SupportsVPArrayIndex) //supports single pass render to cube map
		{
			l_D3D->GSUploadConstantBuffersToGPU();
			for (int i = 0; i < m_LightConstantBufferData.numPoint; i++)
			{
				int shadowMapIndex = m_LightConstantBufferData.numDir + i;
				LightShadowMap* s = &m_ShadowMaps[shadowMapIndex];
				if (s->LightIsDirty && s->l->enabled)
				{
					PointLight* l = &m_LightConstantBufferData.pointLights[i];
					D3D11_VIEWPORT vps[6];
					for (int j = 0; j < 6; j++)
					{
						//set viewports
						vps[j].TopLeftX = l->textureOffset[j].x;
						vps[j].TopLeftY = l->textureOffset[j].y;
						vps[j].Width = l->textureOffset[j].z;
						vps[j].Height = l->textureOffset[j].w;
						vps[j].MinDepth = 0.0f;
						vps[j].MaxDepth = 1.0f;
					}
					l_D3D->m_DevCon->RSSetViewports(6, &vps[0]);
					l_D3D->m_DevCon->PSSetShader(m_ShadowMaterial->m_PixelShader, 0, 0);
					l_D3D->m_DevCon->VSSetShader(m_ShadowMaterial->m_VertexShader, 0, 0);
					l_D3D->m_DevCon->GSSetShader(m_ShadowMaterial->m_GeometryShader, 0, 0);
					RenderShadowsPointCamera(l);

					for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
					{
						l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
					}
					s->LightIsDirty = false;
				}
			}
			l_D3D->GSUploadConstantBuffersToGPUNull();
		}
		else //slow point light rendering path due to no support
		{
			for (int i = 0; i < m_LightConstantBufferData.numPoint; i++)
			{
				int shadowMapIndex = m_LightConstantBufferData.numDir + i;
				LightShadowMap* s = &m_ShadowMaps[shadowMapIndex];
				if (s->LightIsDirty && s->l->enabled)
				{
					PointLight* l = &m_LightConstantBufferData.pointLights[i];
					D3D11_VIEWPORT vp;

					l_D3D->m_DevCon->PSSetShader(m_ShadowMaterial->m_PixelShader, 0, 0);
					l_D3D->m_DevCon->VSSetShader(m_ShadowMaterial->m_VertexShader, 0, 0);
					l_D3D->m_DevCon->GSSetShader(0, 0, 0);

					//set viewports
					vp.MinDepth = 0.0f;
					vp.MaxDepth = 1.0f;

					m_ShadowCamera->SetPosition(XMFLOAT3(l->position.x, l->position.y, l->position.z));
					m_ShadowCamera->SetProjection(Camera::CameraType::Perspective);
					m_ShadowCamera->isDirty = true;

					////////////// Face 1
					m_ShadowCamera->SetLookTo(XMFLOAT3(-1, 0, 0), XMFLOAT3(0, 1, 0), XMFLOAT3(0, 0, 1));
					m_ShadowCamera->UpdateMatrices();
					m_ShadowCamera->isDirty = true;
					l->lightviewmatrix[0] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;

					vp.TopLeftX = l->textureOffset[0].x;
					vp.TopLeftY = l->textureOffset[0].y;
					vp.Width = l->textureOffset[0].z;
					vp.Height = l->textureOffset[0].w;
					l_D3D->SetViewPort(l->textureOffset[0].x, l->textureOffset[0].y, l->textureOffset[0].z, l->textureOffset[0].w);

					for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
					{
						l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
					}
					//////////////

					///////////// Face 2
					m_ShadowCamera->SetLookTo(XMFLOAT3(-1, 0, 0), XMFLOAT3(0, 1, 0), XMFLOAT3(0, 0, -1));
					m_ShadowCamera->UpdateMatrices();
					m_ShadowCamera->isDirty = true;
					l->lightviewmatrix[1] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
					vp.TopLeftX = l->textureOffset[1].x;
					vp.TopLeftY = l->textureOffset[1].y;
					vp.Width = l->textureOffset[1].z;
					vp.Height = l->textureOffset[1].w;
					l_D3D->SetViewPort(l->textureOffset[1].x, l->textureOffset[1].y, l->textureOffset[1].z, l->textureOffset[1].w);

					for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
					{
						l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
					}
					//////////////

					////////////// Face 3
					m_ShadowCamera->SetLookTo(XMFLOAT3(0, 1, 0), XMFLOAT3(1, 0, 0));
					m_ShadowCamera->UpdateMatrices();
					m_ShadowCamera->isDirty = true;
					l->lightviewmatrix[2] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
					vp.TopLeftX = l->textureOffset[2].x;
					vp.TopLeftY = l->textureOffset[2].y;
					vp.Width = l->textureOffset[2].z;
					vp.Height = l->textureOffset[2].w;
					l_D3D->SetViewPort(l->textureOffset[2].x, l->textureOffset[2].y, l->textureOffset[2].z, l->textureOffset[2].w);

					for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
					{
						l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
					}
					//////////////

					////////////// Face 4
					m_ShadowCamera->SetLookTo(XMFLOAT3(0, -1, 0), XMFLOAT3(1, 0, 0));
					m_ShadowCamera->UpdateMatrices();
					m_ShadowCamera->isDirty = true;
					l->lightviewmatrix[3] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
					vp.TopLeftX = l->textureOffset[3].x;
					vp.TopLeftY = l->textureOffset[3].y;
					vp.Width = l->textureOffset[3].z;
					vp.Height = l->textureOffset[3].w;
					l_D3D->SetViewPort(l->textureOffset[3].x, l->textureOffset[3].y, l->textureOffset[3].z, l->textureOffset[3].w);

					for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
					{
						l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
					}
					//////////////

					////////////// Face 5
					m_ShadowCamera->SetLookTo(XMFLOAT3(0, 0, 1), XMFLOAT3(0, 1, 0));
					m_ShadowCamera->UpdateMatrices();
					m_ShadowCamera->isDirty = true;
					l->lightviewmatrix[4] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
					vp.TopLeftX = l->textureOffset[4].x;
					vp.TopLeftY = l->textureOffset[4].y;
					vp.Width = l->textureOffset[4].z;
					vp.Height = l->textureOffset[4].w;
					l_D3D->SetViewPort(l->textureOffset[4].x, l->textureOffset[4].y, l->textureOffset[4].z, l->textureOffset[4].w);

					for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
					{
						l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
					}
					//////////////

					////////////// Face 6
					m_ShadowCamera->SetLookTo(XMFLOAT3(0, 0, -1), XMFLOAT3(0, 1, 0));
					m_ShadowCamera->UpdateMatrices();
					l->lightviewmatrix[5] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
					vp.TopLeftX = l->textureOffset[5].x;
					vp.TopLeftY = l->textureOffset[5].y;
					vp.Width = l->textureOffset[5].z;
					vp.Height = l->textureOffset[5].w;
					l_D3D->SetViewPort(l->textureOffset[5].x, l->textureOffset[5].y, l->textureOffset[5].z, l->textureOffset[5].w);

					for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
					{
						l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
					}
					//////////////
					s->LightIsDirty = false;
				}
			}
		}
	}
	void ShadowUnfiltered::DrawSpotShadow()
	{
		D3D* l_D3D = D3D::s_D3D;
		Game* l_Game = Themp::System::tSys->m_Game;
		for (size_t i = 0; i < m_LightConstantBufferData.numSpot; i++)
		{
			int shadowMapIndex = m_LightConstantBufferData.numPoint + m_LightConstantBufferData.numDir + i;
			LightShadowMap* s = &m_ShadowMaps[shadowMapIndex];
			if (s->LightIsDirty && s->l->enabled)
			{
				SpotLight* l = &m_LightConstantBufferData.spotLights[i];

				//clear current light data
				l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_ShadowClearDepthStencilState, 1);
				l_D3D->m_DevCon->RSSetState(l_D3D->m_RasterizerState);
				l_D3D->m_DevCon->PSSetShader(m_ShadowClearMaterial->m_PixelShader, 0, 0);
				l_D3D->m_DevCon->VSSetShader(m_ShadowClearMaterial->m_VertexShader, 0, 0);
				l_D3D->m_DevCon->GSSetShader(nullptr, 0, 0);
				l_D3D->SetViewPort(l->textureOffset.x, l->textureOffset.y, l->textureOffset.z, l->textureOffset.w);
				l_D3D->m_FullScreenQuad->Draw(*l_D3D, Mesh::DrawPass::SHADOW);

				//restore shadow rendering and draw
				l_D3D->m_DevCon->OMSetDepthStencilState(l_D3D->m_DepthStencilState, 1);
				l_D3D->m_DevCon->RSSetState(l_D3D->m_ShadowRasterizerState);

				l_D3D->m_DevCon->PSSetShader(m_ShadowMaterial->m_PixelShader, 0, 0);
				l_D3D->m_DevCon->VSSetShader(m_ShadowMaterial->m_VertexShader, 0, 0);
				l_D3D->m_DevCon->GSSetShader(nullptr, 0, 0);

				l_D3D->SetLightConstantBuffer(m_LightBuffer);
				l_D3D->PSUploadConstantBuffersToGPU();

				l_D3D->SetViewPort(l->textureOffset.x, l->textureOffset.y, l->textureOffset.z, l->textureOffset.w);
				RenderShadowsSpotCamera(l);

				for (int j = 0; j < l_Game->m_Objects3D.size(); ++j)
				{
					l_Game->m_Objects3D[j]->Draw(*l_D3D, Mesh::DrawPass::SHADOW);
				}
				s->LightIsDirty = false;
			}
		}
	}

	void ShadowUnfiltered::RenderShadowsPointCamera(PointLight* l)
	{
		m_ShadowCamera->isDirty = true;
		m_ShadowCamera->SetProjection(Camera::CameraType::Perspective);
		m_ShadowCamera->SetPosition(l->position.x, l->position.y, l->position.z);
		XMFLOAT3 lookDir = XMFLOAT3(1, 0, 0);
		XMFLOAT3 up = XMFLOAT3(0, 1, 0);
		XMFLOAT3 right = XMFLOAT3(0, 0, 1);
		//face 1 +X
		m_ShadowCamera->SetLookTo(lookDir, up, right);
		m_ShadowCamera->UpdateMatrices();
		l->lightviewmatrix[0] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
		//face 2 -X
		lookDir = XMFLOAT3(-1, 0, 0);
		right = XMFLOAT3(0, 0, -1);
		m_ShadowCamera->SetLookTo(lookDir, up, right);
		m_ShadowCamera->UpdateMatrices();
		l->lightviewmatrix[1] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
		//face 3 +Y
		lookDir = XMFLOAT3(0, 1, 0);
		up = XMFLOAT3(1, 0, 0);
		right = XMFLOAT3(0, 0, -1);
		m_ShadowCamera->SetLookTo(lookDir, up, right);
		m_ShadowCamera->UpdateMatrices();
		l->lightviewmatrix[2] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
		//face 4 -Y
		lookDir = XMFLOAT3(0, -1, 0);
		up = XMFLOAT3(1, 0, 0);
		right = XMFLOAT3(0, 0, 1);
		m_ShadowCamera->SetLookTo(lookDir, up, right);
		m_ShadowCamera->UpdateMatrices();
		l->lightviewmatrix[3] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
		//face 5 +Z
		lookDir = XMFLOAT3(0, 0, 1);
		up = XMFLOAT3(0, 1, 0);
		right = XMFLOAT3(1, 0, 0);
		m_ShadowCamera->SetLookTo(lookDir, up, right);
		m_ShadowCamera->UpdateMatrices();
		l->lightviewmatrix[4] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
		//face 6 -Z
		lookDir = XMFLOAT3(0, 0, -1);
		up = XMFLOAT3(0, 1, 0);
		right = XMFLOAT3(-1, 0, 0);
		m_ShadowCamera->SetLookTo(lookDir, up, right);
		m_ShadowCamera->UpdateMatrices();
		l->lightviewmatrix[5] = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
	}
	void ShadowUnfiltered::RenderShadowsSpotCamera(SpotLight* l)
	{
		m_ShadowCamera->SetPosition(XMFLOAT3(l->position.x, l->position.y, l->position.z));
		m_ShadowCamera->SetProjection(Camera::CameraType::Perspective);
		m_ShadowCamera->SetLookTo(XMFLOAT3(l->direction.x, l->direction.y, l->direction.z), XMFLOAT3(0, 1, 0));
		m_ShadowCamera->UpdateMatrices();
		l->lightprojmatrix = m_ShadowCamera->m_CameraConstantBufferData.projectionMatrix;
		l->lightviewmatrix = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
	}
	void ShadowUnfiltered::RenderShadowsDirectionalCamera(DirectionalLight* l)
	{
		m_ShadowCamera->SetPosition(l->position.x, l->position.y, l->position.z);
		m_ShadowCamera->SetProjection(Camera::CameraType::Orthographic);
		m_ShadowCamera->SetLookTo(XMFLOAT3(l->direction.x, l->direction.y, l->direction.z), XMFLOAT3(0, 1, 0));
		m_ShadowCamera->UpdateMatrices();
		l->lightprojmatrix = m_ShadowCamera->m_CameraConstantBufferData.projectionMatrix;
		l->lightviewmatrix = m_ShadowCamera->m_CameraConstantBufferData.viewMatrix;
	}
}