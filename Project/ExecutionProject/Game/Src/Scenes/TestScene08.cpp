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
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);

	cube_buffer = std::make_shared<KGL::Resource<CubeMapBuffer>>(device, 1u);

	// KGL::OBJ_Loader obj_file("./Assets/Models/Mr.Incredible/Mr.Incredible.obj");
	std::shared_ptr<KGL::StaticModelLoader> s_loader =
		std::make_shared<KGL::OBJ_Loader>(
			"./Assets/Models/SpaceShip/99-intergalactic_spaceship-obj/Intergalactic_Spaceship-(Wavefront).obj"
			);
	auto& model = s_models.emplace_back();
	model = std::make_shared<KGL::StaticModel>(device, s_loader);

	return hr;
}

HRESULT TestScene08::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	return S_OK;
}

HRESULT TestScene08::Update(const SceneDesc& desc, float elapsed_time)
{
	// [←][→]キーでシーン移動
	auto input = desc.input;
	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<LoadScene00<TestScene04>>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<LoadScene00<TestScene00>>(desc);

	return Render(desc);
}

HRESULT TestScene08::Render(const SceneDesc& desc)
{


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