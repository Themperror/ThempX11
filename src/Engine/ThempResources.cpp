#include "ThempSystem.h"
#include "ThempResources.h"
#include "ThempD3D.h"
#include "ThempObject3D.h"
#include "ThempMesh.h"

#include <DDSTextureLoader.h>

#include <iostream>
#include <fstream>

namespace Themp
{
	Resources* Resources::TRes = nullptr;
	Resources::Resources()
	{
		Resources::TRes = this;
	}
	Resources::~Resources()
	{
		for each (auto i in m_VertexShaders)
		{
			i.second->Release();
			i.second = nullptr;
		}
		m_VertexShaders.clear();
		for each (auto i in m_PixelShaders)
		{
			i.second->Release();
			i.second = nullptr;
		}
		m_PixelShaders.clear();
		for each (auto i in m_GeometryShaders)
		{
			i.second->Release();
			i.second = nullptr;
		}
		m_GeometryShaders.clear();
		
		for each (auto i in m_Textures)
		{
			delete i.second;
			i.second = nullptr;
		}
		m_Textures.clear();
		for each (auto i in m_Materials)
		{
			delete i.second;
			i.second = nullptr;
		}
		m_Materials.clear();
		for each (auto i in m_3DObjects)
		{
			delete i;
			i = nullptr;
		}
		m_3DObjects.clear();
		Resources::TRes = nullptr;
	}
	ID3D10Blob* Resources::ReadToBlob(std::string path)
	{
		ID3D10Blob* nBlob;
		std::ifstream ifs(path, std::ios::binary | std::ios::ate);
		if (!ifs.good()) return nullptr;
		std::ifstream::pos_type pos = ifs.tellg();
		int length = pos;
		D3D10CreateBlob(length, &nBlob);
		ifs.seekg(0, std::ios::beg);
		ifs.read((char*)nBlob->GetBufferPointer(), length);
		ifs.close();
		return nBlob;
	}
	Texture* Resources::GetTexture(std::string path)
	{
		if (path == "")
		{
			return nullptr;
		}
		std::string basePath = BASE_TEXTURE_PATH;
		basePath = basePath.append(path);
		std::unordered_map<std::string, Texture*>::iterator s = m_Textures.find(basePath);
		if (s != m_Textures.end()) return s->second;

		Texture* tex = new Texture();
		memset(tex, 0, sizeof(Texture));
		std::wstring wPath = std::wstring(basePath.begin(), basePath.end());
		HRESULT res = DirectX::CreateDDSTextureFromFile(Themp::System::tSys->m_D3D->m_Device, wPath.c_str(), &tex->m_Resource, &tex->m_View);
		
		if(res != S_OK) 
		{
			printf("DirectTK couldn't load the texture: %s , Returning Default Texture instead!!  \n",basePath.c_str());
			delete tex; 
			return GetTexture("DefaultDiffuse.dds", true); 
		}

		//tex->width = FreeImage_GetWidth(loadedImage);
		//tex->height = FreeImage_GetHeight(loadedImage);
		//tex->data = FreeImage_GetBits(loadedImage);

		m_Textures[basePath] = tex;
		return tex;
	}

	Texture* Resources::GetTexture(std::string path, bool isDefaulted = true)
	{
		std::string basePath = BASE_TEXTURE_PATH;
		basePath.append(path);
		std::unordered_map<std::string, Texture*>::iterator s = m_Textures.find(basePath);
		if (s != m_Textures.end()) return s->second;

		Texture* tex = new Texture();
		memset(tex, 0, sizeof(Texture));
		std::wstring wPath = std::wstring(basePath.begin(), basePath.end());
		HRESULT res = DirectX::CreateDDSTextureFromFile(Themp::System::tSys->m_D3D->m_Device, wPath.c_str(), &tex->m_Resource, &tex->m_View);

		if (res != S_OK) 
		{ 
			printf("DirectTK couldn't load the default texture: %s!!!\n", basePath.c_str());
			delete tex; 
			return nullptr; 
		}

		m_Textures[basePath] = tex;
		return tex;
	}
	ID3D11VertexShader * Resources::GetVertexShader(std::string name)
	{
		std::string basePath = BASE_SHADER_PATH;
		basePath.append(name);

		std::unordered_map<std::string, ID3D11VertexShader*>::iterator s = m_VertexShaders.find(basePath);
		if (s != m_VertexShaders.end()) return s->second;

		HRESULT result = 0;
		ID3D11VertexShader* vShader = nullptr;
		ID3D10Blob* VSRaw = ReadToBlob(basePath);
		result = System::tSys->m_D3D->m_Device->CreateVertexShader(VSRaw->GetBufferPointer(), VSRaw->GetBufferSize(), nullptr, &vShader);
		if (result != S_OK) { std::cout << "Could not create default vertex shader from: " <<name.c_str() << std::endl; return nullptr; }
		m_VertexShaders[basePath] = vShader;

		return vShader;
	}

	ID3D11PixelShader * Resources::GetPixelShader(std::string name)
	{
		std::string basePath = BASE_SHADER_PATH;
		basePath.append(name);
		std::unordered_map<std::string, ID3D11PixelShader*>::iterator s = m_PixelShaders.find(basePath);
		if (s != m_PixelShaders.end()) return s->second;
		
		HRESULT result = 0;
		ID3D11PixelShader* pShader = nullptr;
		ID3D10Blob* PSRaw = ReadToBlob(basePath);
		result = System::tSys->m_D3D->m_Device->CreatePixelShader(PSRaw->GetBufferPointer(), PSRaw->GetBufferSize(), nullptr, &pShader);
		if (result != S_OK) { std::cout << "Could not create default vertex shader from: " << name.c_str() << std::endl; return nullptr; }
		m_PixelShaders[basePath] = pShader;

		return pShader;
	}
	ID3D11GeometryShader * Resources::GetGeometryShader(std::string name)
	{
		std::string basePath = BASE_SHADER_PATH;
		basePath.append(name);
		std::unordered_map<std::string, ID3D11GeometryShader*>::iterator s = m_GeometryShaders.find(basePath);
		if (s != m_GeometryShaders.end()) return s->second;
		
		HRESULT result = 0;
		ID3D11GeometryShader* gShader = nullptr;
		ID3D10Blob* GSRaw = ReadToBlob(basePath);
		result = System::tSys->m_D3D->m_Device->CreateGeometryShader(GSRaw->GetBufferPointer(), GSRaw->GetBufferSize(), nullptr, &gShader);
		if (result != S_OK) { std::cout << "Could not create default vertex shader from: " << name.c_str() << std::endl; return nullptr; }
		m_GeometryShaders[basePath] = gShader;

		return gShader;
	}
	Themp::Material* Resources::LoadMaterial(std::string texture, std::string shaderPath, bool vertexShader, bool pixelShader, bool geometryShader)
	{
		std::string tempPath = shaderPath;
		Material* material = nullptr;
		if (vertexShader)
		{
			HRESULT res;
#ifdef _DEBUG
			tempPath.append("_VS_d.cso");
#else
			tempPath.append("_VS.cso");
#endif
			material = new Themp::Material();
			material->m_VertexShader = Resources::TRes->GetVertexShader(tempPath);
			if (!material->m_VertexShader)
			{
				std::cout << "Couldn't find Vertex shader: " << tempPath.c_str() << std::endl;
				delete material;
				return nullptr;
			}
			std::string blobPath = BASE_SHADER_PATH;
			blobPath.append(tempPath);
			ID3D10Blob* vsShaderBlob = material->ReadToBlob(blobPath);
			res = Themp::System::tSys->m_D3D->m_Device->CreateInputLayout(Themp::D3D::DefaultInputLayoutDesc, Themp::D3D::DefaultInputLayoutNumElements, vsShaderBlob->GetBufferPointer(), vsShaderBlob->GetBufferSize(), &material->m_InputLayout);
			if (res == S_OK)
				vsShaderBlob->Release();
			if (res != S_OK) { std::cout << "Could not create shader input layout" << std::endl; delete material; return nullptr; }
		}
		else
		{
			return nullptr;
		}
		if (pixelShader)
		{
			tempPath = shaderPath;
#ifdef _DEBUG
			tempPath.append("_PS_d.cso");
#else
			tempPath.append("_PS.cso");
#endif

			material->m_PixelShader = Resources::TRes->GetPixelShader(tempPath);
			if (!material->m_PixelShader) std::cout << "Couldn't find Pixel shader: " << tempPath.c_str() << std::endl;
		}
		if (geometryShader)
		{
			tempPath = shaderPath;
#ifdef _DEBUG
			tempPath.append("_GS_d.cso");
#else
			tempPath.append("_GS.cso");
#endif
			material->m_GeometryShader = Resources::TRes->GetGeometryShader(tempPath);
			if (!material->m_GeometryShader) std::cout << "Couldn't find Geometry shader: " << tempPath.c_str() << std::endl;
		}
		material->ReadTexture(texture);
		m_Materials[std::string(shaderPath).append(std::to_string(m_Materials.size()))] = material;
		return material;
	}

	Themp::Material* Resources::LoadMaterial(std::vector<std::string>& textures, std::vector<uint8_t>& textureTypes, std::string shaderPath, bool vertexShader, bool pixelShader, bool geometryShader)
	{
		std::string tempPath = shaderPath;
		Themp::Material* material = nullptr;
		if (vertexShader)
		{
			HRESULT res;
#ifdef _DEBUG
			tempPath.append("_VS_d.cso");
#else
			tempPath.append("_VS.cso");
#endif
			material = new Themp::Material();
			material->m_VertexShader = Resources::TRes->GetVertexShader(tempPath);
			if (!material->m_VertexShader)
			{
				std::cout << "Couldn't find Vertex shader: " << tempPath.c_str() << std::endl;
				delete material;
				return nullptr;
			}

			std::string blobPath = BASE_SHADER_PATH;
			blobPath.append(tempPath);
			ID3D10Blob* vsShaderBlob = material->ReadToBlob(blobPath);
			res = Themp::System::tSys->m_D3D->m_Device->CreateInputLayout(Themp::D3D::DefaultInputLayoutDesc, Themp::D3D::DefaultInputLayoutNumElements, vsShaderBlob->GetBufferPointer(), vsShaderBlob->GetBufferSize(), &material->m_InputLayout);
			if(res == S_OK)
				vsShaderBlob->Release();
			if (res != S_OK) { std::cout << "Could not create shader input layout" << std::endl; delete material;  return nullptr; }
		}
		else
		{
			return nullptr;
		}
		if (pixelShader)
		{
			tempPath = shaderPath;
#ifdef _DEBUG
			tempPath.append("_PS_d.cso");
#else
			tempPath.append("_PS.cso");
#endif
			material->m_PixelShader = Resources::TRes->GetPixelShader(tempPath);
			if (!material->m_PixelShader) std::cout << "Couldn't find Pixel shader: " << tempPath.c_str() << std::endl;
		}
		if (geometryShader)
		{
			tempPath = shaderPath;
#ifdef _DEBUG
			tempPath.append("_GS_d.cso");
#else
			tempPath.append("_GS.cso");
#endif
			material->m_GeometryShader = Resources::TRes->GetGeometryShader(tempPath);
			if (!material->m_GeometryShader) std::cout << "Couldn't find Geometry shader: " << tempPath.c_str() << std::endl;
		}

		material->ReadTextures(textures,textureTypes);

		m_Materials[std::string(shaderPath).append(std::to_string(m_Materials.size()))] = material;
		return material;
	}




	Themp::Object3D* Resources::LoadModel(std::string name)
	{
		struct MeshHeader
		{
			uint32_t numVertices;
			uint32_t numIndices;
			uint32_t materialID;
		};
		struct ModelHeader
		{
			char modelVersionHigh;
			char modelVersionLow;
			uint32_t numMeshes;
			uint32_t numMaterials;
			uint32_t numTextures;
			uint32_t numChars;
		};
		std::string modelPath = BASE_MODEL_PATH + name;
		FILE* modelFile = fopen(modelPath.c_str(), "rb");

		if (modelFile == nullptr)
		{
			printf("Could not find model: %s", modelPath.c_str());
			return nullptr;
		}
		Object3D* newObject = new Object3D();

		std::vector<int> materialOrder;

		ModelHeader header;
		fread(&header, sizeof(ModelHeader), 1, modelFile);
		if (header.modelVersionHigh != MODEL_VERSION_HIGH || header.modelVersionLow != MODEL_VERSION_LOW)
		{
			printf("Model version mismatch, cannot load this model!: %s", name.c_str());
			return nullptr;
		}
		for (size_t i = 0; i < header.numMeshes; i++)
		{
			MeshHeader meshHeader;
			//read in mesh header info, contains number of vertices,indices and the belonging material ID
			fread_s(&meshHeader, sizeof(meshHeader), sizeof(MeshHeader), 1, modelFile);
			
			//allocate room for the vertices and indices, read them in afterwards, these pointers are also used for the object themselves so we don't need to delete them.
			Themp::Vertex* vertices = new Vertex[meshHeader.numVertices];
			std::uint32_t* indices = new uint32_t[meshHeader.numIndices];

			fread_s(vertices, sizeof(Vertex)*meshHeader.numVertices, sizeof(Vertex), meshHeader.numVertices, modelFile);
			fread_s(indices, sizeof(uint32_t)*meshHeader.numIndices, sizeof(uint32_t), meshHeader.numIndices, modelFile);

			//create the mesh object, assign all data.
			Mesh* mesh = new Mesh();
			mesh->vertices = vertices;
			mesh->indices = indices;
			mesh->numVertices = meshHeader.numVertices;
			mesh->numIndices = meshHeader.numIndices;
			//the mesh itself doesn't store a material ID but a pointer to material itself, we assign it later in the program so for now keep track of it..
			materialOrder.push_back(meshHeader.materialID);
			
			newObject->m_Meshes.push_back(mesh);
		}

		std::vector<Themp::Material*> loadedMaterials;
		for (size_t i = 0; i < header.numMaterials; i++)
		{
			std::vector<std::string> textureNames;
			std::vector<uint8_t> textureTypes;
			uint32_t numTextures = 0;

			//read in number of textures this material contains.
			fread_s(&numTextures,sizeof(numTextures), sizeof(uint32_t), 1, modelFile);

			//read in the number of characters the material name has.
			uint32_t materialNameSize = 0;
			fread_s(&materialNameSize, sizeof(materialNameSize), sizeof(uint32_t), 1, modelFile);

			//allocate room for the string and read in all characters of the material name
			std::string materialName(materialNameSize, 0);
			fread_s(&materialName[0],materialName.capacity(), sizeof(char)*materialNameSize, 1, modelFile);

			for (size_t j = 0; j < numTextures; j++)
			{
				uint8_t textureType = 0;
				uint32_t textureNameSize = 0;
				
				//read in the texture type and size of the texture name (path)
				fread_s(&textureType,sizeof(textureType), sizeof(uint8_t), 1, modelFile);
				fread_s(&textureNameSize, sizeof(textureNameSize), sizeof(uint32_t), 1, modelFile);

				//read in the texture name (path)
				std::string textureName(textureNameSize, 0);
				fread_s(&textureName[0],textureName.capacity(), sizeof(char)*textureNameSize, 1, modelFile);

				textureTypes.push_back(textureType);
				textureNames.push_back(textureName);
			}
			if (numTextures != 0)
			{
				Themp::Material* newMaterial = LoadMaterial(textureNames,textureTypes, "default", true, true, false);
				textureTypes.clear();
				loadedMaterials.push_back(newMaterial);
			}
			else
			{
				Themp::Material* newMaterial = D3D::DefaultMaterial;
				loadedMaterials.push_back(newMaterial);
			}
		}
		for (size_t j = 0; j < newObject->m_Meshes.size(); j++)
		{
			if (loadedMaterials.size() == 0)
			{
				newObject->m_Meshes[j]->m_Material = D3D::DefaultMaterial;
				continue;
			}
			newObject->m_Meshes[j]->m_Material = loadedMaterials[materialOrder[j]];
			//newObject->m_Meshes[j]->m_Material = loadedMaterials[0];
		}
		//m_3DObjects.push_back(newObject);
		newObject->Construct();
		return newObject;
	}
}