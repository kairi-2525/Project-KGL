#include "../../Hrd/Scenes/TestScene04.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>

HRESULT TestScene04::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	hr = KGL::HELPER::CreateCommandAllocatorAndList(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);


	sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device);
	sprite = std::make_shared<KGL::Sprite>(device);

	hr = scene_buffer.Load(desc);
	RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);

	{
		constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
		rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
			device, desc.app->GetRtvBuffers().at(0), clear_value));
		rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
			device, desc.app->GetRtvBuffers().at(0), clear_value));
		rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
			device, desc.app->GetRtvBuffers().at(0), clear_value));
		const auto& resources = KGL::TEXTURE::GetResources(rtv_textures);
		rtvs = std::make_shared<KGL::RenderTargetView>(device, resources);
	}

	particle_resource = std::make_shared<KGL::Resource<Particle>>(device, 500000u);
	particle_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, particle_resource->Size());
	particle_pipeline = std::make_shared<KGL::ComputePipline>(device);

	return hr;
}

HRESULT TestScene04::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();

	camera.eye = { 0.f, 10.f, -15.f };
	camera.focus_vec = { 0.f, 0.f, 15.f };
	camera.up = { 0.f, 1.f, 0.f };

	XMStoreFloat3(&scene_buffer.mapped_data->light_vector, XMVector3Normalize(XMVectorSet(+0.2f, -0.7f, 0.5f, 0.f)));

	const XMMATRIX proj_mat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(window_size.x) / static_cast<float>(window_size.y),	// アスペクト比
		1.0f, 100.0f // near, far
	);

	scene_buffer.mapped_data->proj = proj_mat;

	return S_OK;
}

HRESULT TestScene04::Update(const SceneDesc& desc, float elapsed_time)
{
	using namespace DirectX;

	scene_buffer.mapped_data->eye = camera.eye;
	scene_buffer.mapped_data->view = KGL::CAMERA::GetView(camera);

	return Render(desc);
}

HRESULT TestScene04::Render(const SceneDesc& desc)
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

	cmd_list->RSSetViewports(1, &viewport);
	cmd_list->RSSetScissorRects(1, &scissorrect);
	constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	{
		const auto& rbs_rt = rtvs->GetRtvResourceBarriers(true);
		const size_t rtv_size = rbs_rt.size();
		cmd_list->ResourceBarrier(rtv_size, rbs_rt.data());
		rtvs->SetAll(cmd_list, &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart());
		for (size_t i = 0u; i < rtv_size; i++) rtvs->Clear(cmd_list, clear_value, i);
		desc.app->ClearDsv(cmd_list);

		const auto& rbs_sr = rtvs->GetRtvResourceBarriers(false);
		cmd_list->ResourceBarrier(rtv_size, rbs_sr.data());
	}
	{
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->SetRtv(cmd_list);

		desc.app->ClearRtv(cmd_list, clear_value);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		sprite_renderer->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(0));

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

HRESULT TestScene04::UnInit(const SceneDesc& desc)
{
	return S_OK;
}