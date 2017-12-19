#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
namespace Themp
{
	using namespace DirectX;
	class D3D;

	class ShadowMap
	{
		struct MapNode
		{
			MapNode(int a_Size)
			{
				taken = false;
				n_Size = a_Size;
				n[0] = nullptr;
				n[1] = nullptr;
				n[2] = nullptr;
				n[3] = nullptr;
			}
			bool taken;
			int n_Size;
			MapNode* n[4];
		};
	public:
		//only square textures allowed
		ShadowMap(int size);
		~ShadowMap();
		XMFLOAT3 FindSmallestFit(MapNode* n, int size, XMFLOAT3 offset);
		XMFLOAT4 ObtainTextureArea(int size);

		ID3D11DepthStencilState* m_DeptStencilState = nullptr;
		ID3D11DepthStencilView* m_DepthStencilView = nullptr;
		ID3D11ShaderResourceView* m_ShaderResourceView = nullptr;
		ID3D11Texture2D* m_ShadowMap = nullptr;
		int m_Size;
		MapNode* m_Base;
	};
}