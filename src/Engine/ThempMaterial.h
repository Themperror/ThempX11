#pragma once
#include <d3d11.h>
#include <string>

#define MAX_TEXTURES 4

namespace Themp
{
	struct Texture
	{
		ID3D11Resource* m_Resource = nullptr;
		ID3D11ShaderResourceView* m_View = nullptr;
		ID3D11SamplerState* m_SamplerState = nullptr;
		~Texture()
		{
			if (m_Resource)
			{
				m_Resource->Release();
				m_Resource = nullptr;
			}
			if (m_View)
			{
				m_View->Release();
				m_View = nullptr;
			}

			m_SamplerState = nullptr;
		}
	};
	class Material
	{
	public:
		struct MaterialConstantBuffer
		{
			int hasNormal, hasPBR, dummy1, isEmissive; //16 bytes
			float Metallic, Roughness, EmissiveStrength, F0; //16
		};//32

		//This is from Assimp, and also what we import.
		// DIFFUSE AND AMBIENT = diffuse ( 1 & 3)
		// HEIGHT = normal (5)
		// OPACITY = alpha map (8)

		enum aiTextureType : uint8_t
		{
			NONE = 0x0,
			DIFFUSE = 0x1,
			PBR = 0x2,
			AMBIENT = 0x3,
			EMISSIVE = 0x4,
			NORMALS = 0x5,
			HEIGHT = 0x6,
			SHININESS = 0x7,
			OPACITY = 0x8,
			DISPLACEMENT = 0x9,
			LIGHTMAP = 0xA,
			REFLECTION = 0xB,
			UNKNOWN = 0xC,
			UNUSED = 0xFF,
		};
		Material();
		~Material();
		ID3D11VertexShader* m_VertexShader;
		ID3D11PixelShader* m_PixelShader;
		ID3D11GeometryShader* m_GeometryShader;
		ID3D11InputLayout* m_InputLayout;


		ID3D11ShaderResourceView* m_Views[MAX_TEXTURES];
		ID3D11SamplerState* m_SamplerStates[MAX_TEXTURES];
		Themp::Texture* m_Textures[MAX_TEXTURES];
		ID3D11Buffer* m_MaterialConstantBuffer;
		MaterialConstantBuffer m_MaterialConstantBufferData;
		//TextureType* textureTypes;
		uint32_t* textureSlots;
		uint32_t numTextures = 0;

		ID3D10Blob* ReadToBlob(std::string path);
		void ReadTexture(std::string path);
		void ReadTextures(std::vector<std::string>& textures, std::vector<uint8_t>& textureTypes);
		void GetMaterialProperties(std::string matName, std::string* outPBRTexture = nullptr);
		void UpdateBuffer();
		static void GetGBufferShaderName(std::string matName, std::string& outShaderPath, bool& outHasGeometryShader);
		//void ReadTextures(std::vector<std::string>& textures);
	};
}