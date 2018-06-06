#include "ThempSystem.h"
#include "ThempGame.h"
#include "ThempResources.h"
#include "../Library/imgui.h"
#include "../Engine/ThempCamera.h"
#include "../Engine/ThempObject3D.h"
#include "../Engine/ThempMesh.h"
#include "../Engine/ThempMaterial.h"
#include "../Engine/ThempD3D.h"
#include "../Engine/ThempFunctions.h"
#include "../Engine/ThempDebugDraw.h"
#include "../Engine/Shadowing/ThempShadowCascade.h"
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
bool drawLightPositions = false;

bool lightEnabled[3];
XMFLOAT3 lightposition[3];
XMFLOAT3 lightdirection[3];
XMFLOAT4 lightcolor[3];

int numMSAA = 1;

Themp::Object3D* rotatingEarth = nullptr;


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

	dragon->m_Position = XMFLOAT3(148.453445 , 71.424187 ,-695.424377);
	dragon->m_Scale = XMFLOAT3(5.0, 5.0, 5.0);

	Themp::Object3D* dragonSmooth = Themp::System::tSys->m_Resources->GetModel("dragonSmooth.bin");
	m_Objects3D.push_back(dragonSmooth);

	dragonSmooth->m_Position = XMFLOAT3(153.136673,62.182076, -856.141846);
	dragonSmooth->m_Scale = XMFLOAT3(5.0,5.0,5.0);
	 
	TestModel->m_Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	TestModel->m_Scale = XMFLOAT3(0.15f, 0.15f, 0.15f);
	Themp::Object3D* TestModel2 = Themp::System::tSys->m_Resources->GetModel("sphere.bin",true);

	//albedo + roughness
	std::vector<std::string> textures = { "Sphere/f0.dds","Sphere/earth.dds" };
	std::vector<std::uint8_t> types = { Material::DIFFUSE,Material::PBR };
	Material* m = Themp::System::tSys->m_Resources->GetMaterial(std::string("Earth"), textures, types, "", false);
	TestModel2->m_Meshes[0]->m_Material = m;
	TestModel2->m_Position = XMFLOAT3(0.082245f, 20.632223f, -72.250923f);
	TestModel2->m_Scale = XMFLOAT3(3, 3, 3);
	rotatingEarth = TestModel2;
	m_Objects3D.push_back(TestModel2);
	
	textures[0] = "dragon/albedo.dds";
	textures[1] = "";
	types[0] = Material::DIFFUSE;
	types[1] = Material::UNUSED;
	dragon->m_Meshes[0]->m_Material = Themp::System::tSys->m_Resources->GetMaterial("Dragon", textures, types, "", false);
	dragon->m_Meshes[0]->m_Material->GetMaterialProperties("dragon/DefaultMaterial");
	dragon->m_Meshes[0]->m_Material->UpdateBuffer();
	textures[0] = "dragonSmooth/albedo.dds";
	textures[1] = "dragonSmooth/materials/pbrmat.dds";
	types[0] = Material::DIFFUSE;
	types[1] = Material::PBR;
	dragonSmooth->m_Meshes[0]->m_Material = Themp::System::tSys->m_Resources->GetMaterial("DragonSmooth", textures, types, "Lava", false);
	dragonSmooth->m_Meshes[0]->m_Material->GetMaterialProperties("dragonSmooth/DefaultMaterial", &textures[1]);
	dragonSmooth->m_Meshes[0]->m_Material->UpdateBuffer();
	char itoaBuf[16];
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			Themp::Object3D* TestModels = Themp::System::tSys->m_Resources->GetModel("sphere.bin",true);
			Material* mm = Themp::System::tSys->m_Resources->GetMaterial(std::string("TestSphere").append(_itoa(i+j*10, itoaBuf, 10)), "Sphere/cielingBeams_cielingBeams_D.dds", "", false);
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
	m_Camera->SetPosition(-0.100051 , 43.199852, -273.101563);
	m_Camera->Rotate(180, 0);
	m_Camera->SetAspectRatio(Themp::System::tSys->m_SVars["WindowSizeX"] / Themp::System::tSys->m_SVars["WindowSizeY"]);
	m_Camera->SetFoV(75);
	m_Camera->SetNear(0.1f);
	m_Camera->SetFar(1000.0f);

	Themp::System::tSys->m_D3D->m_ConstantBufferData.F0x = 0.55f;
	Themp::System::tSys->m_D3D->m_ConstantBufferData.F0y = 0.55f;
	Themp::System::tSys->m_D3D->m_ConstantBufferData.F0z = 1.0f;


	Themp::Object3D* cylinder = Themp::System::tSys->m_Resources->GetModel("HD_Cylinder.bin");
	m_Objects3D.push_back(cylinder);
	cylinder->m_Position = XMFLOAT3(131.816391, 45.714542, -580.089417);
	cylinder->m_Scale = XMFLOAT3(10, 10, 10);

	for (size_t i = 0; i < 3; i++)
	{
		lightposition[i] = XMFLOAT3(4.258440f, 42.606735f, -98.248665f);
		lightdirection[i] = XMFLOAT3(-0.621556f, -0.783368f, 0.001628f);
		lightcolor[i] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		lightEnabled[i] = false;
	}

	if (currentShadowListItem == 0)
	{
		numMSAA = 1;
		Themp::System::tSys->m_SVars["Multisample"] = numMSAA;
		Themp::System::tSys->m_D3D->SetShadowType(currentShadowListItem);
		Themp::System::tSys->m_D3D->SetMultiSample(numMSAA);
	}
	lightposition[0] = XMFLOAT3(4.258440f, 42.606735f, -98.248665f);
	lightdirection[0] = XMFLOAT3(-0.621556f, -0.783368f, 0.001628f);
	lightcolor[0] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lightEnabled[0] = true;
	D3D::s_D3D->SetDirectionalLight(0, lightEnabled[0], XMFLOAT4(lightposition[0].x, lightposition[0].y, lightposition[0].z, 1.0), XMFLOAT4(lightdirection[0].x, lightdirection[0].y, lightdirection[0].z, 0.0f), lightcolor[0]);
}
void Themp::Game::Update(double dt)
{
	DebugDraw::Update((float)dt);
	cTime += (float)dt;
	m_Camera->SetAspectRatio(Themp::System::tSys->m_SVars["WindowSizeX"] / Themp::System::tSys->m_SVars["WindowSizeY"]);

	m_Objects3D[m_Objects3D.size() - 1]->Update(dt);
	rotatingEarth->Update(dt);
	//printf("DT: %f \n", dt);
	bool changedAValue = false;


	if (ImGui::DragFloat("Brightness", &Themp::System::tSys->m_D3D->m_ConstantBufferData.F0y, 0.005f, 0.005f, 10.0f))
	{
		Themp::System::tSys->m_D3D->m_ConstantBufferData.F0x = Themp::System::tSys->m_D3D->m_ConstantBufferData.F0y;
		changedAValue = true;
	}
	if (ImGui::DragFloat("Light Strength", &Themp::System::tSys->m_D3D->m_ConstantBufferData.F0z, 0.005f, 0.005f, 10.0f))
	{
		changedAValue = true;
	}
	if (ImGui::DragFloat("Skybox Lerp", &Themp::System::tSys->m_D3D->m_ConstantBufferData.SkyboxLerp, 0.05f, 0.0f, 1.0f))
	{
		//Themp::System::tSys->m_D3D->m_ConstantBufferData.globalMetallic = isMetallic ? 1.0 : 0.0f;
		changedAValue = true;
	}
	ImGui::Checkbox("Day/Night Cycle", &movingLight);
	ImGui::Checkbox("Draw Light Positions", &drawLightPositions);

	if (drawLightPositions)
	{
		for (size_t i = 0; i < 3; i++)
		{
			if (!lightEnabled[i]) continue;
			XMFLOAT3 l = lightposition[i];
			XMFLOAT3 c = XMFLOAT3(lightcolor[i].x, lightcolor[i].y, lightcolor[i].z);
			DebugDraw::Line(l - XMFLOAT3(1, 0, 0), l + XMFLOAT3(1, 0, 0),0.0,c);
			DebugDraw::Line(l - XMFLOAT3(0, 1, 0), l + XMFLOAT3(0, 1, 0),0.0,c);
			DebugDraw::Line(l - XMFLOAT3(0, 0, 1), l + XMFLOAT3(0, 0, 1),0.0,c);
		}
	}
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
		float dotResult = std::max(res.m128_f32[0], 0.0f);
		Themp::System::tSys->m_D3D->m_ConstantBufferData.F0x = 1.4 - (dotResult * 0.85);
		Themp::System::tSys->m_D3D->m_ConstantBufferData.F0y = 1.4 - (dotResult * 0.85);

		lightposition[0] = ToXMFLOAT3(nPos);
		lightdirection[0] = ToXMFLOAT3(XMFLOAT4(-v.x, -v.y, -v.z, .0f));
		lightcolor[0] = XMFLOAT4(1, 1, 1, dotResult);

		D3D::s_D3D->SetDirectionalLight(0,true, nPos, XMFLOAT4(-v.x, -v.y, -v.z, .0f), XMFLOAT4(1, 1, 1, dotResult));

		changedAValue = true;
		DirLightAngle += dotResult > 0.0 ? dt * 0.2 : dt;
	}
	
	if (ImGui::CollapsingHeader("Render Types"))
	{
		const char* renderTypes[10] = { "Complete","D Function","F Function","G Function" ,"IBL" ,"Directional PBR","Metallic","Roughness","Albedo","Normal" };
		if (ImGui::ListBox("Render type", &currentRenderListItem, renderTypes, 10))
		{
			changedAValue = true;
			//set visual type
			Themp::System::tSys->m_D3D->m_ConstantBufferData.visualType = (float)currentRenderListItem;

			//set shadow type back to unfiltered (this one has the debug views)
			Themp::System::tSys->m_D3D->SetShadowType(0);

			//reset values so we don't mismatch any types (Multisample & No Multisample)
			numMSAA = 1;
			Themp::System::tSys->m_SVars["Multisample"] = numMSAA;
			Themp::System::tSys->m_D3D->SetMultiSample(numMSAA);

			//set lights (we switched shadow types)
			for (size_t i = 0; i < 3; i++)
			{
				XMFLOAT3 lDir = Normalize(lightdirection[i]);
				D3D::s_D3D->SetDirectionalLight(i, lightEnabled[i], XMFLOAT4(lightposition[i].x, lightposition[i].y, lightposition[i].z, 1.0), XMFLOAT4(lDir.x, lDir.y, lDir.z, 0.0f), lightcolor[i]);
				D3D::s_D3D->SetLightDirty(D3D::LightType::Directional, i);
			}
		}
	}
	if (ImGui::CollapsingHeader("Shadow Types"))
	{
		const char* shadowTypes[5] = { "No Filter","Percentage-Closer","Cascaded","Variance" ,"Moment" };
		if (ImGui::ListBox("Shadow type", &currentShadowListItem, shadowTypes, 5))
		{
			changedAValue = true;
			Themp::System::tSys->m_D3D->m_ConstantBufferData.shadowType = (float)currentShadowListItem;
			Themp::System::tSys->m_D3D->SetShadowType(currentShadowListItem);

			if (currentShadowListItem == 0) //unfiltered
			{
				numMSAA = 1;
				Themp::System::tSys->m_SVars["Multisample"] = numMSAA;
				Themp::System::tSys->m_D3D->SetMultiSample(numMSAA);
			}
			else if (currentShadowListItem == 1) //PCF (Multisampled)
			{
				if (numMSAA == 1)
				{
					numMSAA = 2;
					Themp::System::tSys->m_SVars["Multisample"] = numMSAA;
					Themp::System::tSys->m_D3D->SetMultiSample(numMSAA);
				}
			}
			else //Anything else
			{
				numMSAA = 1;
				Themp::System::tSys->m_SVars["Multisample"] = numMSAA;
				Themp::System::tSys->m_D3D->SetMultiSample(numMSAA);
			}
			//set Lights up again
			for (size_t i = 0; i < 3; i++)
			{
				XMFLOAT3 lDir = Normalize(lightdirection[i]);
				D3D::s_D3D->SetDirectionalLight(i, lightEnabled[i], XMFLOAT4(lightposition[i].x, lightposition[i].y, lightposition[i].z, 1.0), XMFLOAT4(lDir.x, lDir.y, lDir.z, 0.0f), lightcolor[i]);
				D3D::s_D3D->SetLightDirty(D3D::LightType::Directional, i);
			}
		}
	}
	if (currentShadowListItem == 2)
	{
		if (ImGui::CollapsingHeader("Cascade Shadow variables"))
		{
			if (ImGui::SliderFloat("Cascade Num", &D3D::s_D3D->m_ConstantBufferData.num_cascades, 2, 6, "%.0f"))
			{
				changedAValue = true;
				D3D::s_D3D->SetNumberCascades(D3D::s_D3D->m_ConstantBufferData.num_cascades);
			}
			ImGui::SliderFloat("Cascade Bounds 0", &ShadowCascade::CascadeBounds[0], 10, 300.0f);
			ImGui::SliderFloat("Cascade Bounds 1", &ShadowCascade::CascadeBounds[1], 10, 300.0f);
			ImGui::SliderFloat("Cascade Bounds 2", &ShadowCascade::CascadeBounds[2], 10, 300.0f);
			ImGui::SliderFloat("Cascade Bounds 3", &ShadowCascade::CascadeBounds[3], 10, 300.0f);
			ImGui::SliderFloat("Cascade Bounds 4", &ShadowCascade::CascadeBounds[4], 10, 300.0f);
			ImGui::SliderFloat("Cascade Bounds 5", &ShadowCascade::CascadeBounds[5], 10, 300.0f);

			float nearplane = m_Camera->GetNear();
			ImGui::SliderFloat("Camera Nearplane", &nearplane, 0.5f, 50.0f);
			m_Camera->SetNear(nearplane);

			float farplane = m_Camera->GetFar();
			ImGui::SliderFloat("Camera Farplane", &farplane, 51.0f, 1000.0f,"%.3f",5.0f);
			m_Camera->SetFar(farplane);

			//make sure you can't set any of the lower or higher than the previous/next one

			////low bounds
			//ShadowCascade::CascadePartitions[1] = ShadowCascade::CascadePartitions[1] < ShadowCascade::CascadePartitions[0] ? ShadowCascade::CascadePartitions[0] + 1.0f : ShadowCascade::CascadePartitions[1];
			//ShadowCascade::CascadePartitions[2] = ShadowCascade::CascadePartitions[2] < ShadowCascade::CascadePartitions[1] ? ShadowCascade::CascadePartitions[1] + 1.0f : ShadowCascade::CascadePartitions[2];
			//ShadowCascade::CascadePartitions[3] = ShadowCascade::CascadePartitions[3] < ShadowCascade::CascadePartitions[2] ? ShadowCascade::CascadePartitions[2] + 1.0f : ShadowCascade::CascadePartitions[3];
			//ShadowCascade::CascadePartitions[4] = ShadowCascade::CascadePartitions[4] < ShadowCascade::CascadePartitions[3] ? ShadowCascade::CascadePartitions[3] + 1.0f : ShadowCascade::CascadePartitions[4];
			//ShadowCascade::CascadePartitions[5] = ShadowCascade::CascadePartitions[5] < ShadowCascade::CascadePartitions[4] ? ShadowCascade::CascadePartitions[4] + 1.0f : ShadowCascade::CascadePartitions[5];
			//ShadowCascade::CascadePartitions[6] = ShadowCascade::CascadePartitions[6] < ShadowCascade::CascadePartitions[5] ? ShadowCascade::CascadePartitions[5] + 1.0f : ShadowCascade::CascadePartitions[6];
			////high bounds
			//ShadowCascade::CascadePartitions[0] = ShadowCascade::CascadePartitions[0] > ShadowCascade::CascadePartitions[1] ? ShadowCascade::CascadePartitions[1] - 1.0f : ShadowCascade::CascadePartitions[0];
			//ShadowCascade::CascadePartitions[1] = ShadowCascade::CascadePartitions[1] > ShadowCascade::CascadePartitions[2] ? ShadowCascade::CascadePartitions[2] - 1.0f : ShadowCascade::CascadePartitions[1];
			//ShadowCascade::CascadePartitions[2] = ShadowCascade::CascadePartitions[2] > ShadowCascade::CascadePartitions[3] ? ShadowCascade::CascadePartitions[3] - 1.0f : ShadowCascade::CascadePartitions[2];
			//ShadowCascade::CascadePartitions[3] = ShadowCascade::CascadePartitions[3] > ShadowCascade::CascadePartitions[4] ? ShadowCascade::CascadePartitions[4] - 1.0f : ShadowCascade::CascadePartitions[3];
			//ShadowCascade::CascadePartitions[4] = ShadowCascade::CascadePartitions[4] > ShadowCascade::CascadePartitions[5] ? ShadowCascade::CascadePartitions[5] - 1.0f : ShadowCascade::CascadePartitions[4];
			//ShadowCascade::CascadePartitions[5] = ShadowCascade::CascadePartitions[5] > ShadowCascade::CascadePartitions[6] ? ShadowCascade::CascadePartitions[6] - 1.0f : ShadowCascade::CascadePartitions[5];
		}
	}
	ImGui::BeginMainMenuBar();

	ImGui::PushID(1);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7, 0.1, 0.1, 0.8));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9, 0.1, 0.1, 1.0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0, 0.0, 0.0, 1.0));
	if (ImGui::Button("Quit", ImVec2(60, 20)))
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
		else if (Themp::System::tSys->m_D3D->m_ShadowType == 1)
		{
			numMSAA = (numMSAA == 1 ? 2 : numMSAA == 2 ? 4 : numMSAA == 4 ? 8 : 2);
		}
		else
		{
			numMSAA = (numMSAA == 1 ? 2 : numMSAA == 2 ? 4 : numMSAA == 4 ? 8 : 1);
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
		for (size_t i = 0; i < 3; i++)
		{
			XMFLOAT3 lDir = Normalize(lightdirection[i]);
			D3D::s_D3D->SetDirectionalLight(i, lightEnabled[i], XMFLOAT4(lightposition[i].x, lightposition[i].y, lightposition[i].z, 1.0), XMFLOAT4(lDir.x, lDir.y, lDir.z, 0.0f), lightcolor[i]);
			D3D::s_D3D->SetLightDirty(D3D::LightType::Directional, i);
		}
	}
	if (ImGui::Checkbox("Wireframe", &Themp::System::tSys->m_D3D->m_Wireframe))
	{

	}
	ImGui::EndMainMenuBar();

	ImGui::SetNextWindowPos(ImVec2(750, 30), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Lights"))
	{
		ImGui::End();
	}
	else
	{
		XMFLOAT3 p = m_Camera->GetPosition();
		XMFLOAT3 f = m_Camera->GetForward();
		ImGui::Text("Camera Position: X:%f Y:%f Z:%f\n", p.x, p.y, p.z);
		ImGui::Text("Camera Direction: X:%f Y : %f Z : %f\n", f.x, f.y, f.z);

		if (ImGui::CollapsingHeader("Light 0 options"))
		{
			bool lDirty = false;
			if (ImGui::Checkbox("Enabled", &lightEnabled[0])) lDirty = true;
			if (ImGui::InputFloat3("Position", &lightposition[0].x)) lDirty = true;
			if (ImGui::InputFloat3("Direction", &lightdirection[0].x)) lDirty = true;
			if (ImGui::ColorPicker3("Color", &lightcolor[0].x)) lDirty = true;
			if (ImGui::Button("Set to Camera"))
			{
				lightposition[0] = p;
				lightdirection[0] = f;
				lDirty = true;
			}
			if (lDirty)
			{
				lightdirection[0] = Normalize(lightdirection[0]);
				lightcolor[0].w = 1.0;
				D3D::s_D3D->SetDirectionalLight(0, lightEnabled[0], XMFLOAT4(lightposition[0].x, lightposition[0].y, lightposition[0].z, 1.0), XMFLOAT4(lightdirection[0].x, lightdirection[0].y, lightdirection[0].z, 0.0f), lightcolor[0]);
				D3D::s_D3D->SetLightDirty(D3D::LightType::Directional, 0);
			}
		}
		if (ImGui::CollapsingHeader("Light 1 options"))
		{
			bool lDirty = false;
			if (ImGui::Checkbox("Enabled", &lightEnabled[1])) lDirty = true;
			if (ImGui::InputFloat3("Position", &lightposition[1].x)) lDirty = true;
			if (ImGui::InputFloat3("Direction", &lightdirection[1].x)) lDirty = true;
			if (ImGui::ColorPicker3("Color", &lightcolor[1].x)) lDirty = true;
				
			
			if (ImGui::Button("Set to Camera"))
			{
				lightposition[1] = p;
				lightdirection[1] = f;
				lDirty = true;
			}
			if (lDirty)
			{
				lightdirection[1] = Normalize(lightdirection[1]);
				lightcolor[1].w = 1.0;
				D3D::s_D3D->SetDirectionalLight(1, lightEnabled[1], XMFLOAT4(lightposition[1].x, lightposition[1].y, lightposition[1].z, 1.0), XMFLOAT4(lightdirection[1].x, lightdirection[1].y, lightdirection[1].z, 0.0f), lightcolor[1]);
				D3D::s_D3D->SetLightDirty(D3D::LightType::Directional, 1);
			}
		}
		if (ImGui::CollapsingHeader("Light 2 options"))
		{
			bool lDirty = false;
			if (ImGui::Checkbox("Enabled", &lightEnabled[2])) lDirty = true;
			if (ImGui::InputFloat3("Position", &lightposition[2].x)) lDirty = true;
			if (ImGui::InputFloat3("Direction", &lightdirection[2].x)) lDirty = true;
			if (ImGui::ColorPicker3("Color", &lightcolor[2].x)) lDirty = true;
			if (ImGui::Button("Set to Camera"))
			{
				lightposition[2] = p;
				lightdirection[2] = f;
				lDirty = true;
			}
			if (lDirty)
			{
				lightdirection[2] = Normalize(lightdirection[2]);
				lightcolor[2].w = 1.0;
				D3D::s_D3D->SetDirectionalLight(2, lightEnabled[2], XMFLOAT4(lightposition[2].x, lightposition[2].y, lightposition[2].z, 1.0), XMFLOAT4(lightdirection[2].x, lightdirection[2].y, lightdirection[2].z, 0.0f), lightcolor[2]);
				D3D::s_D3D->SetLightDirty(D3D::LightType::Directional, 2);
			}
		}
		ImGui::End();
	}
	
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
	}
	if (m_Keys['S'])
	{
		m_Camera->MoveBackward();
	}
	if (m_Keys['A'])
	{
		m_Camera->MoveLeft();
	}
	if (m_Keys['D'])
	{
		m_Camera->MoveRight();
	}
	//spacebar
	if (m_Keys[32] || m_Keys['E'])
	{
		m_Camera->MoveUp();
	}
	if (m_Keys['X'] || m_Keys['Q'])
	{
		m_Camera->MoveDown();
	}
	if (m_Keys['Y'])
	{
		XMFLOAT3 p = m_Camera->GetPosition();
		XMFLOAT3 f = m_Camera->GetForward();
		System::Print("Position: X:%f Y:%f Z:%f\n", p.x, p.y, p.z);
		System::Print("LookDir: X:%f Y:%f Z:%f\n", f.x, f.y, f.z);
	}

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