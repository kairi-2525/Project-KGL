#include "../../Hrd/Scenes/TestSceneBase.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>

HRESULT TestSceneBase::Load(const SceneDesc& desc)
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
	pmd_renderer = std::make_shared<KGL::PMD_Renderer>(device);

	models.resize(1, { device, *pmd_model });
	for (auto& model : models)
	{
		model.SetAnimation(vmd_data->GetDesc());
	}

	return hr;
}

HRESULT TestSceneBase::Init(const SceneDesc& desc)
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

	clear_color = { 1.f, 1.f, 1.f, 1.f };

	return S_OK;
}

HRESULT TestSceneBase::Update(const SceneDesc& desc, float elapsed_time)
{
	using namespace DirectX;

	for (auto& model : models)
	{
		//model.position.x -= elapsed_time * ((rand() % (20 + 1)) - 10);
		//model.position.z -= elapsed_time * ((rand() % (20 + 1)) - 10);
		model.MotionSetup(elapsed_time, true);
		model.MotionMatrixUpdate();
		model.Update(elapsed_time);
		//model.UpdateWVP();
	}

	return Render(desc);
}

HRESULT TestSceneBase::Render(const SceneDesc& desc)
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

	{	// モデルを描画したテクスチャをSwapchainのレンダーターゲットに描画(歪みNormal)
		desc.app->SetRtvDsv(cmd_list);
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtvDsv(cmd_list, clear_color);

		cmd_list->RSSetViewports(1, &viewport);
		cmd_list->RSSetScissorRects(1, &scissorrect);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		pmd_renderer->SetState(cmd_list);
		for (auto& model : models)
		{
			model.Render(cmd_list);

			hr = pmd_toon_model->Render(
				desc.app->GetDevice(),
				cmd_list
			);
			RCHECK(FAILED(hr), "pmd_model->Renderに失敗", hr);
		}

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

HRESULT TestSceneBase::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	return S_OK;
}