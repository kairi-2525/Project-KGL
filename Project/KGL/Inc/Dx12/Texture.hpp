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
		private:
			std::map<std::filesystem::path, ComPtr<ID3D12Resource>> m_resources;
		private:
			TextureManager(const TextureManager&) = delete;
			TextureManager& operator=(const TextureManager&) = delete;
		public:
			TextureManager() noexcept = default;
			bool GetResource(const std::filesystem::path& path, ComPtr<ID3D12Resource>* resource) const noexcept;
			bool SetResource(const std::filesystem::path& path, ComPtr<ID3D12Resource> resource) noexcept;
		};
		class Texture
		{
		private:
			std::filesystem::path m_path;
			ComPtr<ID3D12Resource> m_buffer;
		private:
			Texture() = delete;
		public:
			// 画像テクスチャ
			explicit Texture(ComPtr<ID3D12Device> device,
				const std::filesystem::path& path, TextureManager* mgr = nullptr) noexcept;
			// 単色テクスチャ
			explicit Texture(ComPtr<ID3D12Device> device,
				UCHAR r = 0xff, UCHAR g = 0xff, UCHAR b = 0xff, UCHAR a = 0xff, TextureManager* mgr = nullptr) noexcept;
			// グラデーションテクスチャ
			explicit Texture(ComPtr<ID3D12Device> device,
				UCHAR tr, UCHAR tg, UCHAR tb, UCHAR ta,
				UCHAR br, UCHAR bg, UCHAR bb, UCHAR ba, UINT16 height = 255, TextureManager* mgr = nullptr) noexcept;
		};
	}
}