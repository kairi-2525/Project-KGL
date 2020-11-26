#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <filesystem>
#include <map>
#include <string>
#include <DirectXMath.h>
#include <memory>

namespace KGL
{
	inline namespace BASE
	{
		namespace OBJ
		{
			struct ObjectData
			{
				std::vector<DirectX::XMFLOAT3>	positions;
				std::vector<DirectX::XMFLOAT2>	uvs;
				std::vector<DirectX::XMFLOAT3>	normals;
			};

			struct Object
			{
				struct Desc
				{
					size_t begin;
					size_t count;
				};

				Desc positions;
				Desc uvs;
				Desc normals;
			};

			struct Vertex
			{
				UINT position;
				UINT uv;
				UINT normal;
			};

			using Vertices = std::vector<Vertex>;
			
			struct Material
			{
				bool		smooth;
				Vertices	vertices;
			};

			struct Desc
			{
				std::filesystem::path								mtl_path;
				std::map<std::string, std::shared_ptr<Material>>	materials;
				std::map<std::string, std::shared_ptr<Object>>		objects;
				ObjectData											object_data;
			};
		}
	}
}