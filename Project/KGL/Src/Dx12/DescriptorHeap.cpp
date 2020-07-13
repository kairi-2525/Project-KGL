#include <Dx12/DescriptorHeap.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Cast.hpp>

using namespace KGL;

DescriptorHandle::DescriptorHandle() noexcept :
	m_cpu_handle(), m_gpu_handle()
{

}

DescriptorHandle::DescriptorHandle(
	ComPtrC<ID3D12DescriptorHeap> heap,
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle
) noexcept :
	m_heap(heap), m_cpu_handle(cpu_handle), m_gpu_handle(gpu_handle)
{

}

DescriptorManager::DescriptorManager(
	ComPtrC<ID3D12Device> device,
	size_t amount,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	D3D12_DESCRIPTOR_HEAP_FLAGS flags
) noexcept
{
	RCHECK(amount == 0u, "amount が 0");
	m_create_amount = amount;
	m_icmt_size = device->GetDescriptorHandleIncrementSize(type);
	m_type = type;
	m_flags = flags;

	Create(device, m_create_amount);
}

HRESULT DescriptorManager::Create(ComPtrC<ID3D12Device> device, size_t amount)
{
	if (amount == 0u) amount = m_create_amount;
	auto& itr = m_descs.emplace_back();

	D3D12_DESCRIPTOR_HEAP_DESC mat_heap_desc = {};
	mat_heap_desc.Flags = m_flags;
	mat_heap_desc.NodeMask = 0;
	mat_heap_desc.NumDescriptors = SCAST<UINT>(amount);
	mat_heap_desc.Type = m_type;
	auto hr = device->CreateDescriptorHeap(
		&mat_heap_desc, IID_PPV_ARGS(itr.heap.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateDescriptorHeapに失敗", hr);

	auto cpu_handle = itr.heap->GetCPUDescriptorHandleForHeapStart();
	auto gpu_handle = itr.heap->GetGPUDescriptorHandleForHeapStart();
	for (size_t i = 0; i < amount; i++)
	{
		itr.free_handles.emplace_back(itr.heap, cpu_handle, gpu_handle);
		cpu_handle.ptr += m_icmt_size;
		gpu_handle.ptr += m_icmt_size;
	}
	return hr;
}

// 新しいハンドルを確保する。
DescriptorHandle DescriptorManager::Alloc() noexcept
{
	for (auto& desc : m_descs)
	{
		if (desc.free_handles.size() > 0)
		{
			auto ret = desc.free_handles.front();
			desc.free_handles.pop_front();
			return ret;
		}
	}
	if (m_descs.size() != 0)
	{
		ComPtr<ID3D12Device> device;
		auto hr = m_descs.begin()->heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
		RCHECK(FAILED(hr), "GetDeviceに失敗", {});
		Create(device);
		return Alloc();
	}
	return {};
}

// ハンドルを再利用可能に
void DescriptorManager::Free(const DescriptorHandle& handle) noexcept
{
	for (auto& desc : m_descs)
	{
		if (desc.heap == handle.Heap())
		{
			desc.free_handles.emplace_back(handle);
		}
	}
}