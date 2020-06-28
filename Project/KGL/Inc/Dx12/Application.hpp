#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include "../Helper/ComPtr.hpp"

#include <vector>

namespace KGL
{
	inline namespace DX12
	{
		class Application
		{
		protected:
			ComPtr<ID3D12Device>					m_dev;
			ComPtr<ID3D12CommandQueue>				m_cmd_queue;
			ComPtr<IDXGISwapChain4>					m_swapchain;
			ComPtr<ID3D12DescriptorHeap>			m_rtv_heap;
			ComPtr<ID3D12DescriptorHeap>			m_dsv_heap;
			std::vector<ComPtr<ID3D12Resource>>		m_rtv_buffers;
			ComPtr<ID3D12Resource>					m_depth_buff;

			bool									m_tearing_support;
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
			HRESULT CreateHeaps();
		public:
			explicit Application(HWND hwnd, bool debug_layer = false, bool gbv = false) noexcept;
			~Application() = default;

			ComPtr<ID3D12Device> GetDevice() const noexcept { return  m_dev; };
		};

		using App = Application;
	}
}