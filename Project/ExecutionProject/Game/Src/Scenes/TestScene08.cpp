#include "../../Hrd/Scenes/Scenes.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>
#include <random>
#include <Loader/OBJLoader.hpp>

HRESULT TestScene08::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "�R�}���h�A���P�[�^�[/���X�g�̍쐬�Ɏ��s", hr);

	cube_buffer = std::make_shared<KGL::Resource<CubeMapBuffer>>(device, 1u);

	// KGL::OBJ_Loader obj_file("./Assets/Models/Mr.Incredible/Mr.Incredible.obj");
	std::shared_ptr<KGL::StaticModelLoader> s_loader =
		std::make_shared<KGL::OBJ_Loader>(
			"./Assets/Models/SpaceShip/99-intergalactic_spaceship-obj/Intergalactic_Spaceship-(Wavefront).obj"
			);
	auto& model = s_models.emplace_back();
	model = std::make_shared<KGL::StaticModel>(device, s_loader);

	s_model_renderer = std::make_shared<>

	return hr;
}

HRESULT TestScene08::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	return S_OK;
}

HRESULT TestScene08::Update(const SceneDesc& desc, float elapsed_time)
{
	// [��][��]�L�[�ŃV�[���ړ�
	auto input = desc.input;
	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<LoadScene00<TestScene04>>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<LoadScene00<TestScene00>>(desc);

	return Render(desc);
}

HRESULT TestScene08::Render(const SceneDesc& desc)
{
	auto resolution = desc.app->GetResolution();
	const DirectX::XMFLOAT2 resolutionf = { SCAST<FLOAT>(resolution.x), SCAST<FLOAT>(resolution.y) };

	// �r���[�ƃV�U�[���Z�b�g
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