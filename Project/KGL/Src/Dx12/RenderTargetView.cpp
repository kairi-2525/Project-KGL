#include <Dx12/RenderTargetView.hpp>

using namespace KGL;

RenderTargetView::RenderTargetView(ComPtr<ID3D12Device> device, const Texture& texture) noexcept
{
	HRESULT hr = S_OK;

	{
		// RTV—pƒq[ƒvì¬
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heap_desc.NumDescriptors = 1;
		hr = device->CreateDescriptorHeap(
			&heap_desc,
			IID_PPV_ARGS(m_rtv_heap.GetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateDescriptorHeap‚ÉŽ¸”s");

		// RTVì¬
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		
		device->CreateRenderTargetView(
			texture.Data().Get(),
			&rtv_desc,
			m_rtv_heap->GetCPUDescriptorHandleForHeapStart()
		);
	}

	{
		// SRV—pƒq[ƒvì¬
		D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
		heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heap_desc.NumDescriptors = 1;
		heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		hr = device->CreateDescriptorHeap(
			&heap_desc,
			IID_PPV_ARGS(m_srv_heap.GetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateDescriptorHeap‚ÉŽ¸”s");

		// SRVì¬
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = texture.Data()->GetDesc().Format;

		device->CreateShaderResourceView(
			texture.Data().Get(),
			&srv_desc,
			m_rtv_heap->GetCPUDescriptorHandleForHeapStart()
		);
	}
}

void RenderTargetView::Set(const ComPtr<ID3D12GraphicsCommandList>& cmd_list,
	const D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle)
{
	cmd_list->OMSetRenderTargets(
		1, &m_rtv_heap->GetCPUDescriptorHandleForHeapStart(), false,
		p_dsv_handle
	);
}