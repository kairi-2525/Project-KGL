#include <Dx12/RenderTargetView.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

RenderTargetView::RenderTargetView(
	ComPtr<ID3D12Device> device,
	const std::vector<ComPtr<ID3D12Resource>>& resources
) noexcept
{
	m_buffers = resources;
	HRESULT hr = S_OK;
	const auto size = m_buffers.size();
	{
		// RTV�p�q�[�v�쐬
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heap_desc.NumDescriptors = SCAST<UINT>(size);
		hr = device->CreateDescriptorHeap(
			&heap_desc,
			IID_PPV_ARGS(m_rtv_heap.GetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateDescriptorHeap�Ɏ��s");

	}

	{
		// SRV�p�q�[�v�쐬
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heap_desc.NumDescriptors = SCAST<UINT>(size);
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hr = device->CreateDescriptorHeap(
			&heap_desc,
			IID_PPV_ARGS(m_srv_heap.GetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateDescriptorHeap�Ɏ��s");
	}

	// RTV�pDesc
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// SRV�pDesc
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// �e�n���h��
	auto rtv_handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	auto srv_handle = m_srv_heap->GetCPUDescriptorHandleForHeapStart();
	const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const auto icmt_srv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (auto i = 0; i < size; i++)
	{
		device->CreateRenderTargetView(
			m_buffers[i].Get(),
			&rtv_desc,
			rtv_handle
		);

		srv_desc.Format = m_buffers[i]->GetDesc().Format;
		device->CreateShaderResourceView(
			m_buffers[i].Get(),
			&srv_desc,
			srv_handle
		);

		rtv_handle.ptr += icmt_rtv;
		srv_handle.ptr += icmt_srv;
	}
}

HRESULT RenderTargetView::Set(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle, UINT num) const noexcept
{
	RCHECK(!cmd_list, "cmd_list �� nullptr", E_FAIL);
	RCHECK(num >= m_buffers.size(), "num �̒l���傫�����܂�", E_FAIL);

	ComPtr<ID3D12Device> device;
	auto hr = m_rtv_heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	RCHECK(FAILED(hr), "GetDevice�Ɏ��s", hr);

	const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += num * icmt_rtv;
	cmd_list->OMSetRenderTargets(
		1, &handle, false,
		p_dsv_handle
	);

	return hr;
}

HRESULT RenderTargetView::SetAll(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle) const noexcept
{
	RCHECK(!cmd_list, "cmd_list �� nullptr", E_FAIL);

	ComPtr<ID3D12Device> device;
	auto hr = m_rtv_heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	RCHECK(FAILED(hr), "GetDevice�Ɏ��s", hr);

	const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(m_buffers.size());
	for (auto& it : handles)
	{
		it = handle;
		handle.ptr = icmt_rtv;
	}
	cmd_list->OMSetRenderTargets(
		handles.size(), handles.data(), false,
		p_dsv_handle
	);

	return hr;
}

HRESULT RenderTargetView::Clear(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	const DirectX::XMFLOAT4& clear_color, UINT num) const noexcept
{
	RCHECK(!cmd_list, "cmd_list �� nullptr", E_FAIL);
	RCHECK(num >= m_buffers.size(), "num �̒l���傫�����܂�", E_FAIL);

	ComPtr<ID3D12Device> device;
	auto hr = m_rtv_heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	RCHECK(FAILED(hr), "GetDevice�Ɏ��s", hr);

	const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += num * icmt_rtv;

	cmd_list->ClearRenderTargetView(handle, (float*)&clear_color, 0, nullptr);

	return hr;
}

D3D12_RESOURCE_BARRIER RenderTargetView::GetRtvResourceBarrier(bool render_target, UINT num) noexcept
{
	RCHECK(num >= m_buffers.size(), "num �̒l���傫�����܂�", {});

	return render_target ?
		CD3DX12_RESOURCE_BARRIER::Transition(
			m_buffers[num].Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		) :
		CD3DX12_RESOURCE_BARRIER::Transition(
			m_buffers[num].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView::GetRTVCPUHandle(UINT num) const noexcept
{
	RCHECK(num >= m_buffers.size(), "num �̒l���傫�����܂�", {});
	ComPtr<ID3D12Device> device;
	auto hr = m_rtv_heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	RCHECK(FAILED(hr), "GetDevice�Ɏ��s", {});

	const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += num * icmt_rtv;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderTargetView::GetRTVGPUHandle(UINT num) const noexcept
{
	RCHECK(num >= m_buffers.size(), "num �̒l���傫�����܂�", {});
	ComPtr<ID3D12Device> device;
	auto hr = m_rtv_heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	RCHECK(FAILED(hr), "GetDevice�Ɏ��s", {});

	const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto handle = m_rtv_heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += num * icmt_rtv;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView::GetSRVCPUHandle(UINT num) const noexcept
{
	RCHECK(num >= m_buffers.size(), "num �̒l���傫�����܂�", {});
	ComPtr<ID3D12Device> device;
	auto hr = m_srv_heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	RCHECK(FAILED(hr), "GetDevice�Ɏ��s", {});

	const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto handle = m_srv_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += num * icmt_rtv;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderTargetView::GetSRVGPUHandle(UINT num) const noexcept
{
	RCHECK(num >= m_buffers.size(), "num �̒l���傫�����܂�", {});
	ComPtr<ID3D12Device> device;
	auto hr = m_srv_heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	RCHECK(FAILED(hr), "GetDevice�Ɏ��s", {});

	const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto handle = m_srv_heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += num * icmt_rtv;
	return handle;
}