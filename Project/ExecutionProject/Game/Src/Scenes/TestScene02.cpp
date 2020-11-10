#include "../../Hrd/Scenes/Scenes.hpp"

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

	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);

	pmd_data = std::make_shared<KGL::PMD_Loader>("./Assets/Models/初音ミク.pmd");
	vmd_data = std::make_shared<KGL::VMD_Loader>("./Assets/Motions/motion.vmd");
	pmd_toon_model = std::make_shared<KGL::PMD_Model>(device, pmd_data->GetDesc(), "./Assets/Toons", &tex_mgr);
	pmd_model = std::make_shared<KGL::PMD_Model>(device, pmd_data->GetDesc(), &tex_mgr);

	{
		auto renderer_desc = KGL::_3D::PMD_Renderer::DEFAULT_DESC;
		auto add_ranges = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
		{
			CD3DX12_ROOT_PARAMETER root_param;
			root_param.InitAsDescriptorTable(1, &add_ranges);
			root_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// ピクセルシェーダーのみで使う
			renderer_desc.root_params.push_back(root_param);
		}
		{
			D3D12_STATIC_SAMPLER_DESC sampler_desc = 
				CD3DX12_STATIC_SAMPLER_DESC(
					2u,
					D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR, // 比較結果をバイリニア補完
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP
					);
			sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;	// <= であれば 1.0 そうでなければ 0.0
			sampler_desc.MaxAnisotropy = 1; // 深度傾斜を有効化
			sampler_desc.MinLOD = -D3D12_FLOAT32_MAX;
			sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


			renderer_desc.static_samplers.push_back(sampler_desc);
		}
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/PMDShadowMap_ps.hlsl";
		pmd_renderer = std::make_shared<KGL::PMD_Renderer>(device, desc.dxc, renderer_desc);
		pmd_renderer->SetName("pmd_renderer");
	}
	{
		auto renderer_desc = KGL::_3D::PMD_Renderer::DEFAULT_DESC;
		renderer_desc.vs_desc.hlsl = "./HLSL/3D/PMDShadowMap_vs.hlsl";
		renderer_desc.ps_desc.hlsl.clear();
		renderer_desc.render_targets.clear();
		pmd_light_renderer = std::make_shared<KGL::PMD_Renderer>(device, desc.dxc, renderer_desc);
		pmd_light_renderer->SetName("pmd_light_renderer");
	}

	sprite = std::make_shared<KGL::Sprite>(device);
	{
		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/Depth_ps.hlsl";
		depth_renderer = std::make_shared<KGL::_2D::Renderer>(device, desc.dxc, renderer_desc);
		depth_renderer->SetName("depth_renderer");
	}

	{
		D3D12_CLEAR_VALUE depth_clear_value = {};
		depth_clear_value.DepthStencil.Depth = 1.0f;		// 深さの最大値でクリア
		depth_clear_value.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;	// 32ビットfloat値としてクリア
		auto texture_desc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R32G8X24_TYPELESS,
			SHADOW_DIFINITION, SHADOW_DIFINITION
		);
		texture_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		light_dsv =
			std::make_shared<KGL::Texture>(device,
				texture_desc,
				depth_clear_value,
				D3D12_RESOURCE_STATE_DEPTH_WRITE
			);
	}
	{
		light_dsv_desc_mgr = std::make_shared<KGL::DescriptorManager>(
			device, 1u, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
		light_dsv_handle = light_dsv_desc_mgr->Alloc();
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;	// フラグ無し
		device->CreateDepthStencilView(light_dsv->Data().Get(), &dsv_desc, light_dsv_handle.Cpu());
	}
	dsv_srv_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	dsv_srv_handle = dsv_srv_desc_mgr->Alloc();
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC res_desc = {};
		res_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		res_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		res_desc.Texture2D.MipLevels = 1;
		res_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		device->CreateShaderResourceView(
			light_dsv->Data().Get(),
			&res_desc,
			dsv_srv_handle.Cpu()
		);
	}
	{
		//std::vector<KGL::ComPtr<ID3D12Resource>> resources(1);
		//resources[0] = light_depth_rt->Data();
		//rtv = std::make_shared<KGL::RenderTargetView>(device, resources);
	}

	models.resize(2, { device, *pmd_model });
	for (auto& model : models)
	{
		model.SetAnimation(vmd_data->GetDesc());
	}

	hr = scene_buffer.Load(desc);
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

	light_camera.up = { 0.f, 1.f, 0.f };
	XMStoreFloat3(&scene_buffer.mapped_data->light_vector, XMVector3Normalize(XMVectorSet(+0.2f, -0.7f, 0.5f, 0.f)));

	const XMMATRIX proj_mat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(window_size.x) / static_cast<float>(window_size.y),	// アスペクト比
		1.0f, 100.0f // near, far
	);

	scene_buffer.mapped_data->proj = proj_mat;

	clear_color = { 1.f, 1.f, 1.f, 1.f };

	return S_OK;
}

HRESULT TestScene02::Update(const SceneDesc& desc, float elapsed_time)
{
	auto input = desc.input;
	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<LoadScene00<TestScene01>>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<LoadScene00<TestScene03>>(desc);

	using namespace DirectX;

	scene_buffer.mapped_data->eye = camera.eye;
	scene_buffer.mapped_data->view = KGL::CAMERA::GetView(camera);

	light_camera.focus = KGL::CAMERA::GetFocusPos(camera);

	auto light_length = XMVector3Length(XMLoadFloat3(&camera.focus_vec));
	XMStoreFloat3(&light_camera.eye, 
		XMLoadFloat3(&light_camera.focus)
		- (XMLoadFloat3(&scene_buffer.mapped_data->light_vector) * light_length)
	);

	scene_buffer.mapped_data->light_cam = KGL::CAMERA::GetView(light_camera) * XMMatrixOrthographicLH(40.f, 40.f, 1.f, 100.f);

	for (auto& model : models)
	{
		//model.position.x -= elapsed_time * ((rand() % (20 + 1)) - 10);
		//model.position.z -= elapsed_time * ((rand() % (20 + 1)) - 10);
		model.rotation.y += XMConvertToRadians(90.f) * elapsed_time;
		model.MotionSetup(elapsed_time, true);
		model.MotionMatrixUpdate();
		model.Update(elapsed_time);
		model.UpdateWVP(scene_buffer.mapped_data->view * scene_buffer.mapped_data->proj);
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
		cmd_list->OMSetRenderTargets(0, nullptr, false, &light_dsv_handle.Cpu());
		cmd_list->ClearDepthStencilView(light_dsv_handle.Cpu(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		cmd_list->RSSetViewports(1, &CD3DX12_VIEWPORT(0.f, 0.f, SCAST<FLOAT>(SHADOW_DIFINITION), SCAST<FLOAT>(SHADOW_DIFINITION)));
		cmd_list->RSSetScissorRects(1, &CD3DX12_RECT(0, 0, SHADOW_DIFINITION, SHADOW_DIFINITION));

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		pmd_light_renderer->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, scene_buffer.handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, scene_buffer.handle.Gpu());

		for (auto& model : models)
		{
			model.Render(cmd_list);

			hr = pmd_toon_model->NonMaterialRender(
				desc.app->GetDevice(),
				cmd_list
			);
			RCHECK(FAILED(hr), "pmd_model->Renderに失敗", hr);
		}
	}
	{
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->SetRtvDsv(cmd_list);
		desc.app->ClearRtvDsv(cmd_list, clear_color);

		cmd_list->RSSetViewports(1, &viewport);
		cmd_list->RSSetScissorRects(1, &scissorrect);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		pmd_renderer->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, dsv_srv_handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(3, dsv_srv_handle.Gpu());

		for (auto& model : models)
		{
			model.Render(cmd_list);

			hr = pmd_toon_model->Render(
				desc.app->GetDevice(),
				cmd_list
			);
			RCHECK(FAILED(hr), "pmd_model->Renderに失敗", hr);
		}

		desc.app->SetRtv(cmd_list);

		auto viewport_l = viewport;
		viewport_l.Width /= 4; viewport_l.Height /= 4;
		auto scissorrect_l = scissorrect;
		scissorrect_l.right /= 4; scissorrect_l.bottom /= 4;
		cmd_list->RSSetViewports(1, &viewport_l);
		cmd_list->RSSetScissorRects(1, &scissorrect_l);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		depth_renderer->SetState(cmd_list);

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

HRESULT TestScene02::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	return S_OK;
}