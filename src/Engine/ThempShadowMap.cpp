#include "ThempSystem.h"
#include "ThempD3D.h"
#include "ThempShadowMap.h"
#include <DirectXMath.h>
namespace Themp
{
	ShadowMap::ShadowMap(int size)
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

		D3D11_TEXTURE2D_DESC depthBufferDesc;
		D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
		D3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc;
		memset(&depthBufferDesc, 0, sizeof(D3D11_TEXTURE2D_DESC));
		memset(&descDSV, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
		memset(&depthSRVDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		depthBufferDesc.SampleDesc.Count = 1;
		depthBufferDesc.SampleDesc.Quality = 0;

		depthBufferDesc.Width = m_Size;
		depthBufferDesc.Height = m_Size;
		depthBufferDesc.MipLevels = 1;

		depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

		depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // DXGI_FORMAT_D16_UNORM;
		depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		depthBufferDesc.CPUAccessFlags = 0;
		depthBufferDesc.ArraySize = 1;
		depthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		
		result = dev->CreateTexture2D(&depthBufferDesc, NULL, &m_ShadowMap);
		if (result != S_OK)
		{
			printf("Could not create Depthstencil texture");
			return;
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
		result = dev->CreateDepthStencilState(&dsDesc, &m_DeptStencilState);
		if (result != S_OK)
		{
			printf("Could not create Depthstencil state");
			return;
		}
		//m_DevCon->OMSetDepthStencilState(m_DeptStencilState, 1);

		descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		descDSV.Texture2D.MipSlice = 0;


		// Create the depth stencil view
		result = dev->CreateDepthStencilView(m_ShadowMap, // Depth stencil texture
			&descDSV, // Depth stencil desc
			&m_DepthStencilView);  // [out] Depth stencil view
		if (result != S_OK)
		{
			printf("Could not create Depthstencil view");
			return;
		}
		depthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		depthSRVDesc.Texture2D.MipLevels = 1;
		depthSRVDesc.Texture2D.MostDetailedMip = 0;

		

		result = dev->CreateShaderResourceView(m_ShadowMap, // Depth stencil texture
			&depthSRVDesc, // Depth stencil desc
			&m_ShaderResourceView);  // [out] Depth stencil view
		if (result != S_OK)
		{
			printf("Could not create Depthstencil shader resource view");
			return;
		}

	}
	
	const XMFLOAT2 offsetTable[4] = { XMFLOAT2(0,0), XMFLOAT2(1,0), XMFLOAT2(0,1), XMFLOAT2(1,1) };
	
	XMFLOAT3 ShadowMap::FindSmallestFit(MapNode* n, int size, XMFLOAT3 currentOffset)
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
	XMFLOAT4 ShadowMap::ObtainTextureArea(int size)
	{
		XMFLOAT3 offset = XMFLOAT3(0,0,0);
		offset = FindSmallestFit(m_Base, size,offset);
		assert(offset.z != 0);
		return XMFLOAT4(offset.x, offset.y, size, size);
	}

	ShadowMap::~ShadowMap()
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
		if (m_DeptStencilState)
		{
			m_DeptStencilState->Release();
			m_DeptStencilState = nullptr;
		}
		if (m_DepthStencilView)
		{
			m_DepthStencilView->Release();
			m_DepthStencilView = nullptr;
		}
	}
}