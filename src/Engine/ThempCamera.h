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
			XMFLOAT4 cameraPosition;
			XMFLOAT4 cameraDir;
		};
	public:
		enum CameraType { Perspective, Orthographic };
		Camera();
		~Camera();
		void MoveForward();
		void MoveBackward();
		void MoveRight();
		void MoveLeft();
		void MoveUp();
		void MoveDown();
		void SetSpeed(float val);
		void AddPosition(float x, float y, float z);
		void SetPosition(float x, float y, float z);
		void SetPosition(XMFLOAT3 pos);
		void SetPosition(XMFLOAT4 pos);

		//Sets m_Forward/m_Up/m_Right , overridden as soon as you use Rotate or SetupCamera!!
		void SetLookTo(XMFLOAT3 forward, XMFLOAT3 up, XMFLOAT3 right);
		//Sets m_Forward/m_Up/m_Right , overridden as soon as you use Rotate or SetupCamera!!
		void SetLookTo(XMFLOAT4 forward, XMFLOAT3 up, XMFLOAT3 right);

		//calculates right vector (more expensive!)
		//Sets m_Forward/m_Up/m_Right , overridden as soon as you use Rotate or SetupCamera!!
		void SetLookTo(XMFLOAT3 forward, XMFLOAT3 up);
		void Rotate(float a_x, float a_y);
		void SetAspectRatio(float x);
		void SetFoV(float degrees);
		void SetOrtho(float w, float h);
		void SetProjection(CameraType t);
		void CalcDirs();
		void Update(float dt);
		XMFLOAT4X4 GetMVPMatrix();
		XMFLOAT4X4 GetInvMVPMatrix();
		XMFLOAT4X4 GetOrthoProjectionMatrix(float nearPlane, float farPlane);
		void UpdateMatrices();

		XMFLOAT3 GetForward() { return m_Forward; }
		XMFLOAT3 GetRight() { return m_Right; }
		XMFLOAT3 GetUp() { return m_Up; }
		XMFLOAT3 GetPosition() { return m_Position; }
		XMFLOAT4 GetQuat() { return m_Quaternion; }
		float GetFoV() { return m_FoV; }
		float GetOrthoWidth() { return m_OrthoWidth; }
		float GetOrthoHeight() { return m_OrthoHeight; }

		CameraBuffer m_CameraConstantBufferData;
		ID3D11Buffer* m_CameraConstantBuffer;
		bool isDirty = true;

	private:
		XMFLOAT3 m_Position;
		XMFLOAT3 m_Velocity;
		XMFLOAT4 m_Quaternion;
		XMFLOAT3 m_Up;
		XMFLOAT3 m_Forward;
		XMFLOAT3 m_Right;
		float m_OrthoWidth, m_OrthoHeight;
		float m_AspectRatio;
		float m_FoV;
		CameraType m_CamType = Perspective;
	};
};