#pragma once

#include <map>
#include <filesystem>

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include "../Helper/ThrowAssert.hpp"
#include "../Loader/Loader.hpp"

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
			TextureManager() = default;
			bool GetResource(const std::filesystem::path& path, ComPtr<ID3D12Resource>* resource) const noexcept;
			bool SetResource(const std::filesystem::path& path, const ComPtr<ID3D12Resource>& resource) noexcept;
		};
		class Texture
		{
		private:
			std::filesystem::path m_path;
			ComPtr<ID3D12Resource> m_buffer;
		public:
			Texture() = default;
			// 画像テクスチャ
			explicit Texture(ComPtr<ID3D12Device> device,
				const std::filesystem::path& path, TextureManager* mgr = nullptr) noexcept
			{
				auto hr = Create(device, path, mgr); AssertLoadResult(hr, m_path.string());
			}
			// 単色テクスチャ
			explicit Texture(ComPtr<ID3D12Device> device,
				UCHAR r = 0xff, UCHAR g = 0xff, UCHAR b = 0xff, UCHAR a = 0xff, TextureManager* mgr = nullptr) noexcept
			{
				auto hr = Create(device, r, g, b, a, mgr); AssertLoadResult(hr, m_path.string());
			}
			// グラデーションテクスチャ
			explicit Texture(ComPtr<ID3D12Device> device,
				UCHAR tr, UCHAR tg, UCHAR tb, UCHAR ta,
				UCHAR br, UCHAR bg, UCHAR bb, UCHAR ba, UINT16 height = 256, TextureManager* mgr = nullptr) noexcept
			{
				auto hr = Create(device, tr, tg, tb, ta, br, bg, bb, ba, height, mgr); AssertLoadResult(hr, m_path.string());
			}
			
			// 画像テクスチャ
			HRESULT Create(ComPtr<ID3D12Device> device,
				const std::filesystem::path& path, TextureManager* mgr = nullptr) noexcept;
			// 単色テクスチャ
			HRESULT Create(ComPtr<ID3D12Device> device,
				UCHAR r = 0xff, UCHAR g = 0xff, UCHAR b = 0xff, UCHAR a = 0xff, TextureManager* mgr = nullptr) noexcept;
			// グラデーションテクスチャ
			HRESULT Create(ComPtr<ID3D12Device> device,
				UCHAR tr, UCHAR tg, UCHAR tb, UCHAR ta,
				UCHAR br, UCHAR bg, UCHAR bb, UCHAR ba, UINT16 height = 256, TextureManager* mgr = nullptr) noexcept;
			
			const ComPtr<ID3D12Resource>& Data() const noexcept { return m_buffer; };
		};
	}
}