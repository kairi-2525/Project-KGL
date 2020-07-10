#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class CommandQueue
		{
		private:
			ComPtr<ID3D12CommandQueue>		m_queue;
			ComPtr<ID3D12Fence>				m_fence;
			UINT64							m_value;
		private:
			CommandQueue() = delete;
			CommandQueue(const CommandQueue&) = delete;
			CommandQueue& operator=(const CommandQueue&) = delete;
		public:
			explicit CommandQueue(ComPtrC<ID3D12CommandQueue> queue, ComPtrC<ID3D12Fence> fence) noexcept;
			explicit CommandQueue(ComPtrC<ID3D12Device> device, const D3D12_COMMAND_QUEUE_DESC& desc) noexcept;
			HRESULT Signal() noexcept;
			HRESULT Wait() const noexcept { return Wait(m_value); }
			HRESULT Wait(UINT64 value) const noexcept;

			const ComPtr<ID3D12CommandQueue>& Data() const noexcept { return m_queue; }
			UINT64 GetValue() const noexcept { return m_value; }
		};
	}
}