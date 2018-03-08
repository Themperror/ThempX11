#include "ThempSystem.h"
#include "ThempGame.h"
#include "ThempResources.h"
#include "ThempCamera.h"
#include "../Library/imgui.h"
#include "../Engine/ThempObject3D.h"
#include "../Engine/ThempMesh.h"
#include "../Engine//ThempMaterial.h"
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
	Themp::Object3D* TestModel2 = Themp::System::tSys->m_Resources->LoadModel("sphere.bin");

	//albedo + roughness
	std::vector<std::string> textures = { "Sphere/f0.dds","Sphere/earth.dds" };
	std::vector<std::uint8_t> types = { Material::DIFFUSE,Material::PBR };
	Material* m = Themp::System::tSys->m_Resources->LoadMaterial(std::string("Earth"), textures, types, "None", false, false, false);
	TestModel2->m_Meshes[0]->m_Material = m;
	TestModel2->m_Position = XMFLOAT3(0.082245, 6.632223, -72.250923);
	m_Objects3D.push_back(TestModel2);


	char itoaBuf[16];
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			Themp::Object3D* TestModels = Themp::System::tSys->m_Resources->LoadModel("sphere.bin");
			Material* m = Themp::System::tSys->m_Resources->LoadMaterial(std::string("TestSphere").append(_itoa(i*j, itoaBuf, 10)), "Sphere/cielingBeams_cielingBeams_D.dds", "None", false, false, false);
			TestModels->m_Position = XMFLOAT3((i-5)*6  , 20, (j-5)*6 - 200);
			m->m_MaterialConstantBufferData.Metallic = 1.0 * ((float)i / 10.0);
			m->m_MaterialConstantBufferData.Roughness = 1.0 * ((float)j / 10.0);
			m->UpdateBuffer();
			TestModels->m_Meshes[0]->m_Material = m;
			m_Objects3D.push_back(TestModels);
		}
	}
	//TestModel2->m_Scale = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//TestModel2->m_Rotation = XMFLOAT3(XMConvertToRadians(0), 0, 0);
	TestModel2->isDirty = true;
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
bool isMetallic = false;
int currentListItem = 0;
void Themp::Game::Update(double dt)
{
	cam->SetAspectRatio(Themp::System::tSys->m_SVars["WindowSizeX"] / Themp::System::tSys->m_SVars["WindowSizeY"]);
	lightPosTime += dt*2.0f;
	//m_Objects3D[0]->Update(dt);
	//printf("DT: %f \n", dt);
	bool changedAValue = false;
	
	if (ImGui::SliderFloat("Roughness", &Themp::System::tSys->m_D3D->m_ConstantBufferData.globalRoughness, 0.05, 1.0))
	{
		changedAValue = true;
	}
	if (ImGui::DragFloat3("F0", &Themp::System::tSys->m_D3D->m_ConstantBufferData.F0x, 0.005f, 0.0f, 1.0F, "%.3f", 1.0f))
	{
		changedAValue = true;
	}
	if (ImGui::Checkbox("Metallic", &isMetallic))
	{
		Themp::System::tSys->m_D3D->m_ConstantBufferData.globalMetallic = isMetallic ? 1.0 : 0.0f;
		changedAValue = true;
	}
	const char* renderTypes[8] = { "Complete","D Function","F Function","G Function" ,"IBL" ,"Directional PBR","unused","unused"};
	if (ImGui::ListBox("Render type", &currentListItem, renderTypes, 8))
	{
		changedAValue = true;
		Themp::System::tSys->m_D3D->m_ConstantBufferData.visualType = currentListItem;
	}
	
	if (changedAValue)
	{
		Themp::System::tSys->m_D3D->dirtySystemBuffer = true;
	}

	//left mouse button
	if (m_Keys[1022] && !Themp::System::tSys->m_CursorShown)
	{
		totalMouseX += m_CursorDeltaX*mouseSensitivity;
		totalMouseY += m_CursorDeltaY*mouseSensitivity;

		totalMouseY = totalMouseY > 90 ? 90 : totalMouseY < -90 ? -90 : totalMouseY;
		cam->Rotate((totalMouseX), totalMouseY);
	}
	float speedMod = 1.0;
	if (m_Keys[VK_SHIFT])
	{
		speedMod = 10.0;
	}
	if (m_Keys['W'])
	{
		cam->MoveForward(dt * 10 * speedMod);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['S'])
	{
		cam->MoveBackward(dt * 10 * speedMod);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['A'])
	{
		cam->MoveLeft(dt * 10 * speedMod);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['D'])
	{
		cam->MoveRight(dt * 10 * speedMod);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	//spacebar
	if (m_Keys[32])
	{
		cam->MoveUp(dt * 10 * speedMod);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['X'])
	{
		cam->MoveDown(dt * 10 * speedMod);
		//printf("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
	}
	if (m_Keys['Y'])
	{
		System::Print("Position: X:%f Y:%f Z:%f\n", cam->m_Position.x, cam->m_Position.y, cam->m_Position.z);
		System::Print("LookDir: X:%f Y:%f Z:%f\n", cam->m_LookDirection.x, cam->m_LookDirection.y, cam->m_LookDirection.z);
	}

	//Themp::System::tSys->m_D3D->m_LightConstantBufferData.dirLights[0].direction = XMFLOAT4(sin(lightPosTime), -1.0, cos(lightPosTime), 0.0);
	
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