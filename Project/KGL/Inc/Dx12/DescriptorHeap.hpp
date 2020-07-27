#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Helper/ComPtr.hpp"
#include <vector>
#include <list>

namespace KGL
{
	inline namespace DX12
	{
		class DescriptorHandle
		{
		private:
			D3D12_CPU_DESCRIPTOR_HANDLE			m_cpu_handle;
			D3D12_GPU_DESCRIPTOR_HANDLE			m_gpu_handle;
			ComPtr<ID3D12DescriptorHeap>		m_heap;
		public:
			D3D12_CPU_DESCRIPTOR_HANDLE			Cpu() const noexcept;
			D3D12_GPU_DESCRIPTOR_HANDLE			Gpu() const noexcept;
			ComPtrC<ID3D12DescriptorHeap>		Heap() const noexcept;
		public:
			DescriptorHandle() noexcept;
			DescriptorHandle(
				ComPtrC<ID3D12DescriptorHeap> heap,
				D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
				D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle
			) noexcept;
		};


		class DescriptorManager
		{
		private:
			struct HeapDesc
			{
				ComPtr<ID3D12DescriptorHeap>	heap;
				std::list<DescriptorHandle>		free_handles;
			};
		private:
			std::vector<HeapDesc>		m_descs;
			size_t						m_icmt_size;
			size_t						m_create_amount;
			D3D12_DESCRIPTOR_HEAP_TYPE	m_type;
			D3D12_DESCRIPTOR_HEAP_FLAGS m_flags;
		public:
			explicit DescriptorManager(
				ComPtrC<ID3D12Device> device,
				size_t amount,
				D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
			) noexcept;

			HRESULT Create(ComPtrC<ID3D12Device> device, size_t amount = 0u);
			// 新しいハンドルを確保する。
			[[nodiscard]] DescriptorHandle Alloc() noexcept;
			// ハンドルを再利用可能に
			void Free(const DescriptorHandle& handle) noexcept;
			ComPtrC<ID3D12DescriptorHeap> Heap(UINT num = 0u) const noexcept { return m_descs[num].heap; }
		};
	}
}


namespace KGL
{
	inline namespace DX12
	{
		inline D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle::Cpu() const noexcept
		{
			return m_cpu_handle;
		}
		inline D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHandle::Gpu() const noexcept
		{
			return m_gpu_handle;
		}
		inline ComPtrC<ID3D12DescriptorHeap> DescriptorHandle::Heap() const noexcept
		{
			return m_heap;
		}
	}
}