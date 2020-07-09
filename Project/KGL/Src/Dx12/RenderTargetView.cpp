#include <Dx12/RenderTargetView.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

RenderTargetView::RenderTargetView(ComPtr<ID3D12Device> device, const ComPtr<ID3D12Resource>& resource) noexcept
	: m_buffer(resource)
{
	HRESULT hr = S_OK;

	{
		// RTV用ヒープ作成
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heap_desc.NumDescriptors = 1;
		hr = device->CreateDescriptorHeap(
			&heap_desc,
			IID_PPV_ARGS(m_rtv_heap.GetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateDescriptorHeapに失敗");

		// RTV作成
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		
		device->CreateRenderTargetView(
			m_buffer.Get(),
			&rtv_desc,
			m_rtv_heap->GetCPUDescriptorHandleForHeapStart()
		);
	}

	{
		// SRV用ヒープ作成
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heap_desc.NumDescriptors = 1;
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hr = device->CreateDescriptorHeap(
			&heap_desc,
			IID_PPV_ARGS(m_srv_heap.GetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateDescriptorHeapに失敗");

		// SRV作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = m_buffer->GetDesc().Format;

		device->CreateShaderResourceView(
			m_buffer.Get(),
			&srv_desc,
			m_srv_heap->GetCPUDescriptorHandleForHeapStart()
		);
	}
}

void RenderTargetView::Set(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle) const noexcept
{
	RCHECK(!cmd_list, "cmd_list が nullptr");
	cmd_list->OMSetRenderTargets(
		1, &m_rtv_heap->GetCPUDescriptorHandleForHeapStart(), false,
		p_dsv_handle
	);
}

void RenderTargetView::Clear(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	const DirectX::XMFLOAT4& clear_color) const noexcept
{
	RCHECK(!cmd_list, "cmd_list が nullptr");
	auto rtv_handle = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
	cmd_list->ClearRenderTargetView(rtv_handle, (float*)&clear_color, 0, nullptr);
}

D3D12_RESOURCE_BARRIER RenderTargetView::GetRtvResourceBarrier(bool render_target) noexcept
{
	return render_target ?
		CD3DX12_RESOURCE_BARRIER::Transition(
			m_buffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		) :
		CD3DX12_RESOURCE_BARRIER::Transition(
			m_buffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
}