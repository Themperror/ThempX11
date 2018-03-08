#include "ThempSystem.h"
#include "ThempRenderTexture.h"
#include "ThempD3D.h"
namespace Themp
{
	RenderTexture::RenderTexture(int width, int height, TextureType type)
	{
		m_TextureType = type;
		m_ResolutionX = width;
		m_ResolutionY = height;
		ID3D11Device* m_Device = Themp::System::tSys->m_D3D->m_Device;
		ID3D11DeviceContext* m_DevCon = Themp::System::tSys->m_D3D->m_DevCon;
		if (type == TextureType::RenderTex || type == TextureType::CubeRenderTex)
		{
			HRESULT result;
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
			renderTexDesc.Height = height;
			renderTexDesc.Width = width;

			result = m_Device->CreateTexture2D(&renderTexDesc, nullptr, &m_Texture);

			if (result != S_OK) { System::Print("Could not CreateTexture2D"); return; }
			// use the back buffer address to create the render target
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			memset(&srvDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			srvDesc.Format = renderTexDesc.Format;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;

			result = m_Device->CreateShaderResourceView(m_Texture, &srvDesc, &m_ShaderResourceView);
			if (result != S_OK) { System::Print("Could not CreateShaderResourceView"); return; }

			result = m_Device->CreateRenderTargetView(m_Texture, nullptr, &m_RenderTarget);
			if (result != S_OK) { System::Print("Could not CreateRenderTargetView"); return; }

		}
		else if (type == TextureType::DepthTex || type == TextureType::CubeDepthTex)
		{
			HRESULT result;

			D3D11_TEXTURE2D_DESC depthBufferDesc;
			D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
			D3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc;
			memset(&depthBufferDesc, 0, sizeof(D3D11_TEXTURE2D_DESC));
			memset(&descDSV, 0, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
			memset(&depthSRVDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

			depthBufferDesc.SampleDesc.Count = 1;
			depthBufferDesc.SampleDesc.Quality = 0;

			depthBufferDesc.Width = width;
			depthBufferDesc.Height = height;
			depthBufferDesc.MipLevels = 1;

			depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

			depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // DXGI_FORMAT_D16_UNORM;
			depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			depthBufferDesc.CPUAccessFlags = 0;
			if (type == TextureType::CubeDepthTex)
			{
				depthBufferDesc.ArraySize = 6;
				depthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURECUBE;
				descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				descDSV.Texture2DArray.ArraySize = -1;
				descDSV.Texture2DArray.FirstArraySlice = 0;
				descDSV.Texture2DArray.MipSlice = 0;
				depthBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
			}
			else
			{
				depthBufferDesc.ArraySize = 1;
				depthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
				descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			}
			result = m_Device->CreateTexture2D(&depthBufferDesc, NULL, &m_Texture);
			if (result != S_OK)
			{
				System::Print("Could not create Depthstencil texture");
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
			result = m_Device->CreateDepthStencilState(&dsDesc, &m_DeptStencilState);
			if (result != S_OK)
			{
				System::Print("Could not create Depthstencil state");
				return;
			}
			//m_DevCon->OMSetDepthStencilState(m_DeptStencilState, 1);

			descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			descDSV.Texture2D.MipSlice = 0;


			// Create the depth stencil view
			result = m_Device->CreateDepthStencilView(m_Texture, // Depth stencil texture
				&descDSV, // Depth stencil desc
				&m_DepthStencilView);  // [out] Depth stencil view
			if (result != S_OK)
			{
				System::Print("Could not create Depthstencil view");
				return;
			}
			depthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			depthSRVDesc.Texture2D.MipLevels = 1;
			depthSRVDesc.Texture2D.MostDetailedMip = 0;

;

			result = m_Device->CreateShaderResourceView(m_Texture, // Depth stencil texture
				&depthSRVDesc, // Depth stencil desc
				&m_ShaderResourceView);  // [out] Depth stencil view
			if (result != S_OK)
			{
				System::Print("Could not create Depthstencil shader resource view");
				return;
			}
		}
	}
	RenderTexture::~RenderTexture()
	{
		CLEAN(m_RenderTarget);
		CLEAN(m_ShaderResourceView);
		CLEAN(m_Texture);
		CLEAN(m_DeptStencilState);
		CLEAN(m_DepthStencilView);
	}
}