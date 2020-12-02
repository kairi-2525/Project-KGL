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

HRESULT TestScene08::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);

	descriptor = std::make_shared<KGL::DescriptorManager>(device, 1u);
	frame_buffer = std::make_shared<KGL::Resource<FrameBuffer>>(device, 1u);
	frame_buffer_handle = std::make_shared<KGL::DescriptorHandle>(descriptor->Alloc());
	frame_buffer->CreateCBV(frame_buffer_handle);

	/*std::shared_ptr<KGL::StaticModelLoader> s_loader =
		std::make_shared<KGL::OBJ_Loader>(
			"./Assets/Models/Mr.Incredible/Mr.Incredible.obj"
			);*/
	std::shared_ptr<KGL::StaticModelLoader> s_loader =
		std::make_shared<KGL::OBJ_Loader>(
			"./Assets/Models/SpaceShip/99-intergalactic_spaceship-obj/Intergalactic_Spaceship-(Wavefront).obj"
			);
	s_model = std::make_shared<KGL::StaticModel>(device, s_loader);
	auto& actor = s_actors.emplace_back();
	actor = std::make_shared<KGL::StaticModelActor>(device, s_model);
	
	{
		auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;

		renderer_desc.vs_desc.hlsl = "./HLSL/3D/StaticModel_vs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/StaticModel_ps.hlsl";

		renderer_desc.input_layouts = KGL::StaticModel::INPUT_LAYOUTS;
		renderer_desc.root_params = KGL::StaticModel::ROOT_PARAMS;

		s_model_renderer = std::make_shared<KGL::BaseRenderer>(device, desc.dxc, renderer_desc);
	}

	return hr;
}

HRESULT TestScene08::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	s_actors[0]->position.z = 10;
	s_actors[0]->scale.x = s_actors[0]->scale.y = s_actors[0]->scale.z = 0.001f;

	camera.eye = { 0.f, 0.f, -10.f };
	camera.focus = s_actors[0]->position;
	camera.up = { 0.f, 1.f, 0.f };

	{
		auto* mapped_fb = frame_buffer->Map();
		mapped_fb->eye_pos = camera.eye;

		XMVECTOR light_vec = XMVectorSet(0.1f, -1.f, -0.4f, 0.f);
		XMStoreFloat3(&mapped_fb->light_vec, XMVector3Normalize(light_vec));

		const auto resolution = desc.app->GetResolution();
		const XMMATRIX proj = XMMatrixPerspectiveFovLH(
			XMConvertToRadians(70.f),	// FOV
			SCAST<float>(resolution.x) / SCAST<float>(resolution.y),	// アスペクト比
			0.1f, 1000.0f // near, far
		);
		const auto view = KGL::CAMERA::GetView(camera);
		DirectX::XMStoreFloat4x4(&mapped_fb->proj, proj);
		DirectX::XMStoreFloat4x4(&mapped_fb->view, view);
		DirectX::XMStoreFloat4x4(&mapped_fb->view_proj, view * proj);
		frame_buffer->Unmap();
	}

	return S_OK;
}

HRESULT TestScene08::Update(const SceneDesc& desc, float elapsed_time)
{
	using namespace DirectX;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// [←][→]キーでシーン移動
	auto input = desc.input;
	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<LoadScene00<TestScene04>>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<LoadScene00<TestScene00>>(desc);

	XMMATRIX view_proj;
	{	// フレームバッファを設定
		auto* mapped_fb = frame_buffer->Map();
		mapped_fb->eye_pos = camera.eye;
		const auto view = KGL::CAMERA::GetView(camera);
		DirectX::XMStoreFloat4x4(&mapped_fb->view, view);
		view_proj = view * XMLoadFloat4x4(&mapped_fb->proj);
		DirectX::XMStoreFloat4x4(&mapped_fb->view_proj, view_proj);
		frame_buffer->Unmap();
	}

	if (ImGui::Begin("Debug"))
	{
		if (ImGui::SliderFloat3("Pos", &s_actors[0]->position.x, -1000, 1000, "%.6f"))
		{
			//s_actors[0]->scale.z = s_actors[0]->scale.y = s_actors[0]->scale.x;
		}
		if (ImGui::SliderFloat3("Scale", &s_actors[0]->scale.x, 0, 1, "%.6f"))
		{
			s_actors[0]->scale.z = s_actors[0]->scale.y = s_actors[0]->scale.x;
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

HRESULT TestScene08::Render(const SceneDesc& desc)
{
	auto resolution = desc.app->GetResolution();
	const DirectX::XMFLOAT2 resolutionf = { SCAST<FLOAT>(resolution.x), SCAST<FLOAT>(resolution.y) };

	// ビューとシザーをセット
	D3D12_VIEWPORT full_viewport =
		CD3DX12_VIEWPORT(0.f, 0.f, resolutionf.x, resolutionf.y);
	auto full_scissorrect =
		CD3DX12_RECT(0, 0, resolutionf.x, resolutionf.y);
	D3D12_VIEWPORT viewport =
		CD3DX12_VIEWPORT(0.f, 0.f, resolutionf.x, resolutionf.y);
	auto scissorrect =
		CD3DX12_RECT(0, 0, resolution.x, resolution.y);
	cmd_list->RSSetViewports(1, &full_viewport);
	cmd_list->RSSetScissorRects(1, &full_scissorrect);

	{
		cmd_list->ResourceBarrier(1u, &desc.app->GetRtvResourceBarrier(true));
		desc.app->SetRtvDsv(cmd_list);
		desc.app->ClearRtvDsv(cmd_list, { 0.5f, 1.0f, 0.5f, 1.f });

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		
		s_model_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, descriptor->Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, frame_buffer_handle->Gpu());
		for (auto& actors : s_actors)
		{
			actors->Render(cmd_list);
		}

		cmd_list->SetDescriptorHeaps(1, desc.imgui_handle.Heap().GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list.Get());

		cmd_list->ResourceBarrier(1u, &desc.app->GetRtvResourceBarrier(false));
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

HRESULT TestScene08::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	return S_OK;
}