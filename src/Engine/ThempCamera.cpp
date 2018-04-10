#include "ThempSystem.h"
#include "ThempCamera.h"
#include "ThempD3D.h"
namespace Themp
{
	DirectX::XMFLOAT3 operator+=(DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		a = DirectX::XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z);
		return a;
	}
	DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		return DirectX::XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z);
	}
	DirectX::XMFLOAT3 operator-(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		return DirectX::XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
	}
	DirectX::XMFLOAT3 operator-=(DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b)
	{
		a = DirectX::XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
		return a;
	}
	DirectX::XMFLOAT3 operator*=(DirectX::XMFLOAT3& a, const float& b)
	{
		a = DirectX::XMFLOAT3(a.x * b, a.y * b, a.z * b);
		return a;
	}
	DirectX::XMFLOAT3 operator*(const DirectX::XMFLOAT3& a, const float& b)
	{
		return DirectX::XMFLOAT3(a.x * b, a.y * b, a.z * b);
	}
	DirectX::XMFLOAT3 operator/(const DirectX::XMFLOAT3& a, const float& b)
	{
		return DirectX::XMFLOAT3(a.x / b, a.y / b, a.z / b);
	}
	DirectX::XMFLOAT3 Normalize(const DirectX::XMFLOAT3& a)
	{
		XMVECTOR v = XMLoadFloat3(&a);
		XMVector3Normalize(v);
		XMFLOAT3 res;
		XMStoreFloat3(&res, v);
		return res;
	}
	XMFLOAT3 Clamp(const DirectX::XMFLOAT3& val, float a_min, float a_max)
	{
		return XMFLOAT3(
			min(max(val.x, a_min), a_max), 
			min(max(val.y, a_min), a_max),
			min(max(val.z, a_min), a_max)
		);
	}
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
		Rotate(0, 0);
		SetPosition(0, 0, 0);
		m_Velocity = XMFLOAT3(0, 0, 0);
	}
	Camera::~Camera()
	{
		m_CameraConstantBuffer->Release();
		m_CameraConstantBuffer = nullptr;
	}
	//forward, back, up , down , left , right
	bool dirs[6] = { false,false,false,false,false,false };
	XMFLOAT3 m_Target = XMFLOAT3(0, 0, 0);
	float m_Speed = 1.0f;
	void Camera::MoveForward()
	{
		isDirty = true;
		dirs[0] = true;
		m_Target = m_Position;
		//XMVECTOR dir = XMLoadFloat3(&m_Forward);
		//XMVECTOR pos = XMLoadFloat3(&m_Position);
		//XMFLOAT3 finalPos;
		//XMStoreFloat3(&finalPos, pos + dir * val);
		//m_Position = finalPos;
	}
	void Camera::MoveBackward()
	{
		isDirty = true;
		dirs[1] = true;
		m_Target = m_Position;
		//XMVECTOR dir = XMLoadFloat3(&m_Forward);
		//XMVECTOR pos = XMLoadFloat3(&m_Position);
		//XMFLOAT3 finalPos;
		//XMStoreFloat3(&finalPos, pos - dir * val);
		//m_Position = finalPos;
	}
	void Camera::MoveRight()
	{
		isDirty = true;
		dirs[5] = true;
		m_Target = m_Position;
		//XMVECTOR dir = XMLoadFloat3(&m_Right);
		//XMVECTOR pos = XMLoadFloat3(&m_Position);
		//XMFLOAT3 finalPos;
		//XMStoreFloat3(&finalPos, pos + XMVector3Normalize(dir) * val);
		//m_Position = finalPos;
	}
	void Camera::MoveLeft()
	{
		isDirty = true;
		dirs[4] = true;
		m_Target = m_Position;
		//XMVECTOR dir = XMLoadFloat3(&m_Right);
		//XMVECTOR pos = XMLoadFloat3(&m_Position);
		//XMFLOAT3 finalPos;
		//XMStoreFloat3(&finalPos, pos - XMVector3Normalize(dir) * val);
		//m_Position = finalPos;
	}
	void Camera::MoveUp()
	{
		isDirty = true;
		dirs[2] = true;
		m_Target = m_Position;
		//XMVECTOR dir = XMLoadFloat3(&m_Up);
		//XMVECTOR pos = XMLoadFloat3(&m_Position);
		//XMFLOAT3 finalPos;
		//XMStoreFloat3(&finalPos, pos + dir * val);
		//m_Position = finalPos;
	}
	void Camera::MoveDown()
	{
		isDirty = true;
		dirs[3] = true;
		m_Target = m_Position;
		//XMVECTOR dir = XMLoadFloat3(&m_Up);
		//XMVECTOR pos = XMLoadFloat3(&m_Position);
		//XMFLOAT3 finalPos;
		//XMStoreFloat3(&finalPos, pos - dir * val);
		//m_Position = finalPos;
	}
	void Camera::SetSpeed(float val)
	{
		m_Speed = val;
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

	void Camera::SetPosition(XMFLOAT3 pos)
	{
		isDirty = true;
		m_Position = pos;
	}

	void Camera::SetPosition(XMFLOAT4 pos)
	{
		isDirty = true;
		m_Position = XMFLOAT3(pos.x,pos.y,pos.z);
	}
	
	void Camera::SetLookTo(XMFLOAT3 forward, XMFLOAT3 up, XMFLOAT3 right)
	{
		isDirty = true;
		m_Forward = forward;
		m_Up = up;
		m_Right = right;
	}
	void Camera::SetLookTo(XMFLOAT4 forward, XMFLOAT3 up, XMFLOAT3 right)
	{
		isDirty = true;
		m_Forward = XMFLOAT3(forward.x,forward.y,forward.z);
		m_Up = up;
		m_Right = right;
	}
	void Camera::SetLookTo(XMFLOAT3 forward, XMFLOAT3 up)
	{
		isDirty = true;
		m_Forward = forward;
		m_Up = up;
		XMVECTOR result = XMVector3Cross(XMLoadFloat3(&m_Forward), XMLoadFloat3(&m_Up));
		XMStoreFloat3(&m_Right, result);
	}
	void Camera::Rotate(float a_x, float a_y)
	{
		XMVECTOR quat = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(-a_y), XMConvertToRadians(-a_x), 0);
		XMStoreFloat4(&m_Quaternion, quat);
		CalcDirs();
	}
	void Camera::SetAspectRatio(float x)
	{
		m_AspectRatio = x;
		isDirty = true;
	}
	void Camera::SetFoV(float degrees)
	{
		m_FoV = degrees*0.0174533f;
		isDirty = true;
	}
	void Camera::SetOrtho(float w, float h)
	{
		m_OrthoWidth = w;
		m_OrthoHeight = h;
		isDirty = true;
	}
	void Camera::SetProjection(CameraType t)
	{
		m_CamType = t;
		isDirty = true;
	}
	void Camera::CalcDirs()
	{
		XMVECTOR quat = XMLoadFloat4(&m_Quaternion);
		XMFLOAT3 inputEdit = XMFLOAT3(1, 0, 0);
		XMVECTOR input = XMLoadFloat3(&inputEdit);
		input = XMVector3Rotate(input, quat);
		XMStoreFloat3(&inputEdit, input);
		m_Right = inputEdit;

		inputEdit = XMFLOAT3(0, 1, 0);
		input = XMLoadFloat3(&inputEdit);
		input = XMVector3Rotate(input, quat);
		XMStoreFloat3(&inputEdit, input);
		m_Up = inputEdit;

		inputEdit = XMFLOAT3(0, 0, 1);
		input = XMLoadFloat3(&inputEdit);
		input = XMVector3Rotate(input, quat);
		XMStoreFloat3(&inputEdit, input);
		m_Forward = inputEdit;

		isDirty = true;
	}

	bool needSmoothingOut = false;
	float time = 0;
	float interpSpeed = 1.0;
	XMFLOAT3 InterpTo(const XMFLOAT3& Current, const XMFLOAT3& Target, float DeltaTime, float InterpSpeed)
	{
		// If no interp speed, jump to target value
		if (InterpSpeed <= 0.f)
		{
			return Target;
		}

		// Distance to reach
		const XMFLOAT3 Dist = Target - Current;
		XMVECTOR dSqr = XMLoadFloat3(&Dist);
		dSqr = XMVector3LengthSq(dSqr);
		// If distance is too small, just set the desired location
		if (dSqr.m128_f32[0] < 0.05)
		{
			return Target;
		}

		//Delta Move, Clamp so we do not over shoot.
		const XMFLOAT3 DeltaMove = Dist * max(min(DeltaTime * InterpSpeed, 0.f), 1.0);

		return Current + DeltaMove;
	}

	void Camera::Update(float dt)
	{
		bool moving = false;
		XMFLOAT3 targetAdd = XMFLOAT3(0,0,0);
		targetAdd += dirs[0] ? m_Forward : XMFLOAT3(0, 0, 0);
		targetAdd -= dirs[1] ? m_Forward : XMFLOAT3(0, 0, 0);
		targetAdd += dirs[2] ? m_Up : XMFLOAT3(0, 0, 0);
		targetAdd -= dirs[3] ? m_Up : XMFLOAT3(0, 0, 0);
		targetAdd -= dirs[4] ? m_Right : XMFLOAT3(0, 0, 0);
		targetAdd += dirs[5] ? m_Right : XMFLOAT3(0, 0, 0);
		m_Target += targetAdd * m_Speed;
		for (size_t i = 0; i < 6; i++)
		{
			if (dirs[i])
			{
				moving = true;
			}
			dirs[i] = false;
		}
		m_Position = InterpTo(m_Position, m_Target, dt, 100);

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
				viewMatrix = XMMatrixLookToLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&m_Forward), XMLoadFloat3(&m_Up));
				invProjectionMatrix = XMMatrixInverse(nullptr, projectionMatrix);
				invViewMatrix = XMMatrixInverse(nullptr, viewMatrix);
			}
			else
			{
				projectionMatrix = XMMatrixOrthographicLH(m_OrthoWidth,m_OrthoHeight, 0.1f, 1000.0f);
				viewMatrix = XMMatrixLookToLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&m_Forward), XMLoadFloat3(&m_Up));
				invProjectionMatrix = XMMatrixInverse(nullptr, projectionMatrix);
				invViewMatrix = XMMatrixInverse(nullptr, viewMatrix);
			}

			XMStoreFloat4x4(&m_CameraConstantBufferData.projectionMatrix, (projectionMatrix));
			XMStoreFloat4x4(&m_CameraConstantBufferData.viewMatrix, (viewMatrix));
			XMStoreFloat4x4(&m_CameraConstantBufferData.invProjectionMatrix, (invProjectionMatrix));
			XMStoreFloat4x4(&m_CameraConstantBufferData.invViewMatrix, (invViewMatrix));
			m_CameraConstantBufferData.cameraPosition = XMFLOAT4(m_Position.x, m_Position.y, m_Position.z, 1.0);
			m_CameraConstantBufferData.cameraDir = XMFLOAT4(m_Forward.x, m_Forward.y, m_Forward.z, 0.0f);

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
	XMFLOAT4X4 Camera::GetOrthoProjectionMatrix(float nearPlane, float farPlane)
	{
		XMMATRIX proj = XMMatrixPerspectiveFovLH(m_FoV, m_AspectRatio, nearPlane, farPlane);
		XMFLOAT4X4 res;
		XMStoreFloat4x4(&res, proj);
		return res;
	}
}