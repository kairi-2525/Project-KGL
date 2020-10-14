#include "../Hrd/DepthOfField.hpp"
#include <DirectXTex/d3dx12.h>

DOFGenerator::DOFGenerator(KGL::ComPtrC<ID3D12Device> device,
	const std::shared_ptr<KGL::DXC>& dxc, KGL::ComPtrC<ID3D12Resource> rsc)
{
	constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	auto desc = rsc->GetDesc();
	const auto dx_clear_value = CD3DX12_CLEAR_VALUE(desc.Format, (float*)&clear_value);

	std::vector<KGL::ComPtr<ID3D12Resource>> resources;
	resources.reserve(rtv_textures.size());
	for (auto& tex : rtv_textures)
	{
		tex = std::make_shared<KGL::Texture>(
			device, desc, dx_clear_value);

		resources.emplace_back(tex->Data());

		desc.Width /= 2;
		desc.Height /= 2;
	}

	rtvs = std::make_shared<KGL::RenderTargetView>(device, resources);

	sprite = std::make_shared<KGL::Sprite>(device);

	auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
	renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
	auto& sampler = renderer_desc.static_samplers[0];
	sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	bloom_renderer = std::make_shared<KGL::_2D::Renderer>(device, dxc, renderer_desc);

	renderer_desc.blend_types[0] = KGL::BDTYPE::ADD;
	renderer_desc.ps_desc.hlsl = "./HLSL/2D/DepthOfField_ps.hlsl";

	renderer_desc.root_params.clear();
	renderer_desc.root_params.reserve(3u);
	auto root_param0 = CD3DX12_ROOT_PARAMETER();
	auto range0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	root_param0.InitAsDescriptorTable(1u, &range0, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params.push_back(root_param0);
	auto root_param1 = CD3DX12_ROOT_PARAMETER();
	auto range1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 1);
	root_param1.InitAsDescriptorTable(1u, &range1, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params.push_back(root_param1);
	auto root_param2 = CD3DX12_ROOT_PARAMETER();
	auto range2 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	root_param2.InitAsDescriptorTable(1u, &range2, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params.push_back(root_param2);
	renderer = std::make_shared<KGL::_2D::Renderer>(device, dxc, renderer_desc);

	rtv_num_res = std::make_shared<KGL::Resource<UINT32>>(device, 1u);
	SetRtvNum(KGL::SCAST<UINT8>(rtv_textures.size()));

	rtv_num_dsmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	rtv_num_handle = rtv_num_dsmgr->Alloc();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = rtv_num_res->Data()->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = SCAST<UINT>(rtv_num_res->SizeInBytes());
	device->CreateConstantBufferView(&cbv_desc, rtv_num_handle.Cpu());
}

void DOFGenerator::Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
	KGL::ComPtrC<ID3D12DescriptorHeap> srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle,
	D3D12_VIEWPORT view_port)
{
	const auto return_view_port = view_port;

	D3D12_RECT scissor_rect;
	scissor_rect.left = scissor_rect.top = 0u;
	scissor_rect.right = SCAST<LONG>(view_port.Width);
	scissor_rect.bottom = SCAST<LONG>(view_port.Height);
	const auto return_scissor_rect = scissor_rect;

	const auto& rtrb = rtvs->GetRtvResourceBarriers(true);
	cmd_list->ResourceBarrier(SCAST<UINT>(rtrb.size()), rtrb.data());

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	bloom_renderer->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, srv_heap.GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(0, srv_gpu_handle);

	UINT32 rtv_num = 0u;
	{
		UINT32* mapped_rtv_num = rtv_num_res->Map(0, &CD3DX12_RANGE(0, 0));
		rtv_num = *mapped_rtv_num;
		rtv_num_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
	for (UINT32 idx = 0u; idx < rtv_num; idx++)
	{
		rtvs->Set(cmd_list, nullptr, idx);
		rtvs->Clear(cmd_list, rtv_textures[idx]->GetClearColor(), idx);
		const auto& desc = rtv_textures[idx]->Data()->GetDesc();

		view_port.Width = SCAST<FLOAT>(desc.Width);
		view_port.Height = SCAST<FLOAT>(desc.Height);
		scissor_rect.right = SCAST<LONG>(desc.Width);
		scissor_rect.bottom = SCAST<LONG>(desc.Height);

		cmd_list->RSSetViewports(1, &view_port);
		cmd_list->RSSetScissorRects(1, &scissor_rect);

		sprite->Render(cmd_list);
	}
	const auto& srvrb = rtvs->GetRtvResourceBarriers(false);
	cmd_list->ResourceBarrier(SCAST<UINT>(srvrb.size()), srvrb.data());

	cmd_list->RSSetViewports(1, &return_view_port);
	cmd_list->RSSetScissorRects(1, &return_scissor_rect);
}

void DOFGenerator::Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
	KGL::ComPtrC<ID3D12DescriptorHeap> depth_heap, D3D12_GPU_DESCRIPTOR_HANDLE depth_srv_handle)
{

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	renderer->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, depth_heap.GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(0, depth_srv_handle);

	cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(1, rtvs->GetSRVGPUHandle(0));

	cmd_list->SetDescriptorHeaps(1, rtv_num_handle.Heap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(2, rtv_num_handle.Gpu());

	sprite->Render(cmd_list);
}

void DOFGenerator::SetRtvNum(UINT8 num)
{
	UINT32* rtv_num = rtv_num_res->Map(0, &CD3DX12_RANGE(0, 0));
	*rtv_num = (std::min)(KGL::SCAST<UINT32>(rtv_textures.size()), KGL::SCAST<UINT32>(num));
	rtv_num_res->Unmap(0, &CD3DX12_RANGE(0, 0));
}

UINT8 DOFGenerator::GetRtvNum()
{
	UINT8 result;
	UINT32* rtv_num = rtv_num_res->Map(0, &CD3DX12_RANGE(0, 0));
	result = KGL::SCAST<UINT8>(*rtv_num);
	rtv_num_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	return result;
}