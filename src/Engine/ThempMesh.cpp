#include "ThempSystem.h"
#include "ThempMesh.h"
#include "ThempD3D.h"
#include <d3d10.h>
#include <d3d11.h>
namespace Themp
{
	Mesh::Mesh()
	{

	}

	Mesh::~Mesh()
	{
		if (m_VertexBuffer)
		{
			m_VertexBuffer->Release();
			m_VertexBuffer = nullptr;
		}
		if (m_IndexBuffer)
		{
			m_IndexBuffer->Release();
			m_IndexBuffer = nullptr;
		}
		delete[] vertices;
		delete[] indices;
		m_Material = nullptr;

	}
	void Mesh::ConstructVertexBuffer()
	{
		if (m_VertexBuffer)
		{
			m_VertexBuffer->Release();
			m_VertexBuffer = nullptr;
		}
		if (m_IndexBuffer)
		{
			m_IndexBuffer->Release();
			m_IndexBuffer = nullptr;
		}

		Themp::D3D* d = Themp::System::tSys->m_D3D;
		D3D11_BUFFER_DESC bd;
		D3D11_MAPPED_SUBRESOURCE ms;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&ms, sizeof(D3D11_MAPPED_SUBRESOURCE));

		//set up for vertices
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(Vertex) * numVertices;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT res = d->m_Device->CreateBuffer(&bd, NULL, &m_VertexBuffer);

		d->m_DevCon->Map(m_VertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
		memcpy(ms.pData, vertices, sizeof(Vertex)*numVertices);
		d->m_DevCon->Unmap(m_VertexBuffer, NULL);

		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&ms, sizeof(D3D11_MAPPED_SUBRESOURCE));
		//set up for indices
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(uint32_t) * numIndices;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		res = d->m_Device->CreateBuffer(&bd, NULL, &m_IndexBuffer);

		d->m_DevCon->Map(m_IndexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
		memcpy(ms.pData, indices, sizeof(uint32_t)*numIndices);
		d->m_DevCon->Unmap(m_IndexBuffer, NULL);
	}
	void Mesh::SetGPUData(Themp::D3D& d3d)
	{
		//set shader from material
		//d3d.m_DevCon->VSSetShader(m_Material->m_VertexShader, 0, 0);
		//d3d.m_DevCon->PSSetShader(m_Material->m_PixelShader, 0, 0);
		//if (m_Material->m_GeometryShader)
		//{
		//	d3d.m_DevCon->GSSetShader(m_Material->m_GeometryShader, 0, 0);
		//}
		//else
		//{
		//	d3d.m_DevCon->GSSetShader(nullptr, 0, 0);
		//}

		
		//m_DevCon->GSSetConstantBuffers(0, 1, &m_CBuffer);
		
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