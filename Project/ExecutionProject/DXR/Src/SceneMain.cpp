#include "../Hrd/Scene.hpp"
#include <Helper/Cast.hpp>
#include <DirectXTex/d3dx12.h>
#include <Dx12/Helper.hpp>
#include <Helper/ThrowAssert.hpp>

HRESULT SceneMain::Load(const SceneDesc& desc)
{
	HRESULT hr = S_OK;

	try
	{
		if (!desc.app->IsDXRSupport())
			throw std::runtime_error("この環境ではDXRを実行できません。");
	}
	catch (std::runtime_error& exception)
	{
		KGL::RuntimeErrorStop(exception);
	}

	const auto& device = desc.app->GetDevice();
	KGL::ComPtr<ID3D12GraphicsCommandList> cmd_list;
	hr = KGL::DX12::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);

	hr = device->QueryInterface(IID_PPV_ARGS(device5.GetAddressOf()));
	RCHECK(FAILED(hr), "device5の作成に失敗", hr);
	hr = cmd_list->QueryInterface(IID_PPV_ARGS(cmd_list4.GetAddressOf()));
	RCHECK(FAILED(hr), "コマンドリスト4の作成に失敗", hr);

	return hr;
}

HRESULT SceneMain::Init(const SceneDesc& desc)
{
	HRESULT hr = S_OK;
	return hr;
}

HRESULT SceneMain::Update(const SceneDesc& desc, float elapsed_time)
{
	HRESULT hr = S_OK;
	return Render(desc);
}

HRESULT SceneMain::Render(const SceneDesc& desc)
{
	HRESULT hr = S_OK;

	using KGL::SCAST;
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

	desc.app->SetRtvDsv(cmd_list4);
	cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
	desc.app->ClearRtvDsv(cmd_list4, DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f));

	cmd_list4->RSSetViewports(1, &viewport);
	cmd_list4->RSSetScissorRects(1, &scissorrect);

	cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));

	cmd_list4->Close();
	ID3D12CommandList* cmd_list4s[] = { cmd_list4.Get() };
	desc.app->GetQueue()->Data()->ExecuteCommandLists(1, cmd_list4s);
	desc.app->GetQueue()->Signal();
	desc.app->GetQueue()->Wait();

	cmd_allocator->Reset();
	cmd_list4->Reset(cmd_allocator.Get(), nullptr);

	if (desc.app->IsTearingSupport())
		desc.app->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	else
		desc.app->GetSwapchain()->Present(1, 1);

	return hr;
}

HRESULT SceneMain::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	HRESULT hr = S_OK;
	return hr;
}