#pragma once
#include <unordered_map>
#include <d3d11.h>
#define BASE_TEXTURE_PATH "../data/textures/"
#define BASE_SHADER_PATH "../data/shaders/"
#define BASE_MODEL_PATH "../data/models/"

#define MODEL_VERSION_HIGH 1
#define MODEL_VERSION_LOW 7

namespace Themp
{
	struct Texture;
	class Object3D;
	class Material;
	class Resources
	{
	public:
		Resources();
		~Resources();
		Texture* GetTexture(std::string path);
		Texture* GetTexture(std::string path, bool isDefaulted);
		std::unordered_map<std::string, ID3D11VertexShader*> m_VertexShaders;
		std::unordered_map<std::string, ID3D11PixelShader*> m_PixelShaders;
		std::unordered_map<std::string, ID3D11GeometryShader*> m_GeometryShaders;
		std::unordered_map<std::string, Texture*> m_Textures;
		std::unordered_map<std::string, Themp::Material*> m_Materials;
		std::vector<Object3D*> m_3DObjects;
		ID3D10Blob* ReadToBlob(std::string path);
		ID3D11VertexShader* GetVertexShader(std::string name);
		ID3D11PixelShader* GetPixelShader(std::string name);
		ID3D11GeometryShader * GetGeometryShader(std::string name);

		//shaderpath will detect shaders in a named pattern like "path_%type%" so as shader_VS, shader_PS, shader_GS
		Themp::Material* LoadMaterial(std::string texture, std::string shaderPath, bool vertexShader, bool pixelShader, bool geometryShader);
		//shaderpath will detect shaders in a named pattern like "path_%type%" so as shader_VS, shader_PS, shader_GS
		Themp::Material * LoadMaterial(std::vector<std::string>& textures, std::vector<uint8_t>& textureTypes, std::string shaderPath, bool vertexShader, bool pixelShader, bool geometryShader);
		//Themp::Material * LoadMaterial(std::vector<std::string>& textures, std::string shaderPath, bool vertexShader, bool pixelShader, bool geometryShader);
		
		Object3D* LoadModel(std::string name);

		static Resources* TRes;
	};
};