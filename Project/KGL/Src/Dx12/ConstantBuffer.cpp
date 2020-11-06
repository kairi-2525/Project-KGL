#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Helper/Cast.hpp>

using namespace KGL;

HRESULT ResourcesBase::CreateCBV(std::shared_ptr<DescriptorHandle> p_handle) const noexcept
{
	HRESULT hr = S_OK;
	if (p_handle)
	{
		const auto& desc = m_buffer->GetDesc();
		// CBV—pDesc
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = m_buffer->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = SCAST<UINT>(SizeInBytes());

		ComPtr<ID3D12Device> device;
		hr = m_buffer->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
		RCHECK_HR(hr, "m_buffer->GetDevice‚ÉŽ¸”s");

		device->CreateConstantBufferView(&cbv_desc, p_handle->Cpu());
	}
	else
	{
		hr = E_FAIL;
	}
	return hr;
}