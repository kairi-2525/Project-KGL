#include "../Hrd/Bloom.hpp"
#include "../Hrd/MSAA.hpp"
#include <DirectXTex/d3dx12.h>

BloomGenerator::BloomGenerator(KGL::ComPtrC<ID3D12Device> device,
	const std::shared_ptr<KGL::DXC>& dxc, KGL::ComPtrC<ID3D12Resource> rsc,
	DXGI_SAMPLE_DESC max_sample_desc)
{
	constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	auto desc = rsc->GetDesc();
	const auto dx_clear_value = CD3DX12_CLEAR_VALUE(desc.Format, (float*)&clear_value);

	UINT renderer_count = MSAASelector::CountToType(max_sample_desc.Count) + 1u;

	rtv_rs.resize(renderer_count);
	auto rtv_rs_itr = rtv_rs.begin();
	const UINT width = desc.Width, height = desc.Height;
	for (UINT i = 1u; i <= max_sample_desc.Count; i *= 2)
	{
		desc.Width = width;
		desc.Height = height;
		desc.SampleDesc.Count = i;

		std::vector<KGL::ComPtr<ID3D12Resource>> resources;
		resources.reserve(RTV_MAX);
		for (auto& tex : (rtv_rs_itr->textures))
		{
			desc.Width = std::max<UINT>(1, desc.Width / 2);
			desc.Height = std::max<UINT>(1, desc.Height / 2);;

			tex = std::make_shared<KGL::Texture>(
				device, desc, dx_clear_value);

			resources.emplace_back(tex->Data());
		}
		rtv_rs_itr->rtvs = 
			std::make_shared<KGL::RenderTargetView>(
				device, resources, nullptr,
				i == 1u ? D3D12_SRV_DIMENSION_TEXTURE2D : D3D12_SRV_DIMENSION_TEXTURE2DMS
			);

		rtv_rs_itr++;
	}

	sprite = std::make_shared<KGL::Sprite>(device);

	auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;

	renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
	auto& sampler = renderer_desc.static_samplers[0];
	sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	
	bloom_renderers.reserve(renderer_count);
	bloom_renderers.push_back(std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc));
	renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
	renderer_desc.ps_desc.entry_point = "PSMainMS";
	// MSAA—p
	for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
	{
		renderer_desc.other_desc.sample_desc.Count *= 2;
		bloom_renderers.push_back(std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc));
	}
	renderer_desc.other_desc.sample_desc.Count = 1u;
	renderer_desc.rastarizer_desc.MultisampleEnable = FALSE;
	renderer_desc.ps_desc.entry_point = "PSMain";

	renderer_desc.blend_types[0] = KGL::BDTYPE::ADD;
	renderer_desc.ps_desc.hlsl = "./HLSL/2D/Bloom_ps.hlsl";

	auto root_param0 = CD3DX12_ROOT_PARAMETER();
	auto range0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 0);
	root_param0.InitAsDescriptorTable(1u, &range0, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params[0] = root_param0;
	auto root_param1 = CD3DX12_ROOT_PARAMETER();
	auto range1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	root_param1.InitAsDescriptorTable(1u, &range1, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params.push_back(root_param1);

	renderers.reserve(renderer_count);
	renderers.push_back(std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc));
	renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
	renderer_desc.ps_desc.entry_point = "PSMainMS";
	// MSAA—p
	for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
	{
		renderer_desc.other_desc.sample_desc.Count *= 2;
		renderers.push_back(std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc));
	}

	buffer_res = std::make_shared<KGL::Resource<Buffer>>(device, 1u);
	SetKernel(KGL::SCAST<UINT8>(RTV_MAX));
	{
		Weights weight;
		weight[0] = 1.f;
		for (UINT i = 1; i < RTV_MAX; i++)
			weight[i] = weight[i - 1u] * 1.f;
		SetWeights(weight);
	}

	rtv_num_dsmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	rtv_num_handle = rtv_num_dsmgr->Alloc();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = buffer_res->Data()->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = buffer_res->SizeInBytes();
	device->CreateConstantBufferView(&cbv_desc, rtv_num_handle.Cpu());
}

void BloomGenerator::Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
	KGL::ComPtrC<ID3D12DescriptorHeap> srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle,
	D3D12_VIEWPORT view_port, UINT msaa_scale)
{
	const auto return_view_port = view_port;

	D3D12_RECT scissor_rect;
	scissor_rect.left = scissor_rect.top = 0u;
	scissor_rect.right = view_port.Width;
	scissor_rect.bottom = view_port.Height;
	const auto return_scissor_rect = scissor_rect;
	const auto& rtvs = rtv_rs[msaa_scale].rtvs;
	const auto& textures = rtv_rs[msaa_scale].textures;

	const auto& rtrb = rtvs->GetRtvResourceBarriers(true);
	cmd_list->ResourceBarrier(rtrb.size(), rtrb.data());
	
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	bloom_renderers[msaa_scale]->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, srv_heap.GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(0, srv_gpu_handle);

	UINT32 rtv_num = 0u;
	{
		Buffer* mapped_buffer = buffer_res->Map(0, &CD3DX12_RANGE(0, 0));
		rtv_num = mapped_buffer->kernel;
		buffer_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
	for (UINT32 idx = 0u; idx < rtv_num; idx++)
	{
		rtvs->Set(cmd_list, nullptr, idx);
		rtvs->Clear(cmd_list, textures[idx]->GetClearColor(), idx);
		const auto& desc = textures[idx]->Data()->GetDesc();

		scissor_rect.right = view_port.Width = desc.Width;
		scissor_rect.bottom = view_port.Height = desc.Height;

		cmd_list->RSSetViewports(1, &view_port);
		cmd_list->RSSetScissorRects(1, &scissor_rect);

		sprite->Render(cmd_list);
	}
	const auto& srvrb = rtvs->GetRtvResourceBarriers(false);
	cmd_list->ResourceBarrier(srvrb.size(), srvrb.data());

	cmd_list->RSSetViewports(1, &return_view_port);
	cmd_list->RSSetScissorRects(1, &return_scissor_rect);
}

void BloomGenerator::Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list, UINT msaa_scale)
{
	const auto& rtvs = rtv_rs[msaa_scale].rtvs;

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	renderers[msaa_scale]->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(0));

	cmd_list->SetDescriptorHeaps(1, rtv_num_handle.Heap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(1, rtv_num_handle.Gpu());

	sprite->Render(cmd_list);
}

void BloomGenerator::SetKernel(UINT8 num) noexcept
{
	Buffer* mapped_buffer = buffer_res->Map(0, &CD3DX12_RANGE(0, 0));
	mapped_buffer->kernel = (std::min)(KGL::SCAST<UINT32>(RTV_MAX), KGL::SCAST<UINT32>(num));
	buffer_res->Unmap(0, &CD3DX12_RANGE(0, 0));
}

UINT8 BloomGenerator::GetKernel() const noexcept
{
	UINT8 result;
	Buffer* mapped_buffer = buffer_res->Map(0, &CD3DX12_RANGE(0, 0));
	result = KGL::SCAST<UINT8>(mapped_buffer->kernel);
	buffer_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	return result;
}

void BloomGenerator::SetWeights(Weights weights) noexcept
{
	Buffer* mapped_buffer = buffer_res->Map(0, &CD3DX12_RANGE(0, 0));
	mapped_buffer->weight = weights;
	buffer_res->Unmap(0, &CD3DX12_RANGE(0, 0));
}

BloomGenerator::Weights BloomGenerator::GetWeights() const noexcept
{
	Weights result;
	Buffer* mapped_buffer = buffer_res->Map(0, &CD3DX12_RANGE(0, 0));
	result = mapped_buffer->weight;
	buffer_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	return result;
}