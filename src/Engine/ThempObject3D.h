#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
namespace Themp
{
	using namespace DirectX;
	class Mesh;
	class D3D;

	class Object3D
	{
		struct Object3DConstantBufferData
		{
			XMFLOAT4X4 worldMatrix;
		};
	public:
		Object3D();
		~Object3D();
		void Update(float dt);
		void Draw(Themp::D3D& d3d, bool lightPass = false);
		void CreateCube(std::string shader, bool vertexShader, bool pixelShader, bool geometryShader);
		void CreateTriangle(std::string shader, bool vertexShader, bool pixelShader, bool geometryShader);

		void CreateQuad(std::string shader, bool vertexShader, bool pixelShader, bool geometryShader);

		//Only need to call this when you're manually building a mesh, this function builds all vertex and index buffers for you
		void Construct();

		std::vector<Themp::Mesh*> m_Meshes;
		XMFLOAT3 m_Position, m_Rotation, m_Scale;
		ID3D11Buffer* m_ConstantBuffer;
		Object3DConstantBufferData m_ConstantBufferData;
		bool isDirty = true;
	};
}