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
	cmd_allocator->SetName(L"Main CA");
	cmd_list->SetName(L"Main CL");
	RCHECK(FAILED(hr), "�R�}���h�A���P�[�^�[/���X�g�̍쐬�Ɏ��s", hr);

	return hr;
}

HRESULT TestSceneBase::Init(const SceneDesc& desc)
{

	clear_color = { 1.f, 1.f, 1.f, 1.f };

	return S_OK;
}

HRESULT TestSceneBase::Update(const SceneDesc& desc, float elapsed_time)
{
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
	viewport.TopLeftX = 0;//�o�͐�̍�����WX
	viewport.TopLeftY = 0;//�o�͐�̍�����WY
	viewport.MaxDepth = 1.0f;//�[�x�ő�l
	viewport.MinDepth = 0.0f;//�[�x�ŏ��l

	auto scissorrect = CD3DX12_RECT(
		0, 0,
		window_size.x, window_size.y
	);

	{
		desc.app->SetRtvDsv(cmd_list);
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtvDsv(cmd_list, clear_color);
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