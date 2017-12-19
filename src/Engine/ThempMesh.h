#pragma once
#include <vector>
#include <d3d11.h>
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
		void ConstructVertexBuffer();
		void SetGPUData(Themp::D3D & d3d);
		void Draw(Themp::D3D& d3d, bool lightPass = false);
		Material* m_Material;
		Vertex* vertices;
		uint32_t numVertices;
		uint32_t* indices;
		uint32_t numIndices;

		ID3D11Buffer* m_VertexBuffer;
		ID3D11Buffer* m_IndexBuffer;
	};
}