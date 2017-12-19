#include "ThempSystem.h"
#include "ThempGame.h"
#include "ThempResources.h"
#include "ThempCamera.h"
#include "../Engine/ThempObject3D.h"
#include "../Engine/ThempD3D.h"
//#include "..\..\include\AccidentalNoise\anl_noise.h"

Themp::Camera* cam;
void Themp::Game::Start()
{
	//Themp::Object3D* Test3D = new Object3D();
	//Test3D->CreateCube("defaultSimple", true, true, false);
	//Test3D->m_Position = XMFLOAT3(10, 10, 0);
	//Test3D->m_Scale = XMFLOAT3(1, 1, 1);
	//Test3D->isDirty = true;
	 
	//m_Objects3D.push_back(Test3D);
	//Themp::Object3D* TestModel = Themp::System::tSys->m_Resources->LoadModel("sponza.bin");
	//m_Objects3D.push_back(TestModel);
	  
	Themp::Object3D* TestModel = Themp::System::tSys->m_Resources->LoadModel("elemental.bin");
	m_Objects3D.push_back(TestModel);
	 
	 
	TestModel->m_Position = XMFLOAT3(0, 0, 0);
	TestModel->m_Scale = XMFLOAT3(0.03f, 0.03f, 0.03f);

	//TestModel2->m_Position = XMFLOAT3(0, 0, 0);
	//TestModel2->m_Scale = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//TestModel2->m_Rotation = XMFLOAT3(XMConvertToRadians(0), 0, 0);

	TestModel->isDirty = true;


	cam = new Themp::Camera();
	cam->SetPosition(2.639275, 10.899924, -56.038624);
	cam->SetLookDir(-0.538092, -0.263115, -0.800767);
	cam->SetUpVector(0, 1, 0);
	cam->SetAspectRatio(Themp::System::tSys->m_SVars["WindowSizeX"] / Themp::System::tSys->m_SVars["WindowSizeY"]);
	cam->SetFoV(90.f);
	m_Camera = cam;
}

float totalMouseX=200, totalMouseY = -20;
float mouseSensitivity = 0.15f;

float lightPosTime = 0;
void Themp::Game::Update(double dt)
{
	cam->SetAspectRatio(Themp::System::tSys->m_SVars["WindowSizeX"] / Themp::System::tSys->m_SVars["WindowSizeY"]);
	lightPosTime += dt*2.0f;
	//m_Objects3D[0]->Update(dt);
	//printf("DT: %f \n", dt);


	//left mouse button
	if (m_Keys[1022])
	{
		totalMouseX += m_CursorDeltaX*mouseSensitivity;
		totalMouseY += m_CursorDeltaY*mouseSensitivity;

		totalMouseY = totalMouseY > 90 ? 90 : totalMouseY < -90 ? -90 : totalMouseY;
		cam->Rotate((totalMouseX), totalMouseY);
	}

	if (m_Keys['W'])
	{
		cam->MoveForward(dt * 10);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['S'])
	{
		cam->MoveBackward(dt * 10);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['A'])
	{
		cam->MoveLeft(dt * 10);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['D'])
	{
		cam->MoveRight(dt * 10);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	//spacebar
	if (m_Keys[32])
	{
		cam->MoveUp(dt * 10);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['X'])
	{
		cam->MoveDown(dt * 10);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['Y'])
	{
		printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
		printf("LookDir: X:%f Y:%f Z:%f\n", cam->m_LookDirection.x, cam->m_LookDirection.y, cam->m_LookDirection.z);
	}

	//Themp::System::tSys->m_D3D->constantBufferData.x = sin(lightPosTime*0.1) * 50.0;
	//Themp::System::tSys->m_D3D->constantBufferData.y = 10;
	//Themp::System::tSys->m_D3D->constantBufferData.z = cos(lightPosTime*0.3) * 30.0;
	//Themp::System::tSys->m_D3D->constantBufferData.w = 1.0f;
	//Themp::System::tSys->m_D3D->constantBufferData.r = sin(lightPosTime);
	//Themp::System::tSys->m_D3D->constantBufferData.g = 1.0f;
	//Themp::System::tSys->m_D3D->constantBufferData.b = cos(lightPosTime);
	//Themp::System::tSys->m_D3D->constantBufferData.a = 1.0f;
	//Themp::System::tSys->m_D3D->dirtySystemBuffer = true;
	//m_Objects3D[0]->m_Position.x = sin(lightPosTime*0.1) * 50.0;
	//m_Objects3D[0]->m_Position.y = 10;
	//m_Objects3D[0]->m_Position.z = cos(lightPosTime*0.3) * 30.0;
	cam->UpdateMatrices();
}

void Themp::Game::Stop()
{
	delete m_Camera;
	m_Camera = nullptr;
	for (int i = 0; i < m_Objects3D.size(); i++)
	{
		m_Objects3D[i] = nullptr;
	}
	m_Objects3D.clear();
}