#include "../Hrd/FXAA.hpp"
#include <Helper/Cast.hpp>
#include <DirectXTex/d3dx12.h>
#include <imgui.h>

FXAAManager::FXAAManager(ComPtrC<ID3D12Device> device, const std::shared_ptr<KGL::DXC>& dxc, const DirectX::XMUINT2& rt_size)
{
	changed = true;
	n = 0.50f;

	gpu_resource = std::make_shared<KGL::Resource<CBuffer>>(device, 1u);

	desc.buffer = DEFAULT_BUFFER;
	SetRCPFrameDesc(rt_size);
	UpdateBuffer();

	desc.type = FXAA_ON;

	{
		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/FXAA_ps.hlsl";
		renderer_desc.ps_desc.entry_point = "AlphaGray";
		gray_renderer = std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc);

		renderer_desc.ps_desc.entry_point = "PSMain";
		std::vector<D3D12_DESCRIPTOR_RANGE> cbv_range =
		{
			{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1u, 0u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
		};
		D3D12_ROOT_PARAMETER cbv_pram = 
		{
			D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			{ { SCAST<UINT>(cbv_range.size()), cbv_range.data() } },
			D3D12_SHADER_VISIBILITY_PIXEL
		};
		renderer_desc.root_params.push_back(cbv_pram);
		auto& sampler = renderer_desc.static_samplers[0];
		sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.MaxAnisotropy = 1;

		renderer = std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc);
	}

	cbv_descriptor = std::make_shared<KGL::DescriptorManager>(device, 1u);
	cbv_handle = cbv_descriptor->Alloc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = gpu_resource->Data()->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = SCAST<UINT>(gpu_resource->SizeInBytes());
	device->CreateConstantBufferView(&cbv_desc, cbv_handle.Cpu());
}

void FXAAManager::UpdateBuffer()
{
	if (changed)
	{
		changed = false;

		auto* mapped_buffer = gpu_resource->Map(0u, &CD3DX12_RANGE(0u, 0u));
		*mapped_buffer = desc.buffer;
		gpu_resource->Unmap(0u, &CD3DX12_RANGE(0u, 0u));
	}
}

DirectX::XMFLOAT2 FXAAManager::GetRCPFrame(const DirectX::XMUINT2& rt_size)
{
	return { 
		1.f / rt_size.x,
		1.f / rt_size.y
	};
}
DirectX::XMFLOAT4 FXAAManager::GetRCPFrameOpt(const DirectX::XMUINT2& rt_size, float N)
{
	return {
		-N / rt_size.x,
		-N / rt_size.y,
		N / rt_size.x,
		N / rt_size.y
	};
}

void FXAAManager::SetRCPFrameDesc(const DirectX::XMUINT2& rt_size, float N)
{
	desc.buffer.rcp_frame = GetRCPFrame(rt_size);
	desc.buffer.rcp_frame_opt = GetRCPFrameOpt(rt_size, N);
	desc.buffer.rcp_frame_opt2 = GetRCPFrameOpt2(rt_size);
}

void FXAAManager::ImGuiTreeUpdate(const DirectX::XMUINT2& rt_size)
{
	bool on_flg = desc.type == FXAA_ON;
	if (ImGui::Checkbox(u8"FXAA", &on_flg))
	{
		if (on_flg) desc.type = FXAA_ON;
		else desc.type = FXAA_OFF;
	}
	if (ImGui::TreeNode(u8"FXAA Option"))
	{
		if (ImGui::SliderFloat("quality_edge_threshold", &desc.buffer.quality_edge_threshold, 0.063f, 0.333f)) changed = true;
		if (ImGui::SliderFloat("quality_edge_threshold_min", &desc.buffer.quality_edge_threshold_min, 0.0312f, 0.0833f)) changed = true;
		if (ImGui::SliderFloat("console_edge_threshold", &desc.buffer.console_edge_threshold, 0.125f, 0.25f)) changed = true;
		if (ImGui::SliderFloat("console_edge_threshold_min", &desc.buffer.console_edge_threshold_min, 0.04f, 0.06f)) changed = true;
		if (ImGui::SliderFloat("edge_sharpness", &desc.buffer.edge_sharpness, 2.0f, 8.0f)) changed = true;
		if (ImGui::SliderFloat("subpix", &desc.buffer.subpix, 0.00f, 1.00f)) changed = true;
		if (ImGui::SliderFloat("n", &n, 0.33f, 0.50f))
		{
			desc.buffer.rcp_frame_opt = GetRCPFrameOpt(rt_size, n);
			changed = true;
		}
		ImGui::TreePop();
	}
}

void FXAAManager::SetState(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list)
{
	renderer->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, cbv_handle.Heap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(1, cbv_handle.Gpu());
}

void FXAAManager::SetGrayState(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list)
{
	gray_renderer->SetState(cmd_list);
}