#include "../Hrd/Scene.hpp"
#include <DirectXTex/d3dx12.h>
#include <Dx12/Helper.hpp>
#include <Loader/OBJLoader.hpp>

HRESULT SceneOriginal::CreatePSO(const SceneDesc& desc)
{
	KGL::DXR::BaseRenderer::Desc renderer_desc = {};

	auto& raygen_sig = renderer_desc.signatures["RayGen"];
	auto& miss_sig = renderer_desc.signatures["Miss"];
	auto& hit_sig = renderer_desc.signatures["ClosestHit"];

	raygen_sig.shader.version =
		miss_sig.shader.version =
			hit_sig.shader.version = "lib_6_3";

	raygen_sig.shader.hlsl	= "./HLSL/DXR/StaticModel/RayGen.hlsl";
	miss_sig.shader.hlsl	= "./HLSL/DXR/StaticModel/Miss.hlsl";
	hit_sig.shader.hlsl		= "./HLSL/DXR/StaticModel/Hit.hlsl";

	// entry_points(?)
	raygen_sig.shader.entry_points.push_back("RayGen");
	miss_sig.shader.entry_points.push_back("Miss");
	hit_sig.shader.entry_points.push_back("ClosestHit");

	raygen_sig.shader.symbols.push_back("RayGen");
	miss_sig.shader.symbols.push_back("Miss");
	hit_sig.shader.symbols.push_back("HitGroup");

	// raygen シェーダー用リソース
	std::vector<D3D12_DESCRIPTOR_RANGE> raygen_ranges(2);
	CD3DX12_DESCRIPTOR_RANGE::Init(raygen_ranges[0], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u);
	CD3DX12_DESCRIPTOR_RANGE::Init(raygen_ranges[1], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 0u);
	CD3DX12_ROOT_PARAMETER raygen_param;
	raygen_param.InitAsDescriptorTable(SCAST<UINT>(raygen_ranges.size()), raygen_ranges.data());
	raygen_sig.root_params.push_back(raygen_param);

	// hit シェーダー用リソース
	std::vector<D3D12_DESCRIPTOR_RANGE> hit_ranges(1);
	CD3DX12_DESCRIPTOR_RANGE::Init(hit_ranges[0], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1u, 0u);
	CD3DX12_ROOT_PARAMETER hit_param;
	hit_param.InitAsDescriptorTable(SCAST<UINT>(hit_ranges.size()), hit_ranges.data());
	hit_sig.root_params.push_back(hit_param);


	renderer_desc.hit_groups["HitGroup"] = "ClosestHit";
	renderer_desc.max_trace_recursion_depth = 1u;
	renderer_desc.shader_config.MaxPayloadSizeInBytes = 4 * sizeof(float); // RGB + distance
	renderer_desc.shader_config.MaxAttributeSizeInBytes = 2 * sizeof(float); // 重心座標

	dxr_renderer = std::make_shared<KGL::DXR::BaseRenderer>(dxr_device, desc.dxc, renderer_desc);

	return S_OK;
}

HRESULT SceneOriginal::CreateShaderResource(const SceneDesc& desc)
{
	const auto& resolution = desc.app->GetResolution();

	srv_uav_discriptor = std::make_shared<KGL::DescriptorManager>(dxr_device, 2u);
	output_uav_handle = std::make_shared<KGL::DescriptorHandle>(srv_uav_discriptor->Alloc());

	D3D12_RESOURCE_DESC res_desc = {};
	res_desc.DepthOrArraySize = 1;
	res_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	res_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	res_desc.Width = resolution.x;
	res_desc.Height = resolution.y;
	res_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	res_desc.MipLevels = 1;
	res_desc.SampleDesc.Count = 1;

	dxr_output_tex = std::make_shared<KGL::Texture>(dxr_device, res_desc, D3D12_CLEAR_VALUE(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	dxr_output_tex->CreateUAVHandle(output_uav_handle);
	return S_OK;
}

HRESULT SceneOriginal::CreateSBT(const SceneDesc& desc)
{
	KGL::DXR::SBT::Desc sbt_desc = {};

	auto& sbt_raygen = sbt_desc.raygen_table.emplace_back("RayGen");
	sbt_raygen.input_data.push_back(RCAST<UINT64*>(output_uav_handle->Gpu().ptr));

	// ミスシェーダーとヒットシェーダーは外部リソースにアクセスせず、
	// 代わりにレイペイロードを通じて結果を伝達します。
	sbt_desc.miss_table.emplace_back("Miss");	// カメラレイ用とシャドウレイ用のミスシェーダーがあるため
	sbt_desc.miss_table.emplace_back("Miss");

	auto& sbt_hit_group = sbt_desc.hit_group_table.emplace_back("HitGroup");
	const auto& vertices_data = dxr_model->GetMaterials().begin()->second.rs_vertices->Data();
	sbt_hit_group.input_data.push_back((void*)(vertices_data->GetGPUVirtualAddress()));

	dxr_sbt = std::make_shared<KGL::DXR::SBT>(dxr_device, sbt_desc);

	return S_OK;
}

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

	// 三角ポリゴン
	{
		t_vert_res = std::make_shared<KGL::Resource<TriangleVertex>>(device, 3);
		std::vector<TriangleVertex> vertex(3);
		vertex[0] = { { +0.0f, +0.7f, +0.0f }, { 1.f, 1.f, 0.f, 1.f } };
		vertex[1] = { { +0.7f, -0.7f, +0.0f }, { 0.f, 1.f, 1.f, 1.f } };
		vertex[2] = { { -0.7f, -0.7f, +0.0f }, { 1.f, 0.f, 1.f, 1.f } };

		auto* mapped_vertices = t_vert_res->Map();
		std::copy(vertex.cbegin(), vertex.cend(), mapped_vertices);
		t_vert_res->Unmap();

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

	{
		auto loader = std::make_shared<KGL::OBJ_Loader>("./Assets/Models/Slime/Slime.obj", true);
		if (!loader->IsFastLoad())
			loader->Export(loader->GetPath());
		dxr_model = std::make_shared<KGL::DXR::StaticModel>(dxr_device, dxr_cmd_list, loader);
		
		std::vector<KGL::DXR::BLAS> instances;
		instances.push_back(dxr_model->GetBLAS());
		auto tlas = KGL::DXR::CreateTLAS(dxr_device, dxr_cmd_list, instances);

		dxr_cmd_list->Close();
		ID3D12CommandList* dxr_cmd_lists[] = { dxr_cmd_list.Get() };
		desc.app->GetQueue()->Data()->ExecuteCommandLists(1, dxr_cmd_lists);
		desc.app->GetQueue()->Signal();

		// 処理中にその他準備物を作成
		CreatePSO(desc);
		CreateShaderResource(desc);
		CreateSBT(desc);


		desc.app->GetQueue()->Wait();

		cmd_allocator->Reset();
		dxr_cmd_list->Reset(cmd_allocator.Get(), nullptr);
	}

	return hr;
}

HRESULT SceneOriginal::Init(const SceneDesc& desc)
{
	HRESULT hr = S_OK;

	raster = false;

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
	auto resolution = desc.app->GetResolution();

	D3D12_VIEWPORT viewport = {};
	viewport.Width = SCAST<FLOAT>(resolution.x);
	viewport.Height = SCAST<FLOAT>(resolution.y);
	viewport.TopLeftX = 0;//出力先の左上座標X
	viewport.TopLeftY = 0;//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;//深度最大値
	viewport.MinDepth = 0.0f;//深度最小値

	auto scissorrect = CD3DX12_RECT(
		0, 0,
		resolution.x, resolution.y
	);

	if (raster)
	{
		dxr_cmd_list->RSSetViewports(1, &viewport);
		dxr_cmd_list->RSSetScissorRects(1, &scissorrect);

		desc.app->SetRtvDsv(dxr_cmd_list);
		const auto& rbrt = desc.app->GetRtvResourceBarrier(true);
		dxr_cmd_list->ResourceBarrier(1, &rbrt);
		desc.app->ClearRtvDsv(dxr_cmd_list, DirectX::XMFLOAT4(0.0f, 0.2f, 0.4f, 1.f));

		dxr_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		t_renderer->SetState(dxr_cmd_list);
		dxr_cmd_list->IASetVertexBuffers(0, 1, &t_vert_view);
		dxr_cmd_list->DrawInstanced(3, 1, 0, 0);

		const auto& rbpr = desc.app->GetRtvResourceBarrier(false);
		dxr_cmd_list->ResourceBarrier(1, &rbpr);
	}
	else
	{
		const auto& rbrt = desc.app->GetRtvResourceBarrier(true);
		dxr_cmd_list->ResourceBarrier(1, &rbrt);

		std::vector<ID3D12DescriptorHeap*> heaps = { srv_uav_discriptor->Heap().Get() };
		dxr_cmd_list->SetDescriptorHeaps(SCAST<UINT>(heaps.size()), heaps.data());

		// OutputテクスチャをUAV用リソースに切り替え
		auto transition = dxr_output_tex->RB(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		dxr_cmd_list->ResourceBarrier(1, &transition);

		{
			D3D12_DISPATCH_RAYS_DESC ray_desc = {};
			dxr_sbt->GenerateRayDesc(&ray_desc);

			ray_desc.Width = resolution.x;
			ray_desc.Height = resolution.y;
			ray_desc.Depth = 1;

			dxr_renderer->Set(dxr_cmd_list);
			// 光線をディスパッチし、光線追跡出力に書き込みます
			dxr_cmd_list->DispatchRays(&ray_desc);
		}

		// Outputテクスチャへ書きこみが完了したのでコピーリソースに切り替え、レンダーターゲットへコピーする
		transition = dxr_output_tex->RB(D3D12_RESOURCE_STATE_COPY_SOURCE);
		dxr_cmd_list->ResourceBarrier(1, &transition);

		const auto& back_buffer = desc.app->GetRtvBuffers().at(desc.app->GetSwapchain()->GetCurrentBackBufferIndex());
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			back_buffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_DEST
		);
		dxr_cmd_list->ResourceBarrier(1, &transition);

		dxr_cmd_list->CopyResource(back_buffer.Get(), dxr_output_tex->Data().Get());

		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			back_buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PRESENT
		);
		dxr_cmd_list->ResourceBarrier(1, &transition);
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