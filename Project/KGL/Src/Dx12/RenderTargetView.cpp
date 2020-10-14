#include <Dx12/RenderTargetView.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

RenderTargetView::RenderTargetView(
	ComPtr<ID3D12Device> device,
	const std::vector<ComPtr<ID3D12Resource>>& resources,
	const std::shared_ptr<KGL::DescriptorManager>& rtv_desc_mgr,
	D3D12_SRV_DIMENSION srv_dimension
) noexcept
{
	m_buffers = resources;
	HRESULT hr = S_OK;
	const auto size = m_buffers.size();
	if (rtv_desc_mgr)
	{
		m_rtv_mgr = rtv_desc_mgr;
		m_rtv_handles.resize(size);
	}
	else
	{
		{
			// RTV用ヒープ作成
			D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
			heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			heap_desc.NumDescriptors = SCAST<UINT>(size);
			hr = device->CreateDescriptorHeap(
				&heap_desc,
				IID_PPV_ARGS(m_rtv_heap.GetAddressOf())
			);
			RCHECK(FAILED(hr), "CreateDescriptorHeapに失敗");

		}
	}

	{
		// SRV用ヒープ作成
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heap_desc.NumDescriptors = SCAST<UINT>(size);
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		hr = device->CreateDescriptorHeap(
			&heap_desc,
			IID_PPV_ARGS(m_srv_heap.GetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateDescriptorHeapに失敗");
	}

	// RTV用Desc
	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};

	// SRV用Desc
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = srv_dimension;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	auto srv_handle = m_srv_heap->GetCPUDescriptorHandleForHeapStart();
	const auto icmt_srv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	if (m_rtv_mgr)
	{
		for (auto i = 0; i < size; i++)
		{
			if (!m_buffers[i]) continue;
			m_rtv_handles[i] = m_rtv_mgr->Alloc();
			srv_desc.Format = rtv_desc.Format = m_buffers[i]->GetDesc().Format;
			rtv_desc.ViewDimension = m_buffers[i]->GetDesc().SampleDesc.Count > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
			device->CreateRenderTargetView(
				m_buffers[i].Get(),
				&rtv_desc,
				m_rtv_handles[i].Cpu()
			);

			device->CreateShaderResourceView(
				m_buffers[i].Get(),
				&srv_desc,
				srv_handle
			);

			srv_handle.ptr += icmt_srv;
		}
	}
	else
	{
		// 各ハンドル
		auto rtv_handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		for (auto i = 0; i < size; i++)
		{
			if (!m_buffers[i]) continue;


			rtv_desc.ViewDimension = m_buffers[i]->GetDesc().SampleDesc.Count > 1 ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
			srv_desc.Format = rtv_desc.Format = m_buffers[i]->GetDesc().Format;
			device->CreateRenderTargetView(
				m_buffers[i].Get(),
				&rtv_desc,
				rtv_handle
			);

			device->CreateShaderResourceView(
				m_buffers[i].Get(),
				&srv_desc,
				srv_handle
			);

			rtv_handle.ptr += icmt_rtv;
			srv_handle.ptr += icmt_srv;
		}
	}
}

RenderTargetView::~RenderTargetView()
{
	if (m_rtv_mgr)
	{
		for (const auto& handle : m_rtv_handles)
		{
			m_rtv_mgr->Free(handle);
		}
	}
}

HRESULT RenderTargetView::GetDevice(ComPtr<ID3D12Device>* p_device) const noexcept
{
	RCHECK(!p_device, "p_device が nullptr", E_FAIL);
	if (m_rtv_mgr)
	{
		return m_rtv_mgr->Heap()->GetDevice(IID_PPV_ARGS(p_device->GetAddressOf()));
	}
	return m_rtv_heap->GetDevice(IID_PPV_ARGS(p_device->GetAddressOf()));
}

HRESULT RenderTargetView::Set(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle, UINT start_num, UINT count) const noexcept
{
	RCHECK(!cmd_list, "cmd_list が nullptr", E_FAIL);
	RCHECK(start_num + count > m_buffers.size(), "渡された値が大きすぎます", E_FAIL);
	RCHECK(count == 0u, "渡された値が小さすぎます", E_FAIL);

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(count);
	UINT idx = 0u;
	for (auto& handle : handles)
	{
		handle = GetRTVCPUHandle(start_num + idx);
		idx++;
	}

	cmd_list->OMSetRenderTargets(
		count, handles.data(), false,
		p_dsv_handle
	);

	return S_OK;
}

HRESULT RenderTargetView::SetAll(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle) const noexcept
{
	RCHECK(!cmd_list, "cmd_list が nullptr", E_FAIL);
	
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(m_buffers.size());
	if (m_rtv_mgr)
	{
		UINT i = 0u;
		for (auto& it : handles)
		{
			it = m_rtv_handles[i].Cpu();
			i++;
		}
	}
	else
	{
		ComPtr<ID3D12Device> device;
		auto hr = GetDevice(&device);
		RCHECK(FAILED(hr), "GetDeviceに失敗", hr);
		const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		for (auto& it : handles)
		{
			it = handle;
			handle.ptr += icmt_rtv;
		}
	}
	
	cmd_list->OMSetRenderTargets(
		SCAST<UINT>(handles.size()), handles.data(), false,
		p_dsv_handle
	);

	return S_OK;
}

HRESULT RenderTargetView::Clear(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	const float* clear_color, UINT start_num, UINT count) const noexcept
{
	RCHECK(!cmd_list, "cmd_list が nullptr", E_FAIL);
	RCHECK(start_num + count > m_buffers.size(), "渡された値が大きすぎます", E_FAIL);
	RCHECK(count == 0u, "渡された値が小さすぎます", E_FAIL);
	RCHECK(!clear_color, "clear_color が nullptr", E_FAIL);

	for (UINT i = 0u; i < count; i++)
	{
		cmd_list->ClearRenderTargetView(GetRTVCPUHandle(start_num + i), clear_color, 0, nullptr);
	}

	return S_OK;
}

D3D12_RESOURCE_BARRIER RenderTargetView::GetRtvResourceBarrier(bool render_target, UINT num) noexcept
{
	RCHECK(num >= m_buffers.size(), "num の値が大きすぎます", {});

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

std::vector<D3D12_RESOURCE_BARRIER> RenderTargetView::GetRtvResourceBarriers(bool render_target, UINT start_num, UINT count) noexcept
{
	RCHECK(SCAST<size_t>(start_num) + count > m_buffers.size(), "渡された値が大きすぎます", {});
	RCHECK(count == 0, "渡された値が小さすぎます", {});
	const size_t buffer_num = m_buffers.size();
	std::vector<D3D12_RESOURCE_BARRIER> rbs(count);
	for (UINT i = 0; i < count; i++)
	{
		rbs[i] = GetRtvResourceBarrier(render_target, start_num + i);
	}
	return std::move(rbs);
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView::GetRTVCPUHandle(UINT num) const noexcept
{
	RCHECK(num >= m_buffers.size(), "num の値が大きすぎます", {});
	if (m_rtv_mgr)
	{
		return m_rtv_handles[num].Cpu();
	}
	else
	{
		ComPtr<ID3D12Device> device;
		auto hr = GetDevice(&device);
		RCHECK(FAILED(hr), "GetDeviceに失敗", {});

		const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += SCAST<UINT64>(num * icmt_rtv);
		return handle;
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderTargetView::GetRTVGPUHandle(UINT num) const noexcept
{
	RCHECK(num >= m_buffers.size(), "num の値が大きすぎます", {});
	if (m_rtv_mgr)
	{
		return m_rtv_handles[num].Gpu();
	}
	else
	{
		ComPtr<ID3D12Device> device;
		auto hr = GetDevice(&device);
		RCHECK(FAILED(hr), "GetDeviceに失敗", {});

		const auto icmt_rtv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto handle = m_rtv_heap->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += SCAST<UINT64>(num * icmt_rtv);
		return handle;
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView::GetSRVCPUHandle(UINT num) const noexcept
{
	RCHECK(num >= m_buffers.size(), "num の値が大きすぎます", {});
	ComPtr<ID3D12Device> device;
	auto hr = m_srv_heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	RCHECK(FAILED(hr), "GetDeviceに失敗", {});

	const auto icmt_srv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto handle = m_srv_heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += SCAST<UINT64>(num * icmt_srv);
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderTargetView::GetSRVGPUHandle(UINT num) const noexcept
{
	RCHECK(num >= m_buffers.size(), "num の値が大きすぎます", {});
	ComPtr<ID3D12Device> device;
	auto hr = m_srv_heap->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	RCHECK(FAILED(hr), "GetDeviceに失敗", {});

	const auto icmt_srv = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto handle = m_srv_heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += SCAST<UINT64>(num * icmt_srv);
	return handle;
}