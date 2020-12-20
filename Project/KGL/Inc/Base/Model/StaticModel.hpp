#pragma once

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace KGL
{
	inline namespace BASE
	{
		namespace S_MODEL
		{
			struct Vertex
			{
				DirectX::XMFLOAT3 position;
				DirectX::XMFLOAT2 uv;
				DirectX::XMFLOAT3 normal;
				DirectX::XMFLOAT3 tangent;
				DirectX::XMFLOAT3 bitangent;
			};
			
			using Vertices = std::vector<Vertex>;

			struct Textures
			{
				using ReflectionsPath = std::unordered_map<std::string, std::filesystem::path>;

				std::filesystem::path	ambient;
				std::filesystem::path	diffuse;
				std::filesystem::path	specular;
				std::filesystem::path	specular_highlights;
				std::filesystem::path	dissolve;
				std::filesystem::path	bump;
				std::filesystem::path	displacement;
				std::filesystem::path	stencil_decal;
				ReflectionsPath			reflections;
			};

			struct Material
			{
				struct Parameter
				{
					DirectX::XMFLOAT3			ambient_color; float pad0;
					DirectX::XMFLOAT3			diffuse_color; float pad1;
					DirectX::XMFLOAT3			specular_color;
					float						specular_weight;
					bool						specular_flg; __int8 pad2[3];
					float						dissolve;	// ìßñæìx 1Ç»ÇÁìßñæ
					float						refraction;	// ã¸ê‹ó¶
					bool						smooth; __int8 pad3[3];
				};

				Vertices	vertices;
				Textures	tex;

				Parameter	param;
			};
			
			using Materials = std::unordered_map<std::string, Material>;
		}
	}
}