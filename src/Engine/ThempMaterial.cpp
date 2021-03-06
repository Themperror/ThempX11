#include "ThempSystem.h"
#include "ThempMaterial.h"
#include "ThempD3D.h"
#include "ThempResources.h"
#include <d3d10.h>
#include <istream>
#include <fstream>
#include <iostream>
#include <sstream>
namespace Themp
{
	Material::Material()
	{
		numTextures = 0;
		for (size_t i = 0; i < MAX_TEXTURES; i++)
		{
			m_Textures[i] = nullptr;
			m_SamplerStates[i] = nullptr;
			m_Views[i] = nullptr;
		}
		m_MaterialConstantBuffer = nullptr;
		memset(&m_MaterialConstantBufferData, 0, sizeof(MaterialConstantBuffer));
	}

	Material::~Material()
	{
		for (size_t i = 0; i < MAX_TEXTURES; i++)
		{
			m_SamplerStates[i] = nullptr;
			m_Views[i] = nullptr;
		}
		if (m_InputLayout)
		{
			m_InputLayout->Release();
			m_InputLayout = nullptr;
		}
		if (m_GeometryShader)
		{
			m_GeometryShader = nullptr;
		}
		if (m_VertexShader)
		{
			m_VertexShader = nullptr;
		}
		if (m_PixelShader)
		{
			m_PixelShader = nullptr;
		}
		if (m_MaterialConstantBuffer)
		{
			m_MaterialConstantBuffer->Release();
			m_MaterialConstantBuffer = nullptr;
		}
		delete[] textureSlots;
	}
	void Material::ReadTexture(std::string path)
	{
		std::vector<std::string> defaultTextures = {
			"DefaultDiffuse.dds",
			"",
			"",
			"",
		};
		std::vector<uint8_t> defaultTypes = { 1,((uint8_t)(-1)),((uint8_t)(-1)),((uint8_t)(-1)) };
		defaultTextures[0] = path;
		ReadTextures(defaultTextures, defaultTypes);
		this->numTextures = 1;
		
	}
	void Material::ReadTextures(std::vector<std::string>& textures, std::vector<uint8_t>& textureTypes)
	{
		numTextures = (uint32_t)textures.size();
		if (textures.size() > MAX_TEXTURES)
		{
			System::Print("Found more than %i textures for a material, this is not supported yet!", MAX_TEXTURES);
			numTextures = MAX_TEXTURES;
		}
		std::string defaultTextures[4] = {
			"DefaultDiffuse.dds",
			"",
			"",
			"",
		};
		for (size_t i = 0; i < numTextures; i++)
		{
			switch (textureTypes[i])
			{
			case Material::DIFFUSE:
				defaultTextures[0] = textures[i];
				break;
			case Material::PBR: defaultTextures[2] = textures[i];
				m_MaterialConstantBufferData.hasPBR = textures[i] != "DefaultPBR.dds" ? true : false;
				break;
			case Material::AMBIENT: 
				defaultTextures[0] = textures[i]; 
				break;
			case Material::NORMALS: defaultTextures[1] = textures[i];
				m_MaterialConstantBufferData.hasNormal = true;
				break;
			case Material::UNKNOWN: defaultTextures[3] = textures[i];
				m_MaterialConstantBufferData.dummy1 = true;
				break;
			case Material::UNUSED: break;
			default: System::Print("ThempMaterial::ReadTextures | New unhandled texture type found: %i : %s \n", textureTypes[i], textures.at(i).c_str()); break;
			}
		}

		textureSlots = new uint32_t[MAX_TEXTURES];
		
		for (size_t i = 0; i < MAX_TEXTURES; i++)
		{
			//CAN RETURN NULLPTR, This is valid as not all texture slots might be used!!
			Themp::Texture* tex = Resources::TRes->GetTexture(defaultTextures[i]);

			//this is so I can map all MAX_TEXTURES states and views through a single call
			if (tex != nullptr)
			{
				m_Views[i] = tex->m_View;
				tex->m_SamplerState = D3D::DefaultTextureSampler;
				m_SamplerStates[i] = tex->m_SamplerState;
			}
			else
			{
				m_Views[i] = nullptr;
				m_SamplerStates[i] = D3D::DefaultTextureSampler;
			}
			m_Textures[i] = tex;

		}

		if (!m_MaterialConstantBuffer)
		{
			// Fill in a buffer description.
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.ByteWidth = sizeof(MaterialConstantBuffer);
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = 0;

			// Fill in the subresource data.
			D3D11_SUBRESOURCE_DATA InitData;
			InitData.pSysMem = &m_MaterialConstantBufferData;
			InitData.SysMemPitch = 0;
			InitData.SysMemSlicePitch = 0;

			// Create the buffer.
			Themp::System::tSys->m_D3D->m_Device->CreateBuffer(&cbDesc, &InitData, &m_MaterialConstantBuffer);
		}
		else
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			Themp::System::tSys->m_D3D->m_DevCon->Map(m_MaterialConstantBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
			memcpy(ms.pData, &m_MaterialConstantBufferData, sizeof(MaterialConstantBuffer));
			Themp::System::tSys->m_D3D->m_DevCon->Unmap(m_MaterialConstantBuffer, NULL);
		}

	}
	void Material::GetMaterialProperties(std::string matName, std::string* outPBRTexture)
	{
		if (matName != "")
		{
			matName.append(".mat");
			std::ifstream input(BASE_MATERIAL_PATH + matName);
			if (input.good())
			{
				std::string line;
				std::string a0, a1, a2;
				while (getline(input, line))
				{
					std::stringstream stream(line);
					if (line.size() > 0)
					{
						stream >> a0 >> a1 >> a2;
						if (a0 == "PBRTexture")
						{
							if (outPBRTexture)
							{
								outPBRTexture->clear();
								outPBRTexture->append(a1);
							}
						}
						if (a0 == "Metallic")
						{
							m_MaterialConstantBufferData.Metallic = std::stof(a1);
						}
						if (a0 == "Roughness")
						{
							m_MaterialConstantBufferData.Roughness = std::stof(a1);
						}
						if (a0 == "IsEmissive")
						{
							m_MaterialConstantBufferData.isEmissive = (a1 == "true");
						}
						if (a0 == "EmissiveStrength")
						{
							m_MaterialConstantBufferData.EmissiveStrength = std::stof(a1);
						}
					}
				}
				input.close();
			}
			else
			{
#if LOG_MISSING_MATERIALS==1
				System::Print("Could not find Material: %s", (BASE_MATERIAL_PATH + matName).c_str());
#endif
			}
		}
	}
	void Material::GetGBufferShaderName(std::string matName, std::string& outShaderPath, bool& outHasGeometryShader)
	{
		if (matName != "")
		{
			matName.append(".mat");
			
			std::ifstream input(BASE_MATERIAL_PATH + matName);
			if (input.good())
			{
				std::string line;
				std::string a0, a1, a2;
				while (getline(input, line))
				{
					std::stringstream stream(line);
					if (line.size() > 0)
					{
						stream >> a0 >> a1 >> a2;
						if (a0 == "Shader")
						{
							outShaderPath = a1;
							outHasGeometryShader = (a2 == "true");
						}
					}
				}
				input.close();
			}
			else
			{
#if LOG_MISSING_MATERIALS==1
				System::Print("Could not find Material: %s", (BASE_MATERIAL_PATH + matName).c_str());
#endif
			}
		}
	}
	void Material::UpdateBuffer()
	{
		if (!m_MaterialConstantBuffer)
		{
			// Fill in a buffer description.
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.ByteWidth = sizeof(MaterialConstantBuffer);
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = 0;

			// Fill in the subresource data.
			D3D11_SUBRESOURCE_DATA InitData;
			InitData.pSysMem = &m_MaterialConstantBufferData;
			InitData.SysMemPitch = 0;
			InitData.SysMemSlicePitch = 0;

			// Create the buffer.
			Themp::System::tSys->m_D3D->m_Device->CreateBuffer(&cbDesc, &InitData, &m_MaterialConstantBuffer);
		}
		else
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			Themp::System::tSys->m_D3D->m_DevCon->Map(m_MaterialConstantBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
			memcpy(ms.pData, &m_MaterialConstantBufferData, sizeof(MaterialConstantBuffer));
			Themp::System::tSys->m_D3D->m_DevCon->Unmap(m_MaterialConstantBuffer, NULL);
		}
	}
	ID3D10Blob* Material::ReadToBlob(std::string path)
	{
		ID3D10Blob* nBlob;
		std::ifstream ifs(path, std::ios::binary | std::ios::ate);
		if (!ifs.good()) return nullptr;
		std::ifstream::pos_type pos = ifs.tellg();
		size_t length = pos;
		D3D10CreateBlob(length, &nBlob);
		ifs.seekg(0, std::ios::beg);
		ifs.read((char*)nBlob->GetBufferPointer(), length);
		ifs.close();
		return nBlob;
	}

}