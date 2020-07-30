#pragma once

#include <map>
#include <filesystem>

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include "../Helper/ThrowAssert.hpp"
#include "../Loader/Loader.hpp"
#include "SetName.hpp"
#include <DirectXMath.h>
#include <vector>
#include <memory>

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
			explicit Texture(const ComPtr<ID3D12Device>& device,
				const std::filesystem::path& path, TextureManager* mgr = nullptr) noexcept
			{
				auto hr = Create(device, path, mgr); AssertLoadResult(hr, m_path.string());
				SetName(m_buffer, RCAST<intptr_t>(m_buffer.Get()), m_path.wstring());
			}
			// 単色テクスチャ
			explicit Texture(const ComPtr<ID3D12Device>& device,
				UCHAR r = 0xff, UCHAR g = 0xff, UCHAR b = 0xff, UCHAR a = 0xff, TextureManager* mgr = nullptr) noexcept
			{
				auto hr = Create(device, r, g, b, a, mgr); AssertLoadResult(hr, m_path.string());
				SetName(m_buffer, RCAST<intptr_t>(m_buffer.Get()), m_path.wstring());
			}
			// グラデーションテクスチャ
			explicit Texture(const ComPtr<ID3D12Device>& device,
				UCHAR tr, UCHAR tg, UCHAR tb, UCHAR ta,
				UCHAR br, UCHAR bg, UCHAR bb, UCHAR ba, UINT16 height = 256, TextureManager* mgr = nullptr) noexcept
			{
				auto hr = Create(device, tr, tg, tb, ta, br, bg, bb, ba, height, mgr); AssertLoadResult(hr, m_path.string());
				SetName(m_buffer, RCAST<intptr_t>(m_buffer.Get()), m_path.wstring());
			}
			// フォーマット指定空テクスチャ
			explicit Texture(
				ComPtr<ID3D12Device> device,
				const D3D12_RESOURCE_DESC& res_desc, const D3D12_CLEAR_VALUE& clear_value,
				D3D12_RESOURCE_STATES res_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) noexcept
			{
				auto hr = Create(device, res_desc, clear_value, res_state); AssertLoadResult(hr, m_path.string());
				SetName(m_buffer, RCAST<intptr_t>(m_buffer.Get()), m_path.wstring());
			}
			// 作成済みのテクスチャをもとに作成
			explicit Texture(const ComPtr<ID3D12Device>& device,
				const Texture& resource, const DirectX::XMFLOAT4& clear_value) noexcept
			{
				auto hr = Create(device, resource, clear_value); AssertLoadResult(hr, m_path.string());
				SetName(m_buffer, RCAST<intptr_t>(m_buffer.Get()), m_path.wstring());
			}
			explicit Texture(const ComPtr<ID3D12Device>& device,
				const ComPtr<ID3D12Resource>& resource, const DirectX::XMFLOAT4& clear_value) noexcept
			{
				auto hr = Create(device, resource, clear_value); AssertLoadResult(hr, m_path.string());
				SetName(m_buffer, RCAST<intptr_t>(m_buffer.Get()), m_path.wstring());
			}
			
			// 画像テクスチャ
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				const std::filesystem::path& path, TextureManager* mgr = nullptr) noexcept;
			// 単色最小テクスチャ
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				UCHAR r = 0xff, UCHAR g = 0xff, UCHAR b = 0xff, UCHAR a = 0xff, TextureManager* mgr = nullptr) noexcept;
			// グラデーションテクスチャ
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				UCHAR tr, UCHAR tg, UCHAR tb, UCHAR ta,
				UCHAR br, UCHAR bg, UCHAR bb, UCHAR ba, UINT16 height = 256, TextureManager* mgr = nullptr) noexcept;
			// フォーマット指定空テクスチャ
			HRESULT Create(ComPtr<ID3D12Device> device,
				const D3D12_RESOURCE_DESC& res_desc, const D3D12_CLEAR_VALUE& clear_value,
				D3D12_RESOURCE_STATES res_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) noexcept;
			// 作成済みのテクスチャをもとに作成
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				const Texture& resource, const DirectX::XMFLOAT4& clear_value) noexcept
			{
				m_path = resource.m_path.string() + "_copy"; return Create(device, resource.Data(), clear_value);
			}
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				const ComPtr<ID3D12Resource>& resource, const DirectX::XMFLOAT4& clear_value) noexcept;
			
			const ComPtr<ID3D12Resource>& Data() const noexcept { return m_buffer; }
			const std::filesystem::path& GetPath()  const noexcept { return m_path; }
		};
		
		namespace TEXTURE
		{
			inline std::vector<ComPtr<ID3D12Resource>> GetResources(
				const std::vector<std::shared_ptr<Texture>>& in_tex
			)
			{
				std::vector<ComPtr<ID3D12Resource>> ret;
				ret.reserve(in_tex.size());
				for (const auto& tex : in_tex)
				{
					if (tex) ret.emplace_back(tex->Data());
				}
				return std::move(ret);
			}
		}
	}
}