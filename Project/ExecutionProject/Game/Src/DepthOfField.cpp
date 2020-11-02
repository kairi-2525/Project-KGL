#include "../Hrd/DepthOfField.hpp"
#include <DirectXTex/d3dx12.h>
#include <Math/Gaussian.hpp>

DOFGenerator::DOFGenerator(KGL::ComPtrC<ID3D12Device> device,
	const std::shared_ptr<KGL::DXC>& dxc, KGL::ComPtrC<ID3D12Resource> rsc)
{
	constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	auto desc = rsc->GetDesc();
	const auto dx_clear_value = CD3DX12_CLEAR_VALUE(desc.Format, (float*)&clear_value);

	// 初期化画像をコピーするテクスチャ
	before_texture = std::make_shared<KGL::Texture>(device, desc, dx_clear_value);

	std::vector<KGL::ComPtr<ID3D12Resource>> resources_c, resources_w, resources_h;
	resources_c.reserve(RTV_MAX);
	resources_w.reserve(RTV_MAX);
	resources_h.reserve(RTV_MAX);

	for (UINT8 i = 0u; i < RTV_MAX; i++)
	{
		compression_rtvr.textures[i] = std::make_shared<KGL::Texture>(
			device, desc, dx_clear_value);

		resources_c.emplace_back(compression_rtvr.textures[i]->Data());

		gaussian_rtvr_w.textures[i] = std::make_shared<KGL::Texture>(
			device, desc, dx_clear_value);

		resources_w.emplace_back(gaussian_rtvr_w.textures[i]->Data());

		gaussian_rtvr_h.textures[i] = std::make_shared<KGL::Texture>(
			device, desc, dx_clear_value);

		resources_h.emplace_back(gaussian_rtvr_h.textures[i]->Data());

		desc.Width /= 2;
		desc.Height /= 2;
	}

	compression_rtvr.rtvs = std::make_shared<KGL::RenderTargetView>(device, resources_c);
	gaussian_rtvr_w.rtvs = std::make_shared<KGL::RenderTargetView>(device, resources_w);
	gaussian_rtvr_h.rtvs = std::make_shared<KGL::RenderTargetView>(device, resources_h);

	sprite = std::make_shared<KGL::Sprite>(device);

	// 圧縮用レンダラーを作成
	auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
	renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
	auto& sampler = renderer_desc.static_samplers[0];
	sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	compression_renderer = std::make_shared<KGL::_2D::Renderer>(device, dxc, renderer_desc);

	// W , H のガウスブラー用レンダラーを作成
	renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
	renderer_desc.root_params.clear();
	renderer_desc.root_params.reserve(3u);
	auto root_param0 = CD3DX12_ROOT_PARAMETER();
	auto range0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	root_param0.InitAsDescriptorTable(1u, &range0, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params.push_back(root_param0);
	auto root_param1 = CD3DX12_ROOT_PARAMETER();
	auto range1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	root_param1.InitAsDescriptorTable(1u, &range1, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params.push_back(root_param1);

	renderer_desc.ps_desc.hlsl = "./HLSL/2D/GaussianBlurW_ps.hlsl";
	gaussian_renderer_w = std::make_shared<KGL::_2D::Renderer>(device, dxc, renderer_desc);
	renderer_desc.ps_desc.hlsl = "./HLSL/2D/GaussianBlurH_ps.hlsl";
	gaussian_renderer_h = std::make_shared<KGL::_2D::Renderer>(device, dxc, renderer_desc);

	// 被写界深度用レンダラーを作成
	auto root_param2 = CD3DX12_ROOT_PARAMETER();
	auto range2 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	root_param2.InitAsDescriptorTable(1u, &range2, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params.push_back(root_param2);
	auto root_param3 = CD3DX12_ROOT_PARAMETER();
	auto range3 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 2);
	root_param3.InitAsDescriptorTable(1u, &range3, D3D12_SHADER_VISIBILITY_PIXEL);
	renderer_desc.root_params.push_back(root_param3);
	renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;

	renderer_desc.ps_desc.hlsl = "./HLSL/2D/DepthOfField_ps.hlsl";
	dof_renderer = std::make_shared<KGL::_2D::Renderer>(device, dxc, renderer_desc);

	// 最大値をデフォルトでセット
	rtv_num_res = std::make_shared<KGL::Resource<UINT32>>(device, 1u);
	SetRtvNum(KGL::SCAST<UINT8>(RTV_MAX));

	srv_cbv_dsmgr = std::make_shared<KGL::DescriptorManager>(device, 3u);
	rtv_num_handle = srv_cbv_dsmgr->Alloc();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = rtv_num_res->Data()->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = SCAST<UINT>(rtv_num_res->SizeInBytes());
	device->CreateConstantBufferView(&cbv_desc, rtv_num_handle.Cpu());

	gausian_buffer_handle = srv_cbv_dsmgr->Alloc();

	{
		// ガウス処理をあらかじめ計算しておく
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

	before_tex_srv_handle = std::make_shared<KGL::DescriptorHandle>(srv_cbv_dsmgr->Alloc());
	before_texture->CreateSRVHandle(before_tex_srv_handle);
}

void DOFGenerator::Generate(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
	std::shared_ptr<KGL::Texture> texture,
	KGL::ComPtrC<ID3D12DescriptorHeap> srv_heap, D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle,
	D3D12_VIEWPORT view_port)
{

	UINT32 rtv_num = 0u;
	{
		UINT32* mapped_rtv_num = rtv_num_res->Map(0, &CD3DX12_RANGE(0, 0));
		rtv_num = *mapped_rtv_num;
		rtv_num_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
	if (rtv_num > 0)
	{
		const auto return_view_port = view_port;

		D3D12_RECT scissor_rect;
		scissor_rect.left = scissor_rect.top = 0u;
		scissor_rect.right = SCAST<LONG>(view_port.Width);
		scissor_rect.bottom = SCAST<LONG>(view_port.Height);
		const auto return_scissor_rect = scissor_rect;

		{	// 初期テクスチャをコピー
			std::array<D3D12_RESOURCE_BARRIER, 2u> begin_barrier =
			{
				texture->RB(D3D12_RESOURCE_STATE_GENERIC_READ),
				before_texture->RB(D3D12_RESOURCE_STATE_COPY_DEST)
			};
			cmd_list->ResourceBarrier(SCAST<UINT>(begin_barrier.size()), begin_barrier.data());

			cmd_list->CopyResource(before_texture->Data().Get(), texture->Data().Get());

			std::array<D3D12_RESOURCE_BARRIER, 2u> end_barrier =
			{
				texture->RB(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				before_texture->RB(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			};
			cmd_list->ResourceBarrier(SCAST<UINT>(end_barrier.size()), end_barrier.data());
		}

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		{	// 圧縮
			const auto& rtvr = compression_rtvr;
			const auto& rtrb = rtvr.rtvs->GetRtvResourceBarriers(true);
			cmd_list->ResourceBarrier(SCAST<UINT>(rtrb.size()), rtrb.data());

			compression_renderer->SetState(cmd_list);

			cmd_list->SetDescriptorHeaps(1, srv_heap.GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(0, srv_gpu_handle);

			for (UINT32 idx = 0u; idx < rtv_num; idx++)
			{
				rtvr.rtvs->Set(cmd_list, nullptr, idx);
				rtvr.rtvs->Clear(cmd_list, rtvr.textures[idx]->GetClearColor(), idx);

				const auto& desc = rtvr.textures[idx]->Data()->GetDesc();

				view_port.Width = SCAST<FLOAT>(desc.Width);
				view_port.Height = SCAST<FLOAT>(desc.Height);
				scissor_rect.right = SCAST<LONG>(desc.Width);
				scissor_rect.bottom = SCAST<LONG>(desc.Height);

				cmd_list->RSSetViewports(1, &view_port);
				cmd_list->RSSetScissorRects(1, &scissor_rect);

				sprite->Render(cmd_list);
			}

			const auto& srvrb = rtvr.rtvs->GetRtvResourceBarriers(false);
			cmd_list->ResourceBarrier(SCAST<UINT>(srvrb.size()), srvrb.data());
		}
		{	// Wブラー
			const auto& rtvr = gaussian_rtvr_w;
			const auto& srv_rtvr = compression_rtvr;
			const auto& rtrb = rtvr.rtvs->GetRtvResourceBarriers(true);
			cmd_list->ResourceBarrier(SCAST<UINT>(rtrb.size()), rtrb.data());

			gaussian_renderer_w->SetState(cmd_list);

			cmd_list->SetDescriptorHeaps(1, gausian_buffer_handle.Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(1, gausian_buffer_handle.Gpu());

			cmd_list->SetDescriptorHeaps(1, srv_rtvr.rtvs->GetSRVHeap().GetAddressOf());
			for (UINT32 idx = 0u; idx < rtv_num; idx++)
			{
				rtvr.rtvs->Set(cmd_list, nullptr, idx);
				rtvr.rtvs->Clear(cmd_list, rtvr.textures[idx]->GetClearColor(), idx);

				const auto& desc = rtvr.textures[idx]->Data()->GetDesc();

				view_port.Width = SCAST<FLOAT>(desc.Width);
				view_port.Height = SCAST<FLOAT>(desc.Height);
				scissor_rect.right = SCAST<LONG>(desc.Width);
				scissor_rect.bottom = SCAST<LONG>(desc.Height);

				cmd_list->RSSetViewports(1, &view_port);
				cmd_list->RSSetScissorRects(1, &scissor_rect);

				cmd_list->SetGraphicsRootDescriptorTable(0, srv_rtvr.rtvs->GetSRVGPUHandle(idx));

				sprite->Render(cmd_list);
			}

			const auto& srvrb = rtvr.rtvs->GetRtvResourceBarriers(false);
			cmd_list->ResourceBarrier(SCAST<UINT>(srvrb.size()), srvrb.data());
		}
		{	// Wブラー
			const auto& rtvr = gaussian_rtvr_h;
			const auto& srv_rtvr = gaussian_rtvr_w;
			const auto& rtrb = rtvr.rtvs->GetRtvResourceBarriers(true);
			cmd_list->ResourceBarrier(SCAST<UINT>(rtrb.size()), rtrb.data());

			gaussian_renderer_h->SetState(cmd_list);

			cmd_list->SetDescriptorHeaps(1, gausian_buffer_handle.Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(1, gausian_buffer_handle.Gpu());

			cmd_list->SetDescriptorHeaps(1, srv_rtvr.rtvs->GetSRVHeap().GetAddressOf());
			for (UINT32 idx = 0u; idx < rtv_num; idx++)
			{
				rtvr.rtvs->Set(cmd_list, nullptr, idx);
				rtvr.rtvs->Clear(cmd_list, rtvr.textures[idx]->GetClearColor(), idx);

				const auto& desc = rtvr.textures[idx]->Data()->GetDesc();

				view_port.Width = SCAST<FLOAT>(desc.Width);
				view_port.Height = SCAST<FLOAT>(desc.Height);
				scissor_rect.right = SCAST<LONG>(desc.Width);
				scissor_rect.bottom = SCAST<LONG>(desc.Height);

				cmd_list->RSSetViewports(1, &view_port);
				cmd_list->RSSetScissorRects(1, &scissor_rect);

				cmd_list->SetGraphicsRootDescriptorTable(0, srv_rtvr.rtvs->GetSRVGPUHandle(idx));

				sprite->Render(cmd_list);
			}

			const auto& srvrb = rtvr.rtvs->GetRtvResourceBarriers(false);
			cmd_list->ResourceBarrier(SCAST<UINT>(srvrb.size()), srvrb.data());
		}

		cmd_list->RSSetViewports(1, &return_view_port);
		cmd_list->RSSetScissorRects(1, &return_scissor_rect);
	}
}

void DOFGenerator::Render(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list,
	KGL::ComPtrC<ID3D12DescriptorHeap> depth_heap, D3D12_GPU_DESCRIPTOR_HANDLE depth_srv_handle)
{
	// 8個のブラー済みテクスチャをセットし被写界深度を描画する
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dof_renderer->SetState(cmd_list);

	cmd_list->SetDescriptorHeaps(1, depth_heap.GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(0, depth_srv_handle);

	cmd_list->SetDescriptorHeaps(1, srv_cbv_dsmgr->Heap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(1, rtv_num_handle.Gpu());
	cmd_list->SetGraphicsRootDescriptorTable(2, before_tex_srv_handle->Gpu());

	cmd_list->SetDescriptorHeaps(1, gaussian_rtvr_h.rtvs->GetSRVHeap().GetAddressOf());
	cmd_list->SetGraphicsRootDescriptorTable(3, gaussian_rtvr_h.rtvs->GetSRVGPUHandle(0));

	sprite->Render(cmd_list);
}

void DOFGenerator::SetRtvNum(UINT8 num)
{
	UINT32* rtv_num = rtv_num_res->Map(0, &CD3DX12_RANGE(0, 0));
	*rtv_num = (std::min)(KGL::SCAST<UINT32>(RTV_MAX), KGL::SCAST<UINT32>(num));
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