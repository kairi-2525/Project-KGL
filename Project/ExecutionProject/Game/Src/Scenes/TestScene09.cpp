#include "../../Hrd/Scenes/Scenes.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>
#include <random>
#include <Loader/OBJLoader.hpp>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

HRESULT TestScene09::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);

	descriptor = std::make_shared<KGL::DescriptorManager>(device, 2u);
	frame_buffer = std::make_shared<KGL::Resource<FrameBuffer>>(device, 1u);
	frame_buffer_handle = std::make_shared<KGL::DescriptorHandle>(descriptor->Alloc());
	frame_buffer->CreateCBV(frame_buffer_handle);

	cube_buffer = std::make_shared<KGL::Resource<CubeMapBuffer>>(device, 1u);
	cube_buffer_handle = std::make_shared<KGL::DescriptorHandle>(descriptor->Alloc());
	cube_buffer->CreateCBV(cube_buffer_handle);

	// モデルデータ読み込み
	{
		{
			auto inc_model = std::make_shared<KGL::StaticModel>(
				device, std::make_shared<KGL::OBJ_Loader>(
					"./Assets/Models/Mr.Incredible/Mr.Incredible.obj"
					));
			auto& actor = s_actors.emplace_back();
			inc_actor = actor = std::make_shared<KGL::StaticModelActor>(device, inc_model);
		}
		{
			auto slime_model = std::make_shared<KGL::StaticModel>(
				device, std::make_shared<KGL::OBJ_Loader>(
					"./Assets/Models/Slime/Slime.obj"
					));
			auto& actor = s_actors.emplace_back();
			slime_actor = actor = std::make_shared<KGL::StaticModelActor>(device, slime_model);
		}
		{
			auto sky_model = std::make_shared<KGL::StaticModel>(
				device, std::make_shared<KGL::OBJ_Loader>(
					"./Assets/Models/Sky/sky.obj"
					));
			auto& actor = s_actors.emplace_back();
			sky_actor = actor = std::make_shared<KGL::StaticModelActor>(device, sky_model);
		}
		{
			auto earth_model = std::make_shared<KGL::StaticModel>(
				device, std::make_shared<KGL::OBJ_Loader>(
					"./Assets/Models/earth/earth.obj"
					));
			auto& actor = s_actors.emplace_back();
			earth_actor = actor = std::make_shared<KGL::StaticModelActor>(device, earth_model);
		}
		{
			auto bison_model = std::make_shared<KGL::StaticModel>(
				device, std::make_shared<KGL::OBJ_Loader>(
					"./Assets/Models/Bison/Bison.obj"
					));
			auto& actor = s_actors.emplace_back();
			bison_actor = actor = std::make_shared<KGL::StaticModelActor>(device, bison_model);
		}
	}

	// モデル用レンダラー
	{
		auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;

		renderer_desc.vs_desc.hlsl = "./HLSL/3D/StaticModel_vs.hlsl";
		renderer_desc.gs_desc.hlsl = "./HLSL/3D/StaticModel_gs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/StaticModel_ps.hlsl";
		renderer_desc.rastarizer_desc.CullMode = D3D12_CULL_MODE_BACK;

		renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
		renderer_desc.input_layouts = KGL::StaticModel::INPUT_LAYOUTS;
		renderer_desc.root_params = KGL::StaticModel::ROOT_PARAMS;

		s_model_renderer = std::make_shared<KGL::BaseRenderer>(device, desc.dxc, renderer_desc);

		renderer_desc.ps_desc.hlsl = "./HLSL/3D/StaticModelDiffuse_ps.hlsl";
		sky_renderer = std::make_shared<KGL::BaseRenderer>(device, desc.dxc, renderer_desc);
	}
	// レンダーターゲット
	{
		constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
		constexpr auto clear_value2 = DirectX::XMFLOAT4(1.f, 1.f, 1.f, 1.f);
		D3D12_RESOURCE_DESC res_desc = desc.app->GetRtvBuffers().at(0)->GetDesc();

		rt_textures.push_back({ std::make_shared<KGL::Texture>(
		device, res_desc, CD3DX12_CLEAR_VALUE(res_desc.Format, (float*)&clear_value2)) });
		rt_textures.push_back({ std::make_shared<KGL::Texture>(
		device, res_desc, CD3DX12_CLEAR_VALUE(res_desc.Format, (float*)&clear_value)) });
		rt_textures.push_back({ std::make_shared<KGL::Texture>(
		device, res_desc, CD3DX12_CLEAR_VALUE(res_desc.Format, (float*)&clear_value)) });

		std::vector<ComPtr<ID3D12Resource>> resources;
		resources.reserve(rt_textures.size());
		for (auto& it : rt_textures) resources.push_back(it->Data());

		rtvs = std::make_shared<KGL::RenderTargetView>(device, resources);
	}

	sprite = std::make_shared<KGL::Sprite>(device);
	sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device, desc.dxc);
	{
		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/Brighten_ps.hlsl";
		brighten_renderer = std::make_shared<KGL::_2D::Renderer>(device, desc.dxc, renderer_desc);
	}

	fxaa_mgr = std::make_shared<FXAAManager>(device, desc.dxc, desc.app->GetResolution());

	{
		cube_texture = std::make_shared<KGL::TextureCube>();
		cube_texture->Create(device, XMUINT2(128u, 128u), DXGI_FORMAT_R8G8B8A8_UNORM, 1u, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		cube_depth_texture = std::make_shared<KGL::TextureCube>();
		cube_depth_texture->Create(device, XMUINT2(128u, 128u), DXGI_FORMAT_R32G8X24_TYPELESS, 1u, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		std::vector<ComPtr<ID3D12Resource>> resources(1);
		resources[0] = cube_texture->Data();
		cube_rtv = std::make_shared<KGL::RenderTargetView>(device, resources);
	}

	return hr;
}

static void UpdateCubeBuffer(
	TestScene09::CubeMapBuffer* data, 
	const DirectX::XMFLOAT3& pos
	)
{
	KGL::VecCamera camera;
	camera.eye = pos;
	camera.up = { 0.f, 1.f, 0.f };

	camera.focus_vec = { +1.f, +0.f, +0.f };
	XMStoreFloat4x4(&data->view_mat[0], KGL::CAMERA::GetView(camera));

	camera.focus_vec = { -1.f, +0.f, +0.f };
	XMStoreFloat4x4(&data->view_mat[1], KGL::CAMERA::GetView(camera));

	camera.focus_vec = { +0.f, +1.f, +0.f };
	XMStoreFloat4x4(&data->view_mat[2], KGL::CAMERA::GetView(camera));

	camera.focus_vec = { +0.f, -1.f, +0.f };
	XMStoreFloat4x4(&data->view_mat[3], KGL::CAMERA::GetView(camera));

	camera.focus_vec = { +0.f, +0.f, +1.f };
	XMStoreFloat4x4(&data->view_mat[4], KGL::CAMERA::GetView(camera));

	camera.focus_vec = { +0.f, +0.f, -1.f };
	XMStoreFloat4x4(&data->view_mat[5], KGL::CAMERA::GetView(camera));
}

HRESULT TestScene09::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	// 各種Actorの初期設定
	{
		inc_actor->position = { -2, 0, 0 };
		inc_actor->scale = { 0.5f, 0.5f, 0.5f };
		inc_actor->rotate = { 0.0f, XM_PI, 0.0f };

		slime_actor->position = { 2, 0, 0 };
		slime_actor->scale = { 0.5f, 0.5f, 0.5f };
		slime_actor->rotate = { 0.0f, XM_PI, 0.0f };

		sky_actor->position = { 0, 0, 0 };
		sky_actor->scale = { 1.f, 1.f, 1.f };
		sky_actor->rotate = { 0.0f, 0.0f, 0.0f };

		earth_actor->position = { 0, 0, 0 };
		earth_actor->scale = { 2.f, 2.f, 2.f };
		earth_actor->rotate = { 0.0f, 0.0f, 0.0f };

		bison_actor->position = { 0, 2, 0 };
		bison_actor->scale = { 1.f, 1.f, 1.f };
		bison_actor->rotate = { 0.0f, 0.0f, 0.0f };
	}

	camera = std::make_shared<FPSCamera>(XMFLOAT3(0.f, 0.f, -10.f));
	camera->GetFront() = { 0.f, 0.f, 1.f };

	{
		auto* mapped_fb = frame_buffer->Map();
		mapped_fb->eye_pos = camera->GetPos();

		XMVECTOR light_vec = XMVectorSet(0.1f, -1.f, -0.4f, 0.f);
		XMStoreFloat3(&mapped_fb->light_vec, XMVector3Normalize(light_vec));
		mapped_fb->light_color = { 1.f, 1.f, 1.f };
		mapped_fb->ambient_light_color = { 1.f, 1.f, 1.f };

		const auto resolution = desc.app->GetResolution();
		const XMMATRIX proj = XMMatrixPerspectiveFovLH(
			XMConvertToRadians(70.f),	// FOV
			SCAST<float>(resolution.x) / SCAST<float>(resolution.y),	// アスペクト比
			0.1f, 1000.0f // near, far
		);
		const auto view = camera->GetView();
		DirectX::XMStoreFloat4x4(&mapped_fb->proj, proj);
		DirectX::XMStoreFloat4x4(&mapped_fb->view, view);
		DirectX::XMStoreFloat4x4(&mapped_fb->view_proj, view * proj);
		frame_buffer->Unmap();
	}
	

	{
		auto* mapped_cube_buffer = cube_buffer->Map();
		const XMMATRIX proj = XMMatrixPerspectiveFovLH(
			XM_PI / 2,	// FOV
			256.f / 256.f,	// アスペクト比
			0.1f, 500.0f // near, far
		);
		XMStoreFloat4x4(&mapped_cube_buffer->proj, proj);
		UpdateCubeBuffer(mapped_cube_buffer, earth_actor->position);
		cube_buffer->Unmap();
	}

	return S_OK;
}

HRESULT TestScene09::Update(const SceneDesc& desc, float elapsed_time)
{
	using namespace DirectX;
	auto input = desc.input;

	// 終了キー
	if (input->IsKeyPressed(KGL::KEYS::ESCAPE))
		return E_FAIL;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// [←][→]キーでシーン移動
	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<LoadScene00<TestScene04>>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<LoadScene00<TestScene00>>(desc);

	camera->Update(desc.window, desc.input, elapsed_time, 10.f,
		desc.input->IsMouseHold(KGL::MOUSE_BUTTONS::right));

	XMMATRIX view_proj;
	{	// フレームバッファを設定
		auto* mapped_fb = frame_buffer->Map();
		mapped_fb->eye_pos = camera->GetPos();
		const auto view = camera->GetView();
		DirectX::XMStoreFloat4x4(&mapped_fb->view, view);
		view_proj = view * XMLoadFloat4x4(&mapped_fb->proj);
		DirectX::XMStoreFloat4x4(&mapped_fb->view_proj, view_proj);
		frame_buffer->Unmap();
	}
	{
		auto* mapped_cube_buffer = cube_buffer->Map();
		UpdateCubeBuffer(mapped_cube_buffer, earth_actor->position);
		cube_buffer->Unmap();
	}

	if (ImGui::Begin("Debug"))
	{
		if (ImGui::TreeNode("FrameBuffer"))
		{
			auto* mapped_fb = frame_buffer->Map();
			if (ImGui::SliderFloat3("light_angle", (float*)&mapped_fb->light_vec, -1.f, 1.f))
			{
				XMVECTOR v = XMLoadFloat3(&mapped_fb->light_vec);
				v = XMVector3Normalize(v);
				XMStoreFloat3(&mapped_fb->light_vec, v);
			}
			ImGui::SliderFloat3("ambient_light", (float*)&mapped_fb->ambient_light_color, 0.f, 1.f);
			ImGui::SliderFloat3("light_color", (float*)&mapped_fb->light_color, 0.f, 1.f);
			
			frame_buffer->Unmap();
			ImGui::TreePop();
		}
		UINT actor_idx = 0u;
		for (auto& actor : s_actors)
		{
			if (ImGui::TreeNode((actor->GetModel()->GetPath().string() + "##" + std::to_string(actor_idx++)).c_str()))
			{
				if (ImGui::SliderFloat3("Pos", &actor->position.x, -100.f, 100.f, "%.6f"))
				{
					//actor->scale.z = actor->scale.y = actor->scale.x;
				}
				if (ImGui::SliderFloat("Scale", &actor->scale.x, 0.f, 10.f, "%.6f"))
				{
					actor->scale.z = actor->scale.y = actor->scale.x;
				}
				auto deg_angle = actor->rotate;
				deg_angle.x = XMConvertToDegrees(deg_angle.x);
				deg_angle.y = XMConvertToDegrees(deg_angle.y);
				deg_angle.z = XMConvertToDegrees(deg_angle.z);
				if (ImGui::SliderFloat3("Rotate", &deg_angle.x, 0.f, 360.f, "%.6f"))
				{
					actor->rotate.x = XMConvertToRadians(deg_angle.x);
					actor->rotate.y = XMConvertToRadians(deg_angle.y);
					actor->rotate.z = XMConvertToRadians(deg_angle.z);
				}

				UINT idx = 0u;
				for (auto& it : actor->GetMaterials())
				{
					if (ImGui::TreeNode((it.first + "##" + std::to_string(idx++)).c_str()))
					{
						auto& rc = it.second.resource;
						{
							auto* mtl_buffer = rc->Map();
							ImGui::SliderFloat3("ambient_color", (float*)&mtl_buffer->ambient_color, 0.f, 1.f);
							if (ImGui::SliderFloat("##0", &mtl_buffer->ambient_color.x, 0.f, 1.f))
							{
								mtl_buffer->ambient_color.z = mtl_buffer->ambient_color.y = mtl_buffer->ambient_color.x;
							}
							ImGui::SliderFloat3("diffuse_color", (float*)&mtl_buffer->diffuse_color, 0.f, 1.f);
							if (ImGui::SliderFloat("##1", &mtl_buffer->diffuse_color.x, 0.f, 1.f))
							{
								mtl_buffer->diffuse_color.z = mtl_buffer->diffuse_color.y = mtl_buffer->diffuse_color.x;
							}
							ImGui::SliderFloat3("specular_color", (float*)&mtl_buffer->specular_color, 0.f, 1.f);
							if (ImGui::SliderFloat("##2", &mtl_buffer->specular_color.x, 0.f, 1.f))
							{
								mtl_buffer->specular_color.z = mtl_buffer->specular_color.y = mtl_buffer->specular_color.x;
							}

							ImGui::SliderFloat("specular_weight", &mtl_buffer->specular_weight, 0.f, 1.f);
							ImGui::Checkbox("specular_flg", &mtl_buffer->specular_flg);
							ImGui::SliderFloat("dissolve", &mtl_buffer->dissolve, 0.f, 1.f);
							ImGui::SliderFloat("refraction", &mtl_buffer->refraction, 0.f, 1.f);
							ImGui::Checkbox("smooth", &mtl_buffer->smooth);
							rc->Unmap();
						}
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();

	for (auto& actor : s_actors)
	{
		actor->UpdateBuffer(view_proj);
	}

	ImGui::Render();
	return Render(desc);
}

HRESULT TestScene09::Render(const SceneDesc& desc)
{
	const bool fxaa = fxaa_mgr->GetDesc().type == FXAAManager::TYPE::FXAA_ON;
	auto resolution = desc.app->GetResolution();
	const DirectX::XMFLOAT2 resolutionf = { SCAST<FLOAT>(resolution.x), SCAST<FLOAT>(resolution.y) };

	// ビューとシザーをセット
	D3D12_VIEWPORT full_viewport =
		CD3DX12_VIEWPORT(0.f, 0.f, resolutionf.x, resolutionf.y);
	auto full_scissorrect =
		CD3DX12_RECT(0, 0, SCAST<LONG>(resolutionf.x), SCAST<LONG>(resolutionf.y));
	D3D12_VIEWPORT viewport =
		CD3DX12_VIEWPORT(0.f, 0.f, resolutionf.x, resolutionf.y);
	auto scissorrect =
		CD3DX12_RECT(0, 0, resolution.x, resolution.y);
	cmd_list->RSSetViewports(1, &full_viewport);
	cmd_list->RSSetScissorRects(1, &full_scissorrect);

	{
		const auto& rbrt_world = rtvs->GetRtvResourceBarrier(true, RT::WORLD);
		cmd_list->ResourceBarrier(1u, &rbrt_world);
		desc.app->ClearDsv(cmd_list);
		const auto& dsv_handle = desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
		rtvs->Set(cmd_list, &dsv_handle, RT::WORLD);
		rtvs->Clear(cmd_list, rt_textures[RT::WORLD]->GetClearColor(), RT::WORLD);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		s_model_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, descriptor->Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, frame_buffer_handle->Gpu());

		// 各モデルの描画
		inc_actor->Render(cmd_list);
		slime_actor->Render(cmd_list);
		earth_actor->Render(cmd_list);
		bison_actor->Render(cmd_list);

		const auto& rbsr_world = rtvs->GetRtvResourceBarrier(false, RT::WORLD);
		cmd_list->ResourceBarrier(1u, &rbsr_world);
	}
	{
		const auto& rbrt_world_bt = rtvs->GetRtvResourceBarrier(true, RT::WORLD_BT);
		cmd_list->ResourceBarrier(1u, &rbrt_world_bt);

		rtvs->Set(cmd_list, nullptr, RT::WORLD_BT);
		rtvs->Clear(cmd_list, rt_textures[RT::WORLD_BT]->GetClearColor(), RT::WORLD_BT);
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		brighten_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(RT::WORLD));
		sprite->Render(cmd_list);

		const auto& dsv_handle = desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
		rtvs->Set(cmd_list, &dsv_handle, RT::WORLD_BT);

		// 空の描画
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		sky_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, descriptor->Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, frame_buffer_handle->Gpu());
		sky_actor->Render(cmd_list);

		const auto& rbsr_world_bt = rtvs->GetRtvResourceBarrier(false, RT::WORLD_BT);
		cmd_list->ResourceBarrier(1u, &rbsr_world_bt);
	}
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	{
		const auto& rbrt = desc.app->GetRtvResourceBarrier(true);
		cmd_list->ResourceBarrier(1u, &rbrt);
	}
	if (fxaa)
	{	// FXAA用のレンダーターゲットに切り替え
		const auto& rbrt_fxaa = rtvs->GetRtvResourceBarrier(true, RT::FXAA);
		cmd_list->ResourceBarrier(1u, &rbrt_fxaa);
		rtvs->Set(cmd_list, nullptr, RT::FXAA);
		rtvs->Clear(cmd_list, rt_textures[RT::FXAA]->GetClearColor(), RT::FXAA);
		fxaa_mgr->SetGrayState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(RT::WORLD_BT));
		sprite->Render(cmd_list);
		const auto& rbsr_fxaa = rtvs->GetRtvResourceBarrier(false, RT::FXAA);
		cmd_list->ResourceBarrier(1u, &rbsr_fxaa);

		// SCのレンダーターゲットに切り替え
		desc.app->SetRtv(cmd_list);
		fxaa_mgr->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(RT::FXAA));
		sprite->Render(cmd_list);
	}
	else
	{
		desc.app->SetRtv(cmd_list);
		sprite_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(RT::WORLD_BT));
		sprite->Render(cmd_list);
	}

	// IMGUI
	cmd_list->SetDescriptorHeaps(1, desc.imgui_handle.Heap().GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list.Get());
	
	{
		const auto& rbpr = desc.app->GetRtvResourceBarrier(false);
		cmd_list->ResourceBarrier(1u, &rbpr);
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
	return S_OK;
}

HRESULT TestScene09::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	return S_OK;
}