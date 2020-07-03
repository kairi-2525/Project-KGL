#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

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
			};

			extern HRESULT SetBlend(TYPE type, D3D12_BLEND_DESC* desc);
		}
		using BDTYPE = BLEND::TYPE;
	}
}