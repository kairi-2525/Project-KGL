#include "../Hrd/Bloom.hpp"

BloomGenerator::BloomGenerator(KGL::ComPtrC<ID3D12Device> device, KGL::ComPtrC<ID3D12Resource> rsc)
{
	constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	rtv_texture = std::make_shared<KGL::Texture>(
		device, rsc, clear_value);
	std::vector<KGL::ComPtr<ID3D12Resource>> resources(1u);
	resources[0] = rtv_texture->Data();
	rtvs = std::make_shared<KGL::RenderTargetView>(device, resources);

	sprite = std::make_shared<KGL::Sprite>(device);

	auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
	renderer_desc.ps_desc.hlsl = "./HLSL/2D/Bloom_ps.hlsl";
	renderer_desc.blend_types[0] = KGL::BDTYPE::ADD;
	renderer = std::make_shared<KGL::_2D::Renderer>(device, renderer_desc);

	renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
	renderer_desc.ps_desc.entry_point = "Generate";
	bloom_renderer = std::make_shared<KGL::_2D::Renderer>(device, renderer_desc);
}

void BloomGenerator::Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
	KGL::ComPtrC<ID3D12DescriptorHeap> srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle,
	D3D12_VIEWPORT view_port)
{
	const auto return_view_port = view_port;

	D3D12_RECT scissor_rect;
	scissor_rect.left = scissor_rect.top = 0u;
	scissor_rect.right = view_port.Width;
	scissor_rect.bottom = view_port.Height;
	const auto return_scissor_rect = scissor_rect;

	const UINT rtv_num = 0u;
	cmd_list->ResourceBarrier(1, &rtvs->GetRtvResourceBarrier(true, rtv_num));
	rtvs->Set(cmd_list, nullptr, rtv_num);
	rtvs->Clear(cmd_list, rtv_texture->GetClearColor(), rtv_num);

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	bloom_renderer->SetState(cmd_list);

	for (int i = 0; i < 8; i++)
	{
		view_port.Width /= 2;
		view_port.Height /= 2;
		scissor_rect.right = view_port.Width;
		scissor_rect.bottom = scissor_rect.top + view_port.Height;

		cmd_list->RSSetViewports(1, &view_port);
		cmd_list->RSSetScissorRects(1, &scissor_rect);

		cmd_list->SetDescriptorHeaps(1, srv_heap.GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, srv_gpu_handle);

		sprite->Render(cmd_list);
		view_port.TopLeftY += view_port.Height;
		scissor_rect.top = view_port.TopLeftY;
	}

	cmd_list->ResourceBarrier(1, &rtvs->GetRtvResourceBarrier(false, rtv_num));

	cmd_list->RSSetViewports(1, &return_view_port);
	cmd_list->RSSetScissorRects(1, &return_scissor_rect);
}

void BloomGenerator::Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list)
{

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	renderer->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle());
	sprite->Render(cmd_list);
}