#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Helper/Cast.hpp>

using namespace KGL;

ResourcesBase::ResourcesBase(
	ComPtrC<ID3D12Device> device,
	size_t size, size_t struct_size,
	const D3D12_HEAP_PROPERTIES* prop,
	D3D12_RESOURCE_FLAGS flag,
	D3D12_RESOURCE_STATES state
) noexcept :
	m_size(size)
{
	RCHECK(m_size == 0u, "無効なサイズ");
	m_size_in_bytes = AlignmentStructSize(m_size, struct_size);
	D3D12_RESOURCE_DESC res_desc = {};
	res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	res_desc.Alignment = 0u;
	res_desc.Width = m_size_in_bytes;
	res_desc.Height = 1u;
	res_desc.DepthOrArraySize = 1u;
	res_desc.MipLevels = 1u;
	res_desc.Format = DXGI_FORMAT_UNKNOWN;
	res_desc.SampleDesc.Count = 1;
	res_desc.SampleDesc.Quality = 0;
	res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	res_desc.Flags = flag;
	if (prop)
	{
		auto hr = device->CreateCommittedResource(
			prop,
			D3D12_HEAP_FLAG_NONE,
			&res_desc,
			state,
			nullptr,
			IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateCommittedResourceに失敗");
	}
	else
	{
		D3D12_HEAP_PROPERTIES propeties = {};
		propeties.Type = D3D12_HEAP_TYPE_UPLOAD;
		propeties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		propeties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		propeties.CreationNodeMask = 1;
		propeties.VisibleNodeMask = 1;
		auto hr = device->CreateCommittedResource(
			&propeties,
			D3D12_HEAP_FLAG_NONE,
			&res_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateCommittedResourceに失敗");
	}
}

HRESULT ResourcesBase::CreateCBV(
	D3D12_GPU_VIRTUAL_ADDRESS address,
	UINT size,
	std::shared_ptr<DescriptorHandle> p_handle
) const noexcept
{
	HRESULT hr = S_OK;
	if (p_handle)
	{
		const auto& desc = m_buffer->GetDesc();
		// CBV用Desc
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = address;
		cbv_desc.SizeInBytes = SCAST<UINT>(AlignmentSize(size));

		ComPtr<ID3D12Device> device;
		hr = m_buffer->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
		RCHECK_HR(hr, "m_buffer->GetDeviceに失敗");

		device->CreateConstantBufferView(&cbv_desc, p_handle->Cpu());
	}
	else
	{
		hr = E_FAIL;
	}
	return hr;
}

ResourcesBase& ResourcesBase::operator=(const ResourcesBase& m) noexcept
{
	m_size = m.m_size;
	m_size_in_bytes = m.m_size_in_bytes;

	ComPtr<ID3D12Device> device;
	m.Data()->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));

	// リソースを作成
	auto resource_desc = m.Data()->GetDesc();
	D3D12_HEAP_PROPERTIES propeties = {};
	m.Data()->GetHeapProperties(&propeties, nullptr);

	auto hr = device->CreateCommittedResource(
		&propeties,
		D3D12_HEAP_FLAG_NONE,
		&resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())
	);

	// バッファの中身をコピー
	const D3D12_RANGE read_range = { 0, 0 };
	void* dest_ptr, * src_ptr;
	m_buffer->Map(0u, &read_range, &dest_ptr);
	m.m_buffer->Map(0u, &read_range, &src_ptr);
	std::memcpy(dest_ptr, src_ptr, SCAST<size_t>(m.SizeInBytes()));
	m.m_buffer->Unmap(0u, &read_range);
	m_buffer->Unmap(0u, &read_range);

	return *this;
}