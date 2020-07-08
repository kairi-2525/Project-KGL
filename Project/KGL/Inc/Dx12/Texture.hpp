#pragma once

#include <map>
#include <filesystem>

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include "../Helper/ThrowAssert.hpp"
#include "../Loader/Loader.hpp"
#include <DirectXMath.h>

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
			// �摜�e�N�X�`��
			explicit Texture(const ComPtr<ID3D12Device>& device,
				const std::filesystem::path& path, TextureManager* mgr = nullptr) noexcept
			{
				auto hr = Create(device, path, mgr); AssertLoadResult(hr, m_path.string());
			}
			// �P�F�e�N�X�`��
			explicit Texture(const ComPtr<ID3D12Device>& device,
				UCHAR r = 0xff, UCHAR g = 0xff, UCHAR b = 0xff, UCHAR a = 0xff, TextureManager* mgr = nullptr) noexcept
			{
				auto hr = Create(device, r, g, b, a, mgr); AssertLoadResult(hr, m_path.string());
			}
			// �O���f�[�V�����e�N�X�`��
			explicit Texture(const ComPtr<ID3D12Device>& device,
				UCHAR tr, UCHAR tg, UCHAR tb, UCHAR ta,
				UCHAR br, UCHAR bg, UCHAR bb, UCHAR ba, UINT16 height = 256, TextureManager* mgr = nullptr) noexcept
			{
				auto hr = Create(device, tr, tg, tb, ta, br, bg, bb, ba, height, mgr); AssertLoadResult(hr, m_path.string());
			}
			// �쐬�ς݂̃e�N�X�`�������Ƃɍ쐬
			explicit Texture(const ComPtr<ID3D12Device>& device,
				const Texture& resource, const DirectX::XMFLOAT4& clear_value) noexcept
			{
				auto hr = Create(device, resource, clear_value); AssertLoadResult(hr, m_path.string());
			}
			explicit Texture(const ComPtr<ID3D12Device>& device,
				const ComPtr<ID3D12Resource>& resource, const DirectX::XMFLOAT4& clear_value) noexcept
			{
				auto hr = Create(device, resource, clear_value); AssertLoadResult(hr, m_path.string());
			}
			
			// �摜�e�N�X�`��
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				const std::filesystem::path& path, TextureManager* mgr = nullptr) noexcept;
			// �P�F�ŏ��e�N�X�`��
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				UCHAR r = 0xff, UCHAR g = 0xff, UCHAR b = 0xff, UCHAR a = 0xff, TextureManager* mgr = nullptr) noexcept;
			// �O���f�[�V�����e�N�X�`��
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				UCHAR tr, UCHAR tg, UCHAR tb, UCHAR ta,
				UCHAR br, UCHAR bg, UCHAR bb, UCHAR ba, UINT16 height = 256, TextureManager* mgr = nullptr) noexcept;
			// �t�H�[�}�b�g�w���e�N�X�`��
			//HRESULT Create(ComPtr<ID3D12Device> device, DXGI_FORMAT format, UINT16 w, UINT16 h, UCHAR clear_value) noexcept;
			// �쐬�ς݂̃e�N�X�`�������Ƃɍ쐬
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				const Texture& resource, const DirectX::XMFLOAT4& clear_value) noexcept
			{ return Create(device, resource.Data(), clear_value); }
			HRESULT Create(const ComPtr<ID3D12Device>& device,
				const ComPtr<ID3D12Resource>& resource, const DirectX::XMFLOAT4& clear_value) noexcept;
			
			const ComPtr<ID3D12Resource>& Data() const noexcept { return m_buffer; }
			D3D12_RESOURCE_BARRIER GetRtvResourceBarrier(bool render_target) noexcept;
		};
	}
}