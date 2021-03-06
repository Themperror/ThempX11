#pragma once
#include <vector>
#include <d3d11.h>
#include <DirectXMath.h>
namespace Themp
{
	struct Vertex
	{
		float x, y, z;
		float nx, ny, nz;
		float tx, ty, tz;
		float btx, bty, btz;
		float u, v;
	};
	class Material;
	class Mesh
	{
	public:
		Mesh();
		~Mesh();
		enum DrawPass{GBUFFER,SHADOW,LIGHT};
		void ConstructVertexBuffer();
		void Draw(Themp::D3D& d3d, DrawPass pass = GBUFFER);
		Material* m_Material;
		Vertex* m_Vertices;
		uint32_t m_NumVertices;
		uint32_t* m_Indices;
		uint32_t m_NumIndices;
		size_t i_VertexBuffer; //index to the vertexbuffer in resourcemanager
		size_t i_IndexBuffer; // index to the indexbuffer in resourcemanager
		DirectX::XMFLOAT3 m_BoundsMin;
		DirectX::XMFLOAT3 m_BoundsMax;

		//do not change manually
		ID3D11Buffer* m_VertexBuffer;
		//do not change manually
		ID3D11Buffer* m_IndexBuffer;
	};
}