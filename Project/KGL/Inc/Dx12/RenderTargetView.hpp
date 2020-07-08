#pragma once

#include "Texture.hpp"

#include "../Helper/ComPtr.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class RenderTargetView
		{
			ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
			ComPtr<ID3D12DescriptorHeap> m_srv_heap;
		public:
			explicit RenderTargetView(ComPtr<ID3D12Device> device, const Texture& texture) noexcept;
		
			void Set(const ComPtr<ID3D12GraphicsCommandList>& cmd_list, const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle) noexcept;
		};
	}
}