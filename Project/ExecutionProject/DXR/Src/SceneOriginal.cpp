#include "../Hrd/Scene.hpp"
#include <DirectXTex/d3dx12.h>
#include <Dx12/Helper.hpp>

HRESULT SceneOriginal::Load(const SceneDesc& desc)
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

	hr = device->QueryInterface(IID_PPV_ARGS(dxr_device.GetAddressOf()));
	RCHECK(FAILED(hr), "dxr_deviceの作成に失敗", hr);
	hr = cmd_list->QueryInterface(IID_PPV_ARGS(dxr_cmd_list.GetAddressOf()));
	RCHECK(FAILED(hr), "コマンドリスト4の作成に失敗", hr);

	{
		t_vert_res = std::make_shared<KGL::Resource<TriangleVertex>>(device, 3);
		std::vector<TriangleVertex> vertex(3);
		vertex[0] = { { +0.0f, +0.7f, +0.0f }, { 1.f, 1.f, 0.f, 1.f } };
		vertex[1] = { { +0.7f, -0.7f, +0.0f }, { 0.f, 1.f, 1.f, 1.f } };
		vertex[2] = { { -0.7f, -0.7f, +0.0f }, { 1.f, 0.f, 1.f, 1.f } };

		auto* mapped_vertices = t_vert_res->Map(0, &CD3DX12_RANGE(0, 0));
		std::copy(vertex.cbegin(), vertex.cend(), mapped_vertices);
		t_vert_res->Unmap(0, &CD3DX12_RANGE(0, 0));

		t_vert_view.BufferLocation = t_vert_res->Data()->GetGPUVirtualAddress();
		t_vert_view.StrideInBytes = sizeof(TriangleVertex);
		t_vert_view.SizeInBytes = SCAST<UINT>(sizeof(TriangleVertex) * vertex.size());

		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		renderer_desc.vs_desc.hlsl = "./HLSL/2D/Triangle_vs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/Triangle_ps.hlsl";
		renderer_desc.vs_desc.version = "vs_6_0";
		renderer_desc.ps_desc.version = "ps_6_0";

		renderer_desc.input_layouts.clear();
		renderer_desc.input_layouts.push_back({
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.root_params.clear();
		renderer_desc.static_samplers.clear();
		t_renderer = std::make_shared<KGL::BaseRenderer>(device, desc.dxc, renderer_desc);
	}

	return hr;
}

HRESULT SceneOriginal::Init(const SceneDesc& desc)
{
	HRESULT hr = S_OK;

	raster = true;

	return hr;
}

HRESULT SceneOriginal::Update(const SceneDesc& desc, float elapsed_time)
{
	HRESULT hr = S_OK;

	const auto& input = desc.input;

	if (input->IsKeyPressed(KGL::KEYS::ENTER))
	{
		SetNextScene<SceneMain>(desc);
	}
	if (input->IsKeyPressed(KGL::KEYS::SPACE))
	{
		raster = !raster;
	}

	return Render(desc);
}

HRESULT SceneOriginal::Render(const SceneDesc& desc)
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

	if (raster)
	{
		dxr_cmd_list->RSSetViewports(1, &viewport);
		dxr_cmd_list->RSSetScissorRects(1, &scissorrect);

		desc.app->SetRtvDsv(dxr_cmd_list);
		dxr_cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtvDsv(dxr_cmd_list, DirectX::XMFLOAT4(0.0f, 0.2f, 0.4f, 1.f));

		dxr_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		t_renderer->SetState(dxr_cmd_list);
		dxr_cmd_list->IASetVertexBuffers(0, 1, &t_vert_view);
		dxr_cmd_list->DrawInstanced(3, 1, 0, 0);

		dxr_cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));
	}
	else
	{
	}

	dxr_cmd_list->Close();
	ID3D12CommandList* dxr_cmd_lists[] = { dxr_cmd_list.Get() };
	desc.app->GetQueue()->Data()->ExecuteCommandLists(1, dxr_cmd_lists);
	desc.app->GetQueue()->Signal();
	desc.app->GetQueue()->Wait();

	cmd_allocator->Reset();
	dxr_cmd_list->Reset(cmd_allocator.Get(), nullptr);

	if (desc.app->IsTearingSupport())
		desc.app->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	else
		desc.app->GetSwapchain()->Present(1, 1);

	return hr;
}

HRESULT SceneOriginal::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	HRESULT hr = S_OK;
	return hr;
}