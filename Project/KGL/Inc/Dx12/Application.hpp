#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include "../Helper/ComPtr.hpp"
#include "CommandQueue.hpp"
#include <DirectXMath.h>
#include <vector>
#include <memory>

namespace KGL
{
	inline namespace DX12
	{
		class Application
		{
		protected:
			ComPtr<ID3D12Device>					m_dev;
			ComPtr<IDXGISwapChain4>					m_swapchain;
			ComPtr<ID3D12DescriptorHeap>			m_rtv_heap;
			ComPtr<ID3D12DescriptorHeap>			m_dsv_heap;
			std::vector<ComPtr<ID3D12Resource>>		m_rtv_buffers;
			ComPtr<ID3D12Resource>					m_depth_buff;
			std::shared_ptr<CommandQueue>			m_cmd_queue;

			bool									m_tearing_support;
			UINT									m_max_quality_level;
			UINT									m_max_sample_count;
		public:
			static bool CheckTearingSupport();
		private:
			Application() = delete;
			Application(const Application&) = delete;
			Application& operator=(const Application&) = delete;
		protected:
			HRESULT CreateFactory(ComPtr<IDXGIFactory6>& factory, bool debug_layer, bool gbv);
			HRESULT CreateDevice(ComPtr<IDXGIFactory6> factory) noexcept;
			HRESULT CreateSwapchain(ComPtr<IDXGIFactory6> factory, HWND hwnd) noexcept;
			HRESULT CreateHeaps() noexcept;
			HRESULT CheckMaxSampleCount() noexcept;
		public:
			explicit Application(HWND hwnd, bool debug_layer = false, bool gbv = false) noexcept;
			~Application();

			void SetRtvDsv(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept;
			void SetRtv(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept;
			void ClearRtvDsv(ComPtr<ID3D12GraphicsCommandList> cmd_list,
				const DirectX::XMFLOAT4& clear_color) const noexcept
			{
				ClearRtv(cmd_list, clear_color); ClearDsv(cmd_list);
			}
			void ClearRtv(ComPtr<ID3D12GraphicsCommandList> cmd_list,
				const DirectX::XMFLOAT4& clear_color) const noexcept;
			void ClearDsv(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept;
			void SetViewPort(ComPtr<ID3D12GraphicsCommandList> cmd_list) const noexcept;
			D3D12_RESOURCE_BARRIER GetRtvResourceBarrier(bool render_target) const noexcept;

			const ComPtr<ID3D12Device>& GetDevice() const noexcept { return  m_dev; }
			const ComPtr<IDXGISwapChain4>& GetSwapchain() const noexcept { return  m_swapchain; }
			const std::shared_ptr<CommandQueue>& GetQueue() const noexcept { return  m_cmd_queue; }
			const std::vector<ComPtr<ID3D12Resource>>& GetRtvBuffers() const noexcept { return  m_rtv_buffers; }
			const ComPtr<ID3D12Resource>& GetDsvBuffer() const noexcept { return  m_depth_buff; }
			const ComPtr<ID3D12DescriptorHeap>& GetRtvHeap() const noexcept { return  m_rtv_heap; }
			const ComPtr<ID3D12DescriptorHeap>& GetDsvHeap() const noexcept { return  m_dsv_heap; }
			bool IsTearingSupport() const noexcept { return m_tearing_support; }
			UINT GetMaxQualityLevel() const noexcept { return m_max_quality_level; }
			UINT GetMaxSampleCount() const noexcept { return m_max_sample_count; }
		};

		using App = Application;
	}
}