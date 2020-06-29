#pragma once

#include <map>
#include <filesystem>

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class TextureManager
		{
			std::map<std::filesystem::path, ComPtr<ID3D12Resource>> resources;
		};
		class Texture
		{
			std::filesystem::path path;
			ComPtr<ID3D12Resource> buffer;
		};
	}
}