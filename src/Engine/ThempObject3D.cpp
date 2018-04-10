#include "ThempSystem.h"
#include "ThempObject3D.h"
#include "ThempMesh.h"
#include "ThempMaterial.h"
#include "ThempD3D.h"
#include "ThempResources.h"
#include <DirectXMath.h>

#include <iostream>
using namespace DirectX;
namespace Themp
{
	Object3D::Object3D()
	{
		m_Position = XMFLOAT3(0, 0, 0);
		m_Scale = XMFLOAT3(1,1,1);
		m_Rotation = XMFLOAT3(0, 0, 0);
	}
	Object3D::~Object3D()
	{
		if (m_ConstantBuffer)
		{
			m_ConstantBuffer->Release();
			m_ConstantBuffer = nullptr;
		}
	}
	void Object3D::Update(float dt)
	{
		m_Rotation.y += dt*0.2;
		isDirty = true;
	}
	void Object3D::Draw(D3D& d3d, bool lightPass)
	{
		if (isDirty)
		{
			XMVECTOR trans =XMLoadFloat3(&m_Position), rot=XMLoadFloat3(&m_Rotation), scale=XMLoadFloat3(&m_Scale);

			XMMATRIX RotScale =  XMMatrixRotationRollPitchYawFromVector(rot) *  XMMatrixScalingFromVector(scale);
			XMMATRIX WorldMatrix =  RotScale * XMMatrixTranslationFromVector(trans);

			XMStoreFloat4x4(&m_ConstantBufferData.worldMatrix, (WorldMatrix));

			if (!m_ConstantBuffer)
			{
				// Fill in a buffer description.
				D3D11_BUFFER_DESC cbDesc;
				cbDesc.ByteWidth = sizeof(Object3DConstantBufferData);
				cbDesc.Usage = D3D11_USAGE_DYNAMIC;
				cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				cbDesc.MiscFlags = 0;
				cbDesc.StructureByteStride = 0;

				// Fill in the subresource data.
				D3D11_SUBRESOURCE_DATA InitData;
				InitData.pSysMem = &m_ConstantBufferData;
				InitData.SysMemPitch = 0;
				InitData.SysMemSlicePitch = 0;
				// Create the buffer.
				Themp::System::tSys->m_D3D->m_Device->CreateBuffer(&cbDesc, &InitData, &m_ConstantBuffer);
			}
			D3D11_MAPPED_SUBRESOURCE ms;
			d3d.m_DevCon->Map(m_ConstantBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
			memcpy(ms.pData, &m_ConstantBufferData, sizeof(Object3DConstantBufferData));
			d3d.m_DevCon->Unmap(m_ConstantBuffer, NULL);
			
			//printf("Updated Object buffer \n");
			isDirty = false;
		}
		d3d.SetObject3DConstantBuffer(m_ConstantBuffer);

		d3d.VSUploadConstantBuffersToGPU();
		//if (m_Meshes.size() > 0)
		//{
		// m_Meshes[0]->SetGPUData(d3d); //setting material related data, and for now all meshes use the same material
		//}
		for (int i = 0; i < m_Meshes.size(); ++i)
		{
			m_Meshes[i]->Draw(d3d,lightPass);
		}
	}
	void Object3D::ForceBufferUpdate()
	{
		XMVECTOR trans = XMLoadFloat3(&m_Position), rot = XMLoadFloat3(&m_Rotation), scale = XMLoadFloat3(&m_Scale);

		XMMATRIX RotScale = XMMatrixRotationRollPitchYawFromVector(rot) *  XMMatrixScalingFromVector(scale);
		XMMATRIX WorldMatrix = RotScale * XMMatrixTranslationFromVector(trans);

		XMStoreFloat4x4(&m_ConstantBufferData.worldMatrix, (WorldMatrix));

		if (!m_ConstantBuffer)
		{
			// Fill in a buffer description.
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.ByteWidth = sizeof(Object3DConstantBufferData);
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = 0;

			// Fill in the subresource data.
			D3D11_SUBRESOURCE_DATA InitData;
			InitData.pSysMem = &m_ConstantBufferData;
			InitData.SysMemPitch = 0;
			InitData.SysMemSlicePitch = 0;
			// Create the buffer.
			Themp::System::tSys->m_D3D->m_Device->CreateBuffer(&cbDesc, &InitData, &m_ConstantBuffer);
		}
		D3D11_MAPPED_SUBRESOURCE ms;
		D3D::s_D3D->m_DevCon->Map(m_ConstantBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
		memcpy(ms.pData, &m_ConstantBufferData, sizeof(Object3DConstantBufferData));
		D3D::s_D3D->m_DevCon->Unmap(m_ConstantBuffer, NULL);
	}
	void Object3D::CreateCube(std::string shader, bool vertexShader, bool pixelShader, bool geometryShader)
	{
		Mesh* mesh = new Mesh();
		Themp::System::tSys->m_Resources->m_Meshes.push_back(mesh);
		mesh->vertices = new Vertex[8];
		mesh->vertices[0] = { -0.5f, +0.5f, -0.5f  , 1.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 1.0f,0.0f };
		mesh->vertices[1] = { +0.5f, +0.5f, -0.5f  , 0.0f, 1.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f,1.0f };
		mesh->vertices[2] = { +0.5f, +0.5f,  0.5f  , 0.0f, 0.0f, 1.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f,0.0f };
		mesh->vertices[3] = { -0.5f, +0.5f,  0.5f  , 1.0f, 1.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 1.0f,1.0f };
		mesh->vertices[4] = { -0.5f, -0.5f,  0.5f  , 0.0f, 1.0f, 1.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 1.0f,0.0f };
		mesh->vertices[5] = { +0.5f, -0.5f,  0.5f  , 1.0f, 1.0f, 1.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f,1.0f };
		mesh->vertices[6] = { +0.5f, -0.5f, -0.5f  , 1.0f, 0.0f, 1.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f,0.0f };
		mesh->vertices[7] = { -0.5f, -0.5f, -0.5f  , 1.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 1.0f,1.0f };

		mesh->indices = new uint32_t[36]{
			0, 1, 2,
			0, 2, 3,
			4, 5, 6,
			4, 6, 7,
			3, 2, 5,
			3, 5, 4,
			2, 1, 6,
			2, 6, 5,
			1, 7, 6,
			1, 0, 7,
			0, 3, 4,
			0, 4, 7
		};


		mesh->numIndices = 36;
		mesh->numVertices = 8;
		mesh->ConstructVertexBuffer();
		mesh->m_Material = Themp::System::tSys->m_Resources->GetMaterial("","DefaultDiffuse.dds", shader, vertexShader, pixelShader, geometryShader);
		m_Meshes.push_back(mesh);
	}
	void Object3D::CreateTriangle(std::string shader, bool vertexShader, bool pixelShader, bool geometryShader)
	{
		Mesh* mesh = new Mesh();
		Themp::System::tSys->m_Resources->m_Meshes.push_back(mesh);
		mesh->vertices = new Vertex[3];
		mesh->vertices[0] = { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f,0.0f };
		mesh->vertices[1] = { -0.5f, +0.5f, 0.0f, 1.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f,1.0f };
		mesh->vertices[2] = { +0.5f, +0.5f, 0.0f, 1.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 1.0f,1.0f };

		mesh->indices = new uint32_t[3]{
			// front face
			0, 1, 2
		};


		mesh->numIndices = 3;
		mesh->numVertices = 3;
		mesh->ConstructVertexBuffer();
		mesh->m_Material = Themp::System::tSys->m_Resources->GetMaterial("", "DefaultDiffuse.dds", shader, vertexShader, pixelShader, geometryShader);
		m_Meshes.push_back(mesh);
	}
	void Object3D::CreateQuad(std::string shader, bool vertexShader, bool pixelShader, bool geometryShader)
	{
		Mesh* mesh = new Mesh();
		Themp::System::tSys->m_Resources->m_Meshes.push_back(mesh);
		mesh->vertices = new Vertex[4];
		mesh->vertices[0] = { -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f,0.0f };
		mesh->vertices[1] = { -1.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 0.0f,1.0f };
		mesh->vertices[2] = { +1.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 1.0f,1.0f };
		mesh->vertices[3] = { +1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f, 1.0f,0.0f };

		mesh->indices = new uint32_t[6]{
			// front face
			0, 1, 2,
			0, 2, 3
		};


		mesh->numIndices = 6;
		mesh->numVertices = 4;
		mesh->ConstructVertexBuffer();
		mesh->m_Material = Themp::System::tSys->m_Resources->GetMaterial("", "DefaultDiffuse.dds", shader, vertexShader, pixelShader, geometryShader);
		m_Meshes.push_back(mesh);
	}
	void Object3D::Construct()
	{
		for (size_t i = 0; i < m_Meshes.size(); i++)
		{
			m_Meshes[i]->ConstructVertexBuffer();
		}
	}
}