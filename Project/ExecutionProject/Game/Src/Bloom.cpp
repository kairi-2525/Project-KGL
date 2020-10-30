#include "../Hrd/Bloom.hpp"
#include "../Hrd/MSAA.hpp"
#include <DirectXTex/d3dx12.h>
#include <Math/Gaussian.hpp>

BloomGenerator::BloomGenerator(KGL::ComPtrC<ID3D12Device> device,
	const std::shared_ptr<KGL::DXC>& dxc, KGL::ComPtrC<ID3D12Resource> rsc)
{
	constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	auto desc = rsc->GetDesc();
	const auto dx_clear_value = CD3DX12_CLEAR_VALUE(desc.Format, (float*)&clear_value);

	const UINT64 width = desc.Width;
	const UINT height = desc.Height;
	desc.SampleDesc.Count = 1;

	{
		std::vector<KGL::ComPtr<ID3D12Resource>> resources;
		resources.reserve(1);
		bloom_texture = std::make_shared<KGL::Texture>(
			device, desc, dx_clear_value);
		bloom_texture->Data()->SetName(L"bloom texture");
		resources.emplace_back(bloom_texture->Data());
		// Bloom用レンダーターゲットを作成
		bloom_rtv = std::make_shared<KGL::RenderTargetView>(
			device, resources, nullptr, D3D12_SRV_DIMENSION_TEXTURE2D
			);
	}
	{
		std::vector<KGL::ComPtr<ID3D12Resource>> resources_c, resources_w, resources_h;
		resources_c.reserve(RTV_MAX);
		resources_w.reserve(RTV_MAX);
		resources_h.reserve(RTV_MAX);
		for (UINT8 i = 0u; i < RTV_MAX; i++)
		{
			desc.Width = std::max<UINT64>(1u, desc.Width / 2);
			desc.Height = std::max<UINT>(1u, desc.Height / 2);;

			compression_rtvr.textures[i] = std::make_shared<KGL::Texture>(
				device, desc, dx_clear_value);
			resources_c.emplace_back(compression_rtvr.textures[i]->Data());

			gaussian_rtvr_w.textures[i] = std::make_shared<KGL::Texture>(
				device, desc, dx_clear_value);
			resources_w.emplace_back(gaussian_rtvr_w.textures[i]->Data());

			gaussian_rtvr_h.textures[i] = std::make_shared<KGL::Texture>(
				device, desc, dx_clear_value);
			resources_h.emplace_back(gaussian_rtvr_h.textures[i]->Data());
		}
		// 圧縮用レンダーターゲットを作成
		compression_rtvr.rtvs = std::make_shared<KGL::RenderTargetView>(
			device, resources_c, nullptr, D3D12_SRV_DIMENSION_TEXTURE2D
			);
		// ガウス用レンダーターゲットWを作成
		gaussian_rtvr_w.rtvs = std::make_shared<KGL::RenderTargetView>(
				device, resources_w, nullptr, D3D12_SRV_DIMENSION_TEXTURE2D
			);

		// ガウス用レンダーターゲットHを作成
		gaussian_rtvr_h.rtvs = std::make_shared<KGL::RenderTargetView>(
			device, resources_h, nullptr, D3D12_SRV_DIMENSION_TEXTURE2D
			);
	}

	sprite = std::make_shared<KGL::Sprite>(device);

	auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
	auto& sampler = renderer_desc.static_samplers[0];
	sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	renderer_desc.blend_types[0] = KGL::BDTYPE::ADD;
	bloom_renderer = std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc);

	renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
	compression_renderer = std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc);

	renderer_desc.ps_desc.hlsl = "./HLSL/2D/Bloom_ps.hlsl";

	auto root_param0 = CD3DX12_ROOT_PARAMETER();
	auto range0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 0);
	root_param0.InitAsDescriptorTable(1u, &range0, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params[0] = root_param0;
	auto root_param1 = CD3DX12_ROOT_PARAMETER();
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
	ranges.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0));
	ranges.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1));
	root_param1.InitAsDescriptorTable(SCAST<UINT>(ranges.size()), ranges.data(), D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params.push_back(root_param1);

	renderer_desc.ps_desc.entry_point = "PSMainW";
	gaussian_renderer_w = std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc);
	renderer_desc.ps_desc.entry_point = "PSMainH";
	gaussian_renderer_h = std::make_shared<KGL::BaseRenderer>(device, dxc, renderer_desc);

	frame_buffer = std::make_shared<KGL::Resource<Buffer>>(device, 1u);
	SetKernel(KGL::SCAST<UINT8>(RTV_MAX));
	{
		Weights weight;
		weight[0] = 1.f;
		for (UINT i = 1; i < RTV_MAX; i++)
			weight[i] = weight[i - 1u] * 1.f;
		SetWeights(weight);
	}

	constant_buffer_dsmgr = std::make_shared<KGL::DescriptorManager>(device, 2u);
	frame_buffer_handle = constant_buffer_dsmgr->Alloc();
	gausian_buffer_handle = constant_buffer_dsmgr->Alloc();

	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = frame_buffer->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = SCAST<UINT>(frame_buffer->SizeInBytes());
		device->CreateConstantBufferView(&cbv_desc, frame_buffer_handle.Cpu());
	}
	// ガウス処理をあらかじめ計算しておく
	SetGaussianKernel(8u);
	const auto& weights = KGL::MATH::GetGaussianWeights(8u, 5.f);
	gaussian_buffer = std::make_shared<KGL::Resource<float>>(device, weights.size());
	{
		auto* mapped_weight = gaussian_buffer->Map(0, &CD3DX12_RANGE(0, 0));
		std::copy(weights.cbegin(), weights.cend(), mapped_weight);
		gaussian_buffer->Unmap(0, &CD3DX12_RANGE(0, 0));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = gaussian_buffer->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = SCAST<UINT>(gaussian_buffer->SizeInBytes());
		device->CreateConstantBufferView(&cbv_desc, gausian_buffer_handle.Cpu());
	}
}

void BloomGenerator::Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
	KGL::ComPtrC<ID3D12DescriptorHeap> srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle,
	D3D12_VIEWPORT view_port)
{
	const auto return_view_port = view_port;

	D3D12_RECT scissor_rect;
	scissor_rect.left = scissor_rect.top = 0u;
	scissor_rect.right = SCAST<LONG>(view_port.Width);
	scissor_rect.bottom = SCAST<LONG>(view_port.Height);
	const auto return_scissor_rect = scissor_rect;
	
	const auto& rtvs_c = compression_rtvr.rtvs;
	const auto& textures_c = compression_rtvr.textures;
	const auto& rtvs_w = gaussian_rtvr_w.rtvs;
	const auto& textures_w = gaussian_rtvr_w.textures;
	const auto& rtvs_h = gaussian_rtvr_h.rtvs;
	const auto& textures_h = gaussian_rtvr_h.textures;
	{
		const auto& rtrb = rtvs_c->GetRtvResourceBarriers(true);
		cmd_list->ResourceBarrier(SCAST<UINT>(rtrb.size()), rtrb.data());
	}
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	compression_renderer->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, srv_heap.GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(0, srv_gpu_handle);

	UINT32 rtv_num = 0u;
	{
		Buffer* mapped_buffer = frame_buffer->Map(0, &CD3DX12_RANGE(0, 0));
		rtv_num = mapped_buffer->kernel;
		frame_buffer->Unmap(0, &CD3DX12_RANGE(0, 0));
	}

	// 圧縮テクスチャを作成
	for (UINT32 idx = 0u; idx < rtv_num; idx++)
	{
		rtvs_c->Set(cmd_list, nullptr, idx);
		rtvs_c->Clear(cmd_list, textures_c[idx]->GetClearColor(), idx);
		const auto& desc = textures_c[idx]->Data()->GetDesc();

		view_port.Width = SCAST<FLOAT>(desc.Width);
		view_port.Height = SCAST<FLOAT>(desc.Height);
		scissor_rect.right = SCAST<LONG>(desc.Width);
		scissor_rect.bottom = SCAST<LONG>(desc.Height);

		cmd_list->RSSetViewports(1, &view_port);
		cmd_list->RSSetScissorRects(1, &scissor_rect);

		sprite->Render(cmd_list);
	}
	{
		const auto& srvrb = rtvs_c->GetRtvResourceBarriers(false);
		cmd_list->ResourceBarrier(SCAST<UINT>(srvrb.size()), srvrb.data());
	}
	// Wブラーを描画
	gaussian_renderer_w->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, frame_buffer_handle.Heap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(1, frame_buffer_handle.Gpu());
	{
		const auto& rtrb = rtvs_w->GetRtvResourceBarriers(true);
		cmd_list->ResourceBarrier(SCAST<UINT>(rtrb.size()), rtrb.data());
	}
	for (UINT32 idx = 0u; idx < rtv_num; idx++)
	{
		rtvs_w->Set(cmd_list, nullptr, idx);
		rtvs_w->Clear(cmd_list, textures_w[idx]->GetClearColor(), idx);
		const auto& desc = textures_w[idx]->Data()->GetDesc();

		view_port.Width = SCAST<FLOAT>(desc.Width);
		view_port.Height = SCAST<FLOAT>(desc.Height);
		scissor_rect.right = SCAST<LONG>(desc.Width);
		scissor_rect.bottom = SCAST<LONG>(desc.Height);

		cmd_list->RSSetViewports(1, &view_port);
		cmd_list->RSSetScissorRects(1, &scissor_rect);

		cmd_list->SetDescriptorHeaps(1, rtvs_c->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs_c->GetSRVGPUHandle(idx));

		sprite->Render(cmd_list);
	}
	{
		const auto& srvrb = rtvs_w->GetRtvResourceBarriers(false);
		cmd_list->ResourceBarrier(SCAST<UINT>(srvrb.size()), srvrb.data());
	}
	// Hブラーを描画
	gaussian_renderer_h->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, frame_buffer_handle.Heap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(1, frame_buffer_handle.Gpu());
	{
		const auto& rtrb = rtvs_h->GetRtvResourceBarriers(true);
		cmd_list->ResourceBarrier(SCAST<UINT>(rtrb.size()), rtrb.data());
	}
	for (UINT32 idx = 0u; idx < rtv_num; idx++)
	{
		rtvs_h->Set(cmd_list, nullptr, idx);
		rtvs_h->Clear(cmd_list, textures_w[idx]->GetClearColor(), idx);
		const auto& desc = textures_w[idx]->Data()->GetDesc();

		view_port.Width = SCAST<FLOAT>(desc.Width);
		view_port.Height = SCAST<FLOAT>(desc.Height);
		scissor_rect.right = SCAST<LONG>(desc.Width);
		scissor_rect.bottom = SCAST<LONG>(desc.Height);

		cmd_list->RSSetViewports(1, &view_port);
		cmd_list->RSSetScissorRects(1, &scissor_rect);

		cmd_list->SetDescriptorHeaps(1, rtvs_w->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs_w->GetSRVGPUHandle(idx));

		sprite->Render(cmd_list);
	}
	{
		const auto& srvrb = rtvs_h->GetRtvResourceBarriers(false);
		cmd_list->ResourceBarrier(SCAST<UINT>(srvrb.size()), srvrb.data());
	}

	cmd_list->RSSetViewports(1, &return_view_port);
	cmd_list->RSSetScissorRects(1, &return_scissor_rect);

	// ブラーのかかったテクスチャを拡大しながら加算合成
	{
		const auto& rtrb = bloom_rtv->GetRtvResourceBarriers(true);
		cmd_list->ResourceBarrier(SCAST<UINT>(rtrb.size()), rtrb.data());
	}
	bloom_renderer->SetState(cmd_list);
	bloom_rtv->Set(cmd_list, nullptr);
	bloom_rtv->Clear(cmd_list, bloom_texture->GetClearColor());
	for (UINT32 idx = 0u; idx < rtv_num; idx++)
	{
		cmd_list->SetDescriptorHeaps(1, rtvs_h->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs_h->GetSRVGPUHandle(idx));

		sprite->Render(cmd_list);
	}
	{
		const auto& srvrb = bloom_rtv->GetRtvResourceBarriers(false);
		cmd_list->ResourceBarrier(SCAST<UINT>(srvrb.size()), srvrb.data());
	}
}

void BloomGenerator::Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list)
{
	const auto& rtvs = bloom_rtv;
	bloom_renderer->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(0));

	sprite->Render(cmd_list);
}

void BloomGenerator::SetKernel(UINT8 num) noexcept
{
	Buffer* mapped_buffer = frame_buffer->Map(0, &CD3DX12_RANGE(0, 0));
	mapped_buffer->kernel = (std::min)(KGL::SCAST<UINT32>(RTV_MAX), KGL::SCAST<UINT32>(num));
	frame_buffer->Unmap(0, &CD3DX12_RANGE(0, 0));
}

UINT8 BloomGenerator::GetKernel() const noexcept
{
	UINT8 result;
	Buffer* mapped_buffer = frame_buffer->Map(0, &CD3DX12_RANGE(0, 0));
	result = KGL::SCAST<UINT8>(mapped_buffer->kernel);
	frame_buffer->Unmap(0, &CD3DX12_RANGE(0, 0));
	return result;
}

void BloomGenerator::SetGaussianKernel(UINT8 num) noexcept
{
	Buffer* mapped_buffer = frame_buffer->Map(0, &CD3DX12_RANGE(0, 0));
	mapped_buffer->gaussian_kernel = (std::min)(KGL::SCAST<UINT32>(RTV_MAX), KGL::SCAST<UINT32>(num));
	frame_buffer->Unmap(0, &CD3DX12_RANGE(0, 0));
}

UINT8 BloomGenerator::GetGaussianKernel() const noexcept
{
	UINT8 result;
	Buffer* mapped_buffer = frame_buffer->Map(0, &CD3DX12_RANGE(0, 0));
	result = KGL::SCAST<UINT8>(mapped_buffer->gaussian_kernel);
	frame_buffer->Unmap(0, &CD3DX12_RANGE(0, 0));
	return result;
}

void BloomGenerator::SetWeights(Weights weights) noexcept
{
	Buffer* mapped_buffer = frame_buffer->Map(0, &CD3DX12_RANGE(0, 0));
	mapped_buffer->weight = weights;
	frame_buffer->Unmap(0, &CD3DX12_RANGE(0, 0));
}

BloomGenerator::Weights BloomGenerator::GetWeights() const noexcept
{
	Weights result;
	Buffer* mapped_buffer = frame_buffer->Map(0, &CD3DX12_RANGE(0, 0));
	result = mapped_buffer->weight;
	frame_buffer->Unmap(0, &CD3DX12_RANGE(0, 0));
	return result;
}