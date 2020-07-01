#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <DirectXMath.h>
#include <filesystem>

namespace KGL
{
	inline namespace BASE
	{
		namespace MODEL
		{
			struct MaterialForHLSL		// HLSL—p
			{
				DirectX::XMFLOAT3	deffuse;
				FLOAT				alpha;
				DirectX::XMFLOAT3	specular;
				FLOAT				specularity;
				DirectX::XMFLOAT3	ambient;
			};
			struct AdditionalMaterial	// ‚»‚êˆÈŠO
			{
				std::filesystem::path	tex_path;
				UCHAR					toon_idx;
				bool					edge_flg;
			};
			struct Material
			{
				UINT				indices_num;
				MaterialForHLSL		material;
				AdditionalMaterial	additional;
			};
		}
	}
}