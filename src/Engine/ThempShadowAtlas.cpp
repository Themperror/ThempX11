#include "ThempSystem.h"
#include "ThempShadowAtlas.h"
#include <DirectXMath.h>
namespace Themp
{
	ShadowAtlas::ShadowAtlas(int size)
	{
		m_Size = size;
		m_Base = new MapNode(size);
		m_Base->n_Size = size;
		m_Base->n[0] = new MapNode(size/2);
		m_Base->n[1] = new MapNode(size/2);
		m_Base->n[2] = new MapNode(size/2);
		m_Base->n[3] = new MapNode(size/2);
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
	ShadowAtlas::~ShadowAtlas()
	{
		if(m_Base) delete m_Base;
	}
}