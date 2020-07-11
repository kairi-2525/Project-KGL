#pragma once

#include "Texture.hpp"

#include "../Helper/ComPtr.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class RenderTargetView
		{
			std::vector<ComPtr<ID3D12Resource>>		m_buffers;
			ComPtr<ID3D12DescriptorHeap>			m_rtv_heap;
			ComPtr<ID3D12DescriptorHeap>			m_srv_heap;
		public:
			explicit RenderTargetView(
				ComPtr<ID3D12Device> device,
				const std::vector<ComPtr<ID3D12Resource>>& resources
			) noexcept;
			
			const ComPtr<ID3D12DescriptorHeap>& GetRTVHeap() const noexcept { return m_rtv_heap; };
			const ComPtr<ID3D12DescriptorHeap>& GetSRVHeap() const noexcept { return m_srv_heap; };
			D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle(UINT num = 0u) const noexcept;
			D3D12_GPU_DESCRIPTOR_HANDLE GetRTVGPUHandle(UINT num = 0u) const noexcept;
			D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(UINT num = 0u) const noexcept;
			D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(UINT num = 0u) const noexcept;

			HRESULT Set(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
				const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle, UINT num = 0u) const noexcept;
			HRESULT SetAll(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
				const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle) const noexcept;
			HRESULT Clear(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
				const DirectX::XMFLOAT4& clear_color, UINT num = 0u) const noexcept;
			D3D12_RESOURCE_BARRIER GetRtvResourceBarrier(bool render_target, UINT num = 0u) noexcept;
		};
	}
}