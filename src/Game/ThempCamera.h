#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
namespace Themp
{
	using namespace DirectX;

	class Camera
	{
		struct CameraBuffer
		{
			XMFLOAT4X4 viewMatrix;
			XMFLOAT4X4 projectionMatrix;
			XMFLOAT4X4 invProjectionMatrix;
			XMFLOAT4X4 invViewMatrix;
		};
	public:
		enum CameraType { Perspective, Orthographic };
		Camera();
		~Camera();
		void SetupCamera(XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 up);
		void MoveForward(float val);
		void MoveBackward(float val);
		void MoveRight(float val);
		void MoveLeft(float val);
		void MoveUp(float val);
		void MoveDown(float val);
		void AddPosition(float x, float y, float z);
		void SetPosition(float x,float y, float z);
		void SetLookDir(float x, float y, float z);
		void Rotate(float a_x, float a_y);
		void SetUpVector(float x, float y, float z);
		void SetAspectRatio(float x);
		void SetFoV(float degrees);
		XMFLOAT4X4 GetMVPMatrix();
		XMFLOAT4X4 GetInvMVPMatrix();
		void UpdateMatrices();

		XMFLOAT3 m_Position;
		XMFLOAT3 m_LookDirection;
		XMFLOAT3 m_Up; 
		float m_OrthoWidth, m_OrthoHeight;
		float m_AspectRatio;
		float m_FoV;
		CameraType m_CamType = Perspective;

		CameraBuffer m_CameraConstantBufferData;
		ID3D11Buffer* m_CameraConstantBuffer;
		bool isDirty = true;
	};
};
