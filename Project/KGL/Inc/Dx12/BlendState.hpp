#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <array>

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

namespace KGL
{
	inline namespace DX12
	{
		
		namespace BLEND
		{
			enum class TYPE : UINT
			{
				DEFAULT,
				ALPHA,
				ADD,
				SUBTRACT,
				REPLEASE,
				MULTIPLY,
				LIGHTEN,
				DARKEN,
				SCREEN
			};
			using TYPES = std::array<TYPE, 8u>;
			extern HRESULT SetBlend(const TYPES& types, D3D12_BLEND_DESC* desc);
		}
		using BDTYPE = BLEND::TYPE;
		using BDTYPES = BLEND::TYPES;
	}
}