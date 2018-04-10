#include "ThempSystem.h"
#include "ThempD3D.h"
#include "ThempShadowAtlas.h"
#include <DirectXMath.h>
namespace Themp
{
	ShadowAtlas::ShadowAtlas(int size)
	{
		ID3D11Device* dev = Themp::System::tSys->m_D3D->m_Device;
		m_Size = size;
		m_Base = new MapNode(size);
		m_Base->n_Size = size;
		m_Base->n[0] = new MapNode(size/2);
		m_Base->n[1] = new MapNode(size/2);
		m_Base->n[2] = new MapNode(size/2);
		m_Base->n[3] = new MapNode(size/2);

		HRESULT result;

		SetMultiSample(1);
	}
	
	const DirectX::XMFLOAT2 offsetTable[4] = { XMFLOAT2(0,0), XMFLOAT2(1,0), XMFLOAT2(0,1), XMFLOAT2(1,1) };
	
	XMFLOAT3 ShadowAtlas::FindSmallestFit(MapNode* n, int size, XMFLOAT3 currentOffset)
	{
		//check if it'd fit in a potential child
		if (size > n->n_Size >> 1)
		{
			//it doesn't fit in a child, we check if there has been atleast a single child used.
			if (n->n[0] != nullptr) 
			{
				//implies a child has been made and thus been used before (guaranteed to have a taken child in the tree down)
				//so we pick a different node.
				return currentOffset;
			}
			//if we got here, we didn't have any used childs and we can use this entire node
			n->taken = true;
			currentOffset.z = 1.0;
			return currentOffset;
		}
		//otherwise check childs for a fit
		for (size_t i = 0; i < 4; i++)
		{
			if (n->n[i] == nullptr) n->n[i] = new MapNode(n->n_Size >> 1);
			if (!n->n[i]->taken && n->n[i]->n_Size >= size)
			{
				XMFLOAT3 offset = currentOffset;
				offset.x += offsetTable[i].x * (n->n_Size>>1);
				offset.y += offsetTable[i].y * (n->n_Size>>1);
				offset = FindSmallestFit(n->n[i],size,offset);
				if (offset.z != 0.0)
				{
					return offset;
				}
			}
		}
		//doesn't fit anywhere here..
		return currentOffset;
	}
	XMFLOAT4 ShadowAtlas::ObtainTextureArea(int size)
	{
		XMFLOAT3 offset = XMFLOAT3(0,0,0);
		offset = FindSmallestFit(m_Base, size,offset);
		assert(offset.z != 0);
		return XMFLOAT4(offset.x, offset.y, (float)size, (float)size);
	}
	bool ShadowAtlas::SetMultiSample(int num)
	{
		if (num == 0) num = 1;

		ID3D11Device* dev = Themp::System::tSys->m_D3D->m_Device;
		CLEAN(m_ShaderResourceView);
		CLEAN(m_DepthStencilView);
		CLEAN(m_ShadowMap);

		HRESULT hr;

		D3D11_TEXTURE2D_DESC depthBufferDesc;
		ZeroMemory(&depthBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));
		depthBufferDesc.SampleDesc.Count = num;
		depthBufferDesc.SampleDesc.Quality = 0;

		depthBufferDesc.Width = m_Size;
		depthBufferDesc.Height = m_Size;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.ArraySize = 1;
		depthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS; // DXGI_FORMAT_D16_UNORM;
		depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		depthBufferDesc.CPUAccessFlags = 0;
		depthBufferDesc.MiscFlags = 0;

		hr = dev->CreateTexture2D(&depthBufferDesc, NULL, &m_ShadowMap);
		if (hr != S_OK)
		{
			System::Print("failed to create depth texture");
			return false;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		memset(&descDSV, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
		descDSV.Format = DXGI_FORMAT_D32_FLOAT;
		descDSV.ViewDimension = (num == 1 ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DMS);
		descDSV.Texture2D.MipSlice = 0;


		// Create the depth stencil view
		hr = dev->CreateDepthStencilView(m_ShadowMap, // Depth stencil texture
			&descDSV, // Depth stencil desc
			&m_DepthStencilView);  // [out] Depth stencil view
		if (hr != S_OK)
		{
			System::Print("Could not create Depthstencil view");
			return false;
		}
		D3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc;
		memset(&depthSRVDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		depthSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
		depthSRVDesc.Texture2D.MipLevels = 1;
		depthSRVDesc.Texture2D.MostDetailedMip = 0;
		depthSRVDesc.ViewDimension = (num == 1 ? D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D : D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2DMS);

		hr = dev->CreateShaderResourceView(m_ShadowMap, // Depth stencil texture
			&depthSRVDesc, // Depth stencil desc
			&m_ShaderResourceView);  // [out] Depth stencil view
		if (hr != S_OK)
		{
			System::Print("Could not create Depthstencil shader resource view");
			return false;
		}
		return true;
	}
	ShadowAtlas::~ShadowAtlas()
	{
		if (m_ShadowMap)
		{
			m_ShadowMap->Release();
			m_ShadowMap = nullptr;
		}
		if (m_ShaderResourceView)
		{
			m_ShaderResourceView->Release();
			m_ShaderResourceView = nullptr;
		}
		if (m_DepthStencilView)
		{
			m_DepthStencilView->Release();
			m_DepthStencilView = nullptr;
		}
	}
}