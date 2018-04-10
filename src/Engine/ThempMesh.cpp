#include "ThempSystem.h"
#include "ThempMesh.h"
#include "ThempD3D.h"
#include "ThempResources.h"
#include "ThempMaterial.h"
#include <d3d10.h>
#include <d3d11.h>
namespace Themp
{
	Mesh::Mesh()
	{

	}

	Mesh::~Mesh()
	{
		delete[] vertices;
		vertices = nullptr;
		delete[] indices;
		indices = nullptr;
		m_Material = nullptr;
		m_VertexBuffer = nullptr;
		m_IndexBuffer = nullptr;
	}
	void Mesh::ConstructVertexBuffer()
	{
		i_VertexBuffer = Themp::Resources::TRes->CreateVertexBuffer(vertices, numVertices);
		i_IndexBuffer = Themp::Resources::TRes->CreateIndexBuffer(indices, numIndices);
		m_VertexBuffer = Themp::Resources::TRes->m_VertexBuffers[i_VertexBuffer];
		m_IndexBuffer = Themp::Resources::TRes->m_IndexBuffers[i_VertexBuffer];
	}
	
	void Mesh::Draw(Themp::D3D& d3d, bool lightPass)
	{
		uint32_t stride[] = { sizeof(Vertex) };
		uint32_t offset[] = { 0 };
		//set vertex/index buffers
		d3d.m_DevCon->IASetVertexBuffers(0, 1, &m_VertexBuffer, stride, offset);
		d3d.m_DevCon->IASetIndexBuffer(m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		//set resources for shader pipeline
		if (!lightPass)
		{
			d3d.m_DevCon->PSSetSamplers(0, 4, m_Material->m_SamplerStates);
			d3d.m_DevCon->PSSetShaderResources(0, 4, m_Material->m_Views);
		}
		d3d.SetMaterialConstantBuffer(m_Material->m_MaterialConstantBuffer);

		d3d.PSUploadConstantBuffersToGPU();


		d3d.m_DevCon->DrawIndexed(numIndices, 0, 0);
	}
}