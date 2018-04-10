#include "ThempSystem.h"
#include "ThempGame.h"
#include "ThempResources.h"
#include "../Library/imgui.h"
#include "../Engine/ThempCamera.h"
#include "../Engine/ThempObject3D.h"
#include "../Engine/ThempMesh.h"
#include "../Engine//ThempMaterial.h"
#include "../Engine/ThempD3D.h"
#include <DirectXMath.h>
//#include "..\..\include\AccidentalNoise\anl_noise.h"

float totalMouseX = 200, totalMouseY = -20;
float mouseSensitivity = 0.15f;

bool isMetallic = false;
int currentRenderListItem = 0;
int currentShadowListItem = 0;
float cTime = 0;
float DirLightAngle = 0;
bool staticLight = false;
bool movingLight = false;
int numMSAA = 1;



void Themp::Game::Start()
{
	numMSAA = Themp::System::tSys->m_SVars[std::string("Multisample")];
	//Themp::Object3D* Test3D = new Object3D();
	//Test3D->CreateCube("defaultSimple", true, true, false);
	//Test3D->m_Position = XMFLOAT3(10, 10, 0);
	//Test3D->m_Scale = XMFLOAT3(1, 1, 1);
	//Test3D->isDirty = true;
	 
	//m_Objects3D.push_back(Test3D);
	//Themp::Object3D* TestModel = Themp::System::tSys->m_Resources->GetModel("sponza.bin");
	//m_Objects3D.push_back(TestModel);

	Themp::Object3D* TestModel = Themp::System::tSys->m_Resources->GetModel("elemental.bin");
	m_Objects3D.push_back(TestModel);
	Themp::Object3D* dragon = Themp::System::tSys->m_Resources->GetModel("dragon.bin");
	m_Objects3D.push_back(dragon);
	dragon->m_Position = XMFLOAT3(83.288498f, 26.583342f, -213.086884f);
	dragon->m_Scale = XMFLOAT3(2.0,2.0,2.0);
	 
	TestModel->m_Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	TestModel->m_Scale = XMFLOAT3(0.03f, 0.03f, 0.03f);
	Themp::Object3D* TestModel2 = Themp::System::tSys->m_Resources->GetModel("sphere.bin");

	//albedo + roughness
	std::vector<std::string> textures = { "Sphere/f0.dds","Sphere/earth.dds" };
	std::vector<std::uint8_t> types = { Material::DIFFUSE,Material::PBR };
	Material* m = Themp::System::tSys->m_Resources->GetMaterial(std::string("Earth"), textures, types, "None", false, false, false);
	TestModel2->m_Meshes[0]->m_Material = m;
	TestModel2->m_Position = XMFLOAT3(0.082245f, 6.632223f, -72.250923f);
	m_Objects3D.push_back(TestModel2);
	
	textures[0] = "dragon/albedo.dds";
	textures[1] = ""; 
	types[0] = Material::DIFFUSE;
	types[1] = Material::UNUSED;
	dragon->m_Meshes[0]->m_Material = Themp::System::tSys->m_Resources->GetMaterial("Dragon", textures, types, "", false, false, false);
	dragon->m_Meshes[0]->m_Material->GetMaterialProperties("dragon/DefaultMaterial");
	dragon->m_Meshes[0]->m_Material->UpdateBuffer();
	char itoaBuf[16];
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			Themp::Object3D* TestModels = Themp::System::tSys->m_Resources->GetModel("sphere.bin",true);
			Material* mm = Themp::System::tSys->m_Resources->GetMaterial(std::string("TestSphere").append(_itoa(i+j*10, itoaBuf, 10)), "Sphere/cielingBeams_cielingBeams_D.dds", "None", false, false, false);
			TestModels->m_Position = XMFLOAT3(((float)i-5.0f)*6.0f  , 20, ((float)j-5.0f)*6.0f - 200.0f);
			mm->m_MaterialConstantBufferData.Metallic = 1.0f * ((float)i / 10.0f);
			mm->m_MaterialConstantBufferData.Roughness =  1.0f * ((float)j / 10.0f);
			mm->UpdateBuffer();
			TestModels->m_Meshes[0]->m_Material = mm;
			m_Objects3D.push_back(TestModels);
		}
	}
	//TestModel2->m_Scale = XMFLOAT3(0.01f, 0.01f, 0.01f);
	//TestModel2->m_Rotation = XMFLOAT3(XMConvertToRadians(0), 0, 0);
	TestModel2->isDirty = true;
	TestModel->isDirty = true;

	m_Camera = new Themp::Camera();
	m_Camera->SetPosition(2.639275f, 10.899924f, -56.038624f);
	m_Camera->Rotate(0, 0);
	m_Camera->SetAspectRatio(Themp::System::tSys->m_SVars["WindowSizeX"] / Themp::System::tSys->m_SVars["WindowSizeY"]);
	m_Camera->SetFoV(90.f);

	Themp::System::tSys->m_D3D->m_ConstantBufferData.F0x = 0.55f;
	Themp::System::tSys->m_D3D->m_ConstantBufferData.F0y = 0.55f;
	Themp::System::tSys->m_D3D->m_ConstantBufferData.F0z = 1.0f;


	Themp::Object3D* cylinder = Themp::System::tSys->m_Resources->GetModel("HD_Cylinder.bin");
	m_Objects3D.push_back(cylinder);
	cylinder->m_Position = XMFLOAT3(122.024445, 54.588993, -138.465500);
	cylinder->m_Scale = XMFLOAT3(10, 10, 10);

	if (currentShadowListItem == 0)
	{
		numMSAA = 1;
		Themp::System::tSys->m_SVars["Multisample"] = numMSAA;
		Themp::System::tSys->m_D3D->SetShadowType(currentShadowListItem);
		Themp::System::tSys->m_D3D->SetMultiSample(numMSAA);
	}

	D3D::s_D3D->AddDirectionalLight(XMFLOAT4(4.258440f, 42.606735f, -98.248665f, 1.0f), XMFLOAT4(-0.621556f, -0.783368f, 0.001628f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 4096);
}

DirectX::XMFLOAT3 Normalize(DirectX::XMFLOAT3 v)
{
	DirectX::XMVECTOR x = XMLoadFloat3(&v);
	x = DirectX::XMVector3Normalize(x);
	DirectX::XMFLOAT3 r;
	XMStoreFloat3(&r, x);
	return r;
}
DirectX::XMFLOAT3 Add(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
{
	DirectX::XMFLOAT3 r = DirectX::XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z);
	return r;
}
DirectX::XMFLOAT4 Add4(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b)
{
	DirectX::XMFLOAT4 r = DirectX::XMFLOAT4(a.x + b.x, a.y + b.y, a.z + b.z,1.0);
	return r;
}

void Themp::Game::Update(double dt)
{
	cTime += (float)dt;
	m_Camera->SetAspectRatio(Themp::System::tSys->m_SVars["WindowSizeX"] / Themp::System::tSys->m_SVars["WindowSizeY"]);

	m_Objects3D[m_Objects3D.size()-1]->Update(dt);
	//printf("DT: %f \n", dt);
	bool changedAValue = false;
	
	if (ImGui::DragFloat("Skybox Brightness", &Themp::System::tSys->m_D3D->m_ConstantBufferData.F0x, 0.005f, 0.005f, 10.0f))
	{
		changedAValue = true;
	}
	if (ImGui::DragFloat("IBL Brightness", &Themp::System::tSys->m_D3D->m_ConstantBufferData.F0y, 0.005f, 0.005f, 10.0f))
	{
		changedAValue = true;
	}
	if (ImGui::DragFloat("Light Strength", &Themp::System::tSys->m_D3D->m_ConstantBufferData.F0z, 0.005f, 0.005f, 10.0f))
	{
		changedAValue = true;
	}
	if (ImGui::DragFloat("Skybox Lerp",&Themp::System::tSys->m_D3D->m_ConstantBufferData.SkyboxLerp,0.05f,0.0f,1.0f))
	{
		//Themp::System::tSys->m_D3D->m_ConstantBufferData.globalMetallic = isMetallic ? 1.0 : 0.0f;
		changedAValue = true;
	}
	if (ImGui::Button("Set Light to Camera view"))
	{
		XMFLOAT3 p = m_Camera->GetPosition();
		XMFLOAT3 f = m_Camera->GetForward();
		D3D::s_D3D->SetDirectionalLight(0, XMFLOAT4(p.x, p.y, p.z, 1.0f), XMFLOAT4(f.x, f.y, f.z, 0.0f),XMFLOAT4(1,1,1,1));
		D3D::s_D3D->SetLightDirty(D3D::LightType::Directional, 0);
	}
	if (ImGui::Checkbox("Moving Light", &movingLight))
	{}

	if (movingLight)
	{
		XMFLOAT3 lPos = XMFLOAT3(sin(DirLightAngle) * 200.0f, cos(DirLightAngle) * 200.0f, 0);
		XMFLOAT4 nPos = Add4(m_Camera->GetPosition(), lPos);

		XMFLOAT3 v = XMFLOAT3(sin(DirLightAngle) * 200.0f, cos(DirLightAngle) * 200.0f, -30.0f);
		XMVECTOR x = XMLoadFloat3(&v);
		x = XMVector3Normalize(x);
		XMStoreFloat3(&v, x);
		XMFLOAT4 nDir = XMFLOAT4(-v.x, -v.y, -v.z, .0f);
		XMFLOAT3 dotDir = XMFLOAT3(0, -1, 0);
		XMVECTOR a = XMLoadFloat3(&dotDir);
		XMVECTOR b = XMLoadFloat4(&nDir);
		XMVECTOR res = DirectX::XMVector3Dot(a, b);
		float dotResult = max(res.m128_f32[0], 0.0f);
		Themp::System::tSys->m_D3D->m_ConstantBufferData.F0x = 1.4 - (dotResult * 0.85);
		Themp::System::tSys->m_D3D->m_ConstantBufferData.F0y = 1.4 - (dotResult * 0.85);

		D3D::s_D3D->SetDirectionalLight(0, nPos, XMFLOAT4(-v.x, -v.y, -v.z, .0f), XMFLOAT4(1, 1, 1, dotResult));
		
		changedAValue = true;
		DirLightAngle += dotResult > 0.0 ? dt * 0.2 : dt;
	}

	const char* renderTypes[9] = { "Complete","D Function","F Function","G Function" ,"IBL" ,"Directional PBR","Metallic","Roughness","Albedo"};
	if (ImGui::ListBox("Render type", &currentRenderListItem, renderTypes, 9))
	{
		changedAValue = true;
		Themp::System::tSys->m_D3D->m_ConstantBufferData.visualType = (float)currentRenderListItem;
	}
	
	const char* shadowTypes[5] = { "No Filter","Percentage-Closer","Variance","Cascaded" ,"Moment" };
	if (ImGui::ListBox("Shadow type", &currentShadowListItem, shadowTypes, 5))
	{
		changedAValue = true;
		Themp::System::tSys->m_D3D->m_ConstantBufferData.shadowType = (float)currentShadowListItem;
		Themp::System::tSys->m_D3D->SetShadowType(currentShadowListItem);
		if (currentShadowListItem == 0)
		{
			numMSAA = 1; 
			Themp::System::tSys->m_SVars["Multisample"] = numMSAA;
			Themp::System::tSys->m_D3D->SetMultiSample(numMSAA);
		}
		else
		{
			if (numMSAA == 1)
			{
				numMSAA = 2;
				Themp::System::tSys->m_SVars["Multisample"] = numMSAA;
				Themp::System::tSys->m_D3D->SetMultiSample(numMSAA);
			}
		}
	}
	ImGui::BeginMainMenuBar();

	ImGui::PushID(1);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7, 0.1, 0.1, 0.8));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9, 0.1, 0.1, 1.0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0, 0.0, 0.0, 1.0));
	if (ImGui::Button("Quit",ImVec2(60,20)))
	{
		Themp::System::tSys->m_Quitting = true;
	}
	ImGui::PopStyleColor(3);
	ImGui::PopID();
	const char* MSAA[4] = { "x1","x2","x4","x8" };
	if (ImGui::Button(MSAA[numMSAA == 1 ? 0 : numMSAA == 2 ? 1 : numMSAA == 4 ? 2 : 3], ImVec2(20, 20)))
	{
		if (Themp::System::tSys->m_D3D->m_ShadowType == 0)
		{
			numMSAA = 1;
		}
		else
		{
			numMSAA = (numMSAA == 1 ? 2 : numMSAA == 2 ? 4 : numMSAA == 4 ? 8 : 2);
		}
		Themp::System::tSys->m_SVars["Multisample"] = numMSAA;
		if (Themp::System::tSys->m_D3D->SetMultiSample(numMSAA))
		{
			System::Print("MSAA set to x%i", numMSAA);
		}
		else
		{
			System::Print("Failed to set MSAA to x%i", numMSAA);
		}
	}
	if (ImGui::Checkbox("Wireframe", &Themp::System::tSys->m_D3D->m_Wireframe))
	{

	}
	ImGui::EndMainMenuBar();

	if (changedAValue)
	{
		Themp::System::tSys->m_D3D->dirtySystemBuffer = true;
	}

	//left mouse button
	if (m_Keys[1023])
	{
		totalMouseX += m_CursorDeltaX*mouseSensitivity;
		totalMouseY += m_CursorDeltaY*mouseSensitivity;

		totalMouseY = totalMouseY > 90.0f ? 90.0f : totalMouseY < -90.0f ? -90.0f : totalMouseY;
		m_Camera->Rotate(totalMouseX, totalMouseY);
	}
	double speedMod = 0.3;
	if (m_Keys[VK_SHIFT])
	{
		speedMod = 2.0;
	}
	m_Camera->SetSpeed(speedMod);
	if (m_Keys['W'])
	{
		m_Camera->MoveForward();
		//printf("Position: X:%f Y:%f Z:%f\n", m_Camera->m_Position.x, m_Camera->m_Position.y, m_Camera->m_Position.z);
	}
	if (m_Keys['S'])
	{
		m_Camera->MoveBackward();
		//printf("Position: X:%f Y:%f Z:%f\n", m_Camera->m_Position.x, m_Camera->m_Position.y, m_Camera->m_Position.z);
	}
	if (m_Keys['A'])
	{
		m_Camera->MoveLeft();
		//printf("Position: X:%f Y:%f Z:%f\n", m_Camera->m_Position.x, m_Camera->m_Position.y, m_Camera->m_Position.z);
	}
	if (m_Keys['D'])
	{
		m_Camera->MoveRight();
		//printf("Position: X:%f Y:%f Z:%f\n", m_Camera->m_Position.x, m_Camera->m_Position.y, m_Camera->m_Position.z);
	}
	//spacebar
	if (m_Keys[32] || m_Keys['E'])
	{
		m_Camera->MoveUp();
		//printf("Position: X:%f Y:%f Z:%f\n", m_Camera->m_Position.x, m_Camera->m_Position.y, m_Camera->m_Position.z);
	}
	if (m_Keys['X'] || m_Keys['Q'])
	{
		m_Camera->MoveDown();
		//printf("Position: X:%f Y:%f Z:%f\n", m_Camera->m_Position.x, m_Camera->m_Position.y, m_Camera->m_Position.z);
	}
	if (m_Keys['Y'])
	{
		XMFLOAT3 p = m_Camera->GetPosition();
		XMFLOAT3 f = m_Camera->GetForward();
		System::Print("Position: X:%f Y:%f Z:%f\n", p.x, p.y, p.z);
		System::Print("LookDir: X:%f Y:%f Z:%f\n", f.x, f.y, f.z);
	}

	//Themp::System::tSys->m_D3D->m_LightConstantBufferData.dirLights[0].direction = XMFLOAT4(sin(lightPosTime), -1.0, cos(lightPosTime), 0.0);
	
	//Themp::System::tSys->m_D3D->dirtySystemBuffer = true;
	//m_Objects3D[0]->m_Position.x = sin(lightPosTime*0.1) * 50.0;
	//m_Objects3D[0]->m_Position.y = 10;
	//m_Objects3D[0]->m_Position.z = cos(lightPosTime*0.3) * 30.0;
	m_Camera->Update(dt);
	m_Camera->UpdateMatrices();
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