#pragma once

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <map>

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
			};
			
			using Vertices = std::vector<Vertex>;

			struct Material
			{
				bool		smooth;
				Vertices	vertices;
			};
			
			using Materials = std::map<std::string, Material>;
		}
	}
}