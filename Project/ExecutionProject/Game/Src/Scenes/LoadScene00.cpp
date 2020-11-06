#include "../../Hrd/Scenes/LoadScene00.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>

HRESULT LoadScene00Base::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	cmd_allocator->SetName(L"Main CA");
	cmd_list->SetName(L"Main CL");
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);

	sprite = std::make_shared<KGL::_2D::Sprite>(device);
	cbv_srv_descriptor = std::make_shared<KGL::DescriptorManager>(device, 2u);

	// フレームバッファ
	frame_buffer_handle = std::make_shared<KGL::DescriptorHandle>(cbv_srv_descriptor->Alloc());
	frame_buffer_resource = std::make_shared<KGL::Resource<FrameBuffer>>(device, 1u);
	frame_buffer_resource->CreateCBV(frame_buffer_handle);

	// ノイズテクスチャ
	noise_texture = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Noise/noise-01.png");
	noise_srv_handle = std::make_shared<KGL::DescriptorHandle>(cbv_srv_descriptor->Alloc());
	noise_texture->CreateSRVHandle(noise_srv_handle);

	{	// ノイズ スプライト レンダラー
		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		// renderer_desc.static_samplers[0].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/NoiseEffect_ps.hlsl";
		renderer_desc.root_params.clear();

		std::vector<D3D12_DESCRIPTOR_RANGE> ranges(2);
		{
			ranges[0] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1u, 0u);
			ranges[1] = KGL::_2D::Renderer::DESCRIPTOR_RANGES0[0];
			
			CD3DX12_ROOT_PARAMETER root_pram = {};
			root_pram.InitAsDescriptorTable(SCAST<UINT>(ranges.size()), ranges.data(), D3D12_SHADER_VISIBILITY_PIXEL);
			renderer_desc.root_params.push_back(root_pram);
		}
		noise_anim_renderer = std::make_shared<KGL::BaseRenderer>(device, desc.dxc, renderer_desc);
	}

	return hr;
}

HRESULT LoadScene00Base::Init(const SceneDesc& desc)
{

	clear_color = { 1.f, 1.f, 1.f, 1.f };

	SetLoadScene(desc);

	{	// フレームバッファを初期化
		auto* mapped_fresource = frame_buffer_resource->Map(0, &CD3DX12_RANGE(0u, 0u));
		mapped_fresource->time = 0.f;
		auto resolution = desc.app->GetResolution();
		mapped_fresource->resolution.x = 1.f / SCAST<float>(resolution.x);
		mapped_fresource->resolution.y = 1.f / SCAST<float>(resolution.y);
		frame_buffer_resource->Unmap(0, &CD3DX12_RANGE(0u, 0u));
	}

	return S_OK;
}

HRESULT LoadScene00Base::Update(const SceneDesc& desc, float elapsed_time)
{
	{	// フレームバッファを更新
		auto* mapped_fresource = frame_buffer_resource->Map(0, &CD3DX12_RANGE(0u, 0u));
		mapped_fresource->time += elapsed_time / 10;
		frame_buffer_resource->Unmap(0, &CD3DX12_RANGE(0u, 0u));
	}

	return Render(desc);
}

HRESULT LoadScene00Base::Render(const SceneDesc& desc)
{
	using KGL::SCAST;
	HRESULT hr = S_OK;

	auto window_size = desc.window->GetClientSize();

	D3D12_VIEWPORT viewport = {};
	viewport.Width = SCAST<FLOAT>(window_size.x);
	viewport.Height = SCAST<FLOAT>(window_size.y);
	viewport.TopLeftX = 0;//出力先の左上座標X
	viewport.TopLeftY = 0;//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;//深度最大値
	viewport.MinDepth = 0.0f;//深度最小値

	auto scissorrect = CD3DX12_RECT(
		0, 0,
		window_size.x, window_size.y
	);

	{
		cmd_list->RSSetViewports(1u, &viewport);
		cmd_list->RSSetScissorRects(1u, &scissorrect);

		desc.app->SetRtvDsv(cmd_list);
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtvDsv(cmd_list, clear_color);

		// ノイズ　テクスチャを描画
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		noise_anim_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1u, cbv_srv_descriptor->Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0u, cbv_srv_descriptor->Heap()->GetGPUDescriptorHandleForHeapStart());
		sprite->Render(cmd_list);

		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));
	}

	cmd_list->Close();
	ID3D12CommandList* cmd_lists[] = { cmd_list.Get() };
	desc.app->GetQueue()->Data()->ExecuteCommandLists(1, cmd_lists);
	desc.app->GetQueue()->Signal();
	desc.app->GetQueue()->Wait();

	cmd_allocator->Reset();
	cmd_list->Reset(cmd_allocator.Get(), nullptr);

	if (desc.app->IsTearingSupport())
		desc.app->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	else
		desc.app->GetSwapchain()->Present(1, 1);

	return hr;
}

HRESULT LoadScene00Base::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	return S_OK;
}