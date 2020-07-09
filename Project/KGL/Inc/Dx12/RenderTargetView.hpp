#pragma once

#include "Texture.hpp"

#include "../Helper/ComPtr.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class RenderTargetView
		{
			ComPtr<ID3D12Resource>			m_buffer;
			ComPtr<ID3D12DescriptorHeap>	m_rtv_heap;
			ComPtr<ID3D12DescriptorHeap>	m_srv_heap;
		public:
			explicit RenderTargetView(ComPtr<ID3D12Device> device, const Texture& texture) noexcept
				: RenderTargetView(device, texture.Data()) {}
			explicit RenderTargetView(ComPtr<ID3D12Device> device, const ComPtr<ID3D12Resource>& resource) noexcept;
			
			const ComPtr<ID3D12DescriptorHeap>& GetRTVHeap() const noexcept { return m_rtv_heap; };
			const ComPtr<ID3D12DescriptorHeap>& GetSRVHeap() const noexcept { return m_srv_heap; };
			void Set(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
				const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle) const noexcept;
			void Clear(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
				const DirectX::XMFLOAT4& clear_color) const noexcept;
			D3D12_RESOURCE_BARRIER GetRtvResourceBarrier(bool render_target) noexcept;
		};
	}
}