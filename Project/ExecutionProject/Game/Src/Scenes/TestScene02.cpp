#include "../../Hrd/Scenes/TestScene02.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>

HRESULT TestScene02::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	hr = KGL::HELPER::CreateCommandAllocatorAndList(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);

	pmd_data = std::make_shared<KGL::PMD_Loader>("./Assets/Models/初音ミク.pmd");
	vmd_data = std::make_shared<KGL::VMD_Loader>("./Assets/Motions/motion.vmd");
	pmd_toon_model = std::make_shared<KGL::PMD_Model>(device, pmd_data->GetDesc(), "./Assets/Toons", &tex_mgr);
	pmd_model = std::make_shared<KGL::PMD_Model>(device, pmd_data->GetDesc(), &tex_mgr);

	{
		auto renderer_desc = KGL::PMD_Renderer::DEFAULT_DESC;
		renderer_desc.vs_desc.hlsl = "./HLSL/3D/PMDGroundShadow_vs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/PMDGroundShadow_ps.hlsl";
		pmd_renderer = std::make_shared<KGL::PMD_Renderer>(device, renderer_desc);
	}

	dsv_srv_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	dsv_srv_handle = dsv_srv_desc_mgr->Alloc();
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC res_desc = {};
		res_desc.Format = DXGI_FORMAT_R32_FLOAT;
		res_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		res_desc.Texture2D.MipLevels = 1;
		res_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		device->CreateShaderResourceView(
			desc.app->GetDsvBuffer().Get(),
			&res_desc,
			dsv_srv_handle.Cpu()
		);
	}

	sprite = std::make_shared<KGL::Sprite>(device);
	{
		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		auto add_ranges_dsv = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
		{
			CD3DX12_ROOT_PARAMETER root_param;
			root_param.InitAsDescriptorTable(1, &add_ranges_dsv);
			renderer_desc.add_root_param.push_back(root_param);
		}
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/Depth_ps.hlsl";
		sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device, renderer_desc);
	}

	tex_rt = std::make_shared<KGL::Texture>(device, desc.app->GetRtvBuffers().at(0), DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f));
	{
		std::vector<KGL::ComPtr<ID3D12Resource>> resources(1);
		resources[0] = tex_rt->Data();
		rtv = std::make_shared<KGL::RenderTargetView>(device, resources);
	}

	models.resize(1, { device, *pmd_model });
	for (auto& model : models)
	{
		model.SetAnimation(vmd_data->GetDesc());
	}

	hr = SceneBaseDx12::Load(desc);
	RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);

	return hr;
}

HRESULT TestScene02::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();

	camera.eye = { 0.f, 10.f, -15.f };
	camera.focus_vec = { 0.f, 0.f, 15.f };
	camera.up = { 0.f, 1.f, 0.f };

	const XMMATRIX proj_mat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(window_size.x) / static_cast<float>(window_size.y),	// アスペクト比
		1.0f, 100.0f // near, far
	);

	scene_mapped_buff->proj = proj_mat;

	clear_color = { 1.f, 1.f, 1.f, 1.f };

	return S_OK;
}

HRESULT TestScene02::Update(const SceneDesc& desc, float elapsed_time)
{
	using namespace DirectX;

	scene_mapped_buff->eye = camera.eye;
	scene_mapped_buff->view = KGL::CAMERA::GetView(camera);

	for (auto& model : models)
	{
		//model.position.x -= elapsed_time * ((rand() % (20 + 1)) - 10);
		//model.position.z -= elapsed_time * ((rand() % (20 + 1)) - 10);
		model.rotation.y += XMConvertToRadians(135.f) * elapsed_time;
		model.MotionUpdate(elapsed_time, true);
		model.Update(elapsed_time);
		model.UpdateWVP(scene_mapped_buff->view * scene_mapped_buff->proj);
	}

	return Render(desc);
}

HRESULT TestScene02::Render(const SceneDesc& desc)
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
		cmd_list->ResourceBarrier(1, &rtv->GetRtvResourceBarrier(true));
		rtv->Set(cmd_list, &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart());
		rtv->Clear(cmd_list, DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f));
		desc.app->ClearDsv(cmd_list);

		cmd_list->RSSetViewports(1, &viewport);
		cmd_list->RSSetScissorRects(1, &scissorrect);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		pmd_renderer->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, scene_buff_handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, scene_buff_handle.Gpu());

		for (auto& model : models)
		{
			model.Render(cmd_list);

			hr = pmd_toon_model->Render(
				desc.app->GetDevice(),
				cmd_list
			);
			RCHECK(FAILED(hr), "pmd_model->Renderに失敗", hr);
		}

		cmd_list->ResourceBarrier(1, &rtv->GetRtvResourceBarrier(false));
	}
	{	// モデルを描画したテクスチャをSwapchainのレンダーターゲットに描画(歪みNormal)
		desc.app->SetRtv(cmd_list);
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtv(cmd_list, clear_color);

		cmd_list->RSSetViewports(1, &viewport);
		cmd_list->RSSetScissorRects(1, &scissorrect);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		sprite_renderer->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, dsv_srv_handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, dsv_srv_handle.Gpu());

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

HRESULT TestScene02::UnInit(const SceneDesc& desc)
{
	return S_OK;
}