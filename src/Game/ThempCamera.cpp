#include "ThempSystem.h"
#include "ThempCamera.h"
#include "../Engine/ThempD3D.h"
namespace Themp
{
	Camera::Camera()
	{
		// Fill in a buffer description.
		D3D11_BUFFER_DESC cbDesc;
		cbDesc.ByteWidth = sizeof(CameraBuffer);
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = &m_CameraConstantBufferData;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the buffer.
		Themp::System::tSys->m_D3D->m_Device->CreateBuffer(&cbDesc, &InitData, &m_CameraConstantBuffer);
		SetFoV(90);
		SetAspectRatio(1.0);
		SetUpVector(0, 1.0, 0);
		SetLookDir(1, 0, 0);
		SetPosition(0, 0, 0);
	}
	Camera::~Camera()
	{
		m_CameraConstantBuffer->Release();
		m_CameraConstantBuffer = nullptr;
	}
	void Camera::SetupCamera(XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 up)
	{
		isDirty = true;
		m_Position = pos;
		m_LookDirection = rot;
		m_Up = up;
	}
	void Camera::MoveForward(float val)
	{
		isDirty = true;
		XMVECTOR dir = XMLoadFloat3(&m_LookDirection);
		XMVECTOR pos = XMLoadFloat3(&m_Position);
		XMFLOAT3 finalPos;
		XMStoreFloat3(&finalPos, pos + dir * val);
		m_Position = finalPos;
	}
	void Camera::MoveBackward(float val)
	{
		isDirty = true;
		XMVECTOR dir = XMLoadFloat3(&m_LookDirection);
		XMVECTOR pos = XMLoadFloat3(&m_Position);
		XMFLOAT3 finalPos;
		XMStoreFloat3(&finalPos, pos - dir * val);
		m_Position = finalPos;
	}
	void Camera::MoveRight(float val)
	{
		isDirty = true;
		XMVECTOR up = XMLoadFloat3(&m_Up);
		XMVECTOR dir = XMLoadFloat3(&m_LookDirection);
		XMVECTOR pos = XMLoadFloat3(&m_Position);
		XMFLOAT3 finalPos;
		XMStoreFloat3(&finalPos, pos + XMVector3Normalize(XMVector3Cross(up, dir)) * val);
		m_Position = finalPos;
	}
	void Camera::MoveLeft(float val)
	{
		isDirty = true;
		XMVECTOR up = XMLoadFloat3(&m_Up);
		XMVECTOR dir = XMLoadFloat3(&m_LookDirection);
		XMVECTOR pos = XMLoadFloat3(&m_Position);
		XMFLOAT3 finalPos;
		XMStoreFloat3(&finalPos, pos - XMVector3Normalize(XMVector3Cross(up, dir)) * val);
		m_Position = finalPos;
	}
	void Camera::MoveUp(float val)
	{
		isDirty = true;
		XMVECTOR dir = XMLoadFloat3(&m_Up);
		XMVECTOR pos = XMLoadFloat3(&m_Position);
		XMFLOAT3 finalPos;
		XMStoreFloat3(&finalPos, pos + dir * val);
		m_Position = finalPos;
	}
	void Camera::MoveDown(float val)
	{
		isDirty = true;
		XMVECTOR dir = XMLoadFloat3(&m_Up);
		XMVECTOR pos = XMLoadFloat3(&m_Position);
		XMFLOAT3 finalPos;
		XMStoreFloat3(&finalPos, pos - dir * val);
		m_Position = finalPos;
	}
	void Camera::AddPosition(float x, float y, float z)
	{
		isDirty = true;
		m_Position.x += x;
		m_Position.y += y;
		m_Position.z += z;

	}
	void Camera::SetPosition(float x, float y, float z)
	{
		isDirty = true;
		m_Position = XMFLOAT3(x, y, z);
	}
	void Camera::SetLookDir(float x, float y, float z)
	{
		isDirty = true;
		XMStoreFloat3(&m_LookDirection, XMVector3Normalize(XMVectorSet(x, y, z, 1.0f)));
	}
	void Camera::Rotate(float a_x, float a_y)
	{
		float x = std::cos(a_x * 3.141592f / 180);
		float z = std::sin(a_x * 3.141592f / 180);
		float y = std::tan(a_y * 3.141592f / 200);

		//XMFLOAT3 lookAt;
		//lookAt.x = m_Position.x + x;
		//lookAt.y = m_Position.y + y;
		//lookAt.z = m_Position.z + z;
		//
		//m_LookDirection.x = lookAt.x - m_Position.x;
		//m_LookDirection.y = lookAt.y - m_Position.y;
		//m_LookDirection.z = lookAt.z - m_Position.z;
		XMStoreFloat3(&m_LookDirection, XMVector3Normalize(XMVectorSet(x, y, z, 1)));
		isDirty = true;
	}
	void Camera::SetUpVector(float x, float y, float z)
	{
		isDirty = true;
		XMStoreFloat3(&m_Up,XMVector3Normalize(XMVectorSet(x, y, z, 1)));
	}
	void Camera::SetAspectRatio(float x)
	{
		m_AspectRatio = x;
	}
	void Camera::SetFoV(float degrees)
	{
		m_FoV = degrees*0.0174533f;
	}
	void Camera::UpdateMatrices()
	{
		Themp::D3D* d3d = Themp::System::tSys->m_D3D;
		if (isDirty)
		{
			XMMATRIX projectionMatrix;
			XMMATRIX viewMatrix;
			XMMATRIX invProjectionMatrix;
			XMMATRIX invViewMatrix;

			if (m_CamType == CameraType::Perspective)
			{
				projectionMatrix = XMMatrixPerspectiveFovLH(m_FoV, m_AspectRatio, 0.1f, 1000.0f);
				viewMatrix = XMMatrixLookToLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&m_LookDirection), XMLoadFloat3(&m_Up));
				invProjectionMatrix = XMMatrixInverse(nullptr, projectionMatrix);
				invViewMatrix = XMMatrixInverse(nullptr, viewMatrix);
			}
			else
			{
				projectionMatrix = XMMatrixOrthographicLH(m_OrthoWidth,m_OrthoHeight, 0.1f, 1000.0f);
				viewMatrix = XMMatrixLookToLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&m_LookDirection), XMLoadFloat3(&m_Up));
				invProjectionMatrix = XMMatrixInverse(nullptr, projectionMatrix);
				invViewMatrix = XMMatrixInverse(nullptr, viewMatrix);
			}

			XMStoreFloat4x4(&m_CameraConstantBufferData.projectionMatrix, (projectionMatrix));
			XMStoreFloat4x4(&m_CameraConstantBufferData.viewMatrix, (viewMatrix));
			XMStoreFloat4x4(&m_CameraConstantBufferData.invProjectionMatrix, (invProjectionMatrix));
			XMStoreFloat4x4(&m_CameraConstantBufferData.invViewMatrix, (invViewMatrix));
			m_CameraConstantBufferData.cameraPosition = XMFLOAT4(m_Position.x, m_Position.y, m_Position.z, 1.0);
			m_CameraConstantBufferData.cameraDir = XMFLOAT4(m_LookDirection.x, m_LookDirection.y, m_LookDirection.z, 0.0f);

			D3D11_MAPPED_SUBRESOURCE ms;
			d3d->m_DevCon->Map(m_CameraConstantBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
			memcpy(ms.pData, &m_CameraConstantBufferData, sizeof(m_CameraConstantBufferData));
			d3d->m_DevCon->Unmap(m_CameraConstantBuffer, NULL);
			isDirty = false;
		}
		d3d->SetCameraConstantBuffer(m_CameraConstantBuffer);
	}
	XMFLOAT4X4 Camera::GetMVPMatrix()
	{
		XMMATRIX projectionMatrix = XMLoadFloat4x4(&m_CameraConstantBufferData.projectionMatrix);
		XMMATRIX viewMatrix = XMLoadFloat4x4(&m_CameraConstantBufferData.viewMatrix);

		XMFLOAT4X4 MVP;
		XMStoreFloat4x4(&MVP, (XMMatrixMultiply(viewMatrix, projectionMatrix)));
		return MVP;
	}
	XMFLOAT4X4 Camera::GetInvMVPMatrix()
	{
		XMMATRIX projectionMatrix = XMLoadFloat4x4(&m_CameraConstantBufferData.projectionMatrix);
		XMMATRIX viewMatrix = XMLoadFloat4x4(&m_CameraConstantBufferData.viewMatrix);

		XMFLOAT4X4 MVP;
		XMStoreFloat4x4(&MVP, XMMatrixInverse(nullptr,(XMMatrixMultiply(viewMatrix, projectionMatrix))));
		return MVP;
	}
}