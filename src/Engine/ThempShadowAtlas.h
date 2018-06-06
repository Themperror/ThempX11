#pragma once
#include <DirectXMath.h>
namespace Themp
{
	using namespace DirectX;
	class ShadowAtlas
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
			~MapNode()
			{
				for (int i = 0; i < 4; i++)
				{
					if(n[i])delete n[i];
				}
			}
		};
	public:
		//only square textures allowed
		ShadowAtlas(int size);
		~ShadowAtlas();
		XMFLOAT3 FindSmallestFit(MapNode* n, int size, XMFLOAT3 offset);
		XMFLOAT4 ObtainTextureArea(int size);

		int m_Size;
		MapNode* m_Base;
	};
}