#include "../Hrd/Scene.hpp"
#include <Helper/Cast.hpp>
#include <DirectXTex/d3dx12.h>
#include <Dx12/Helper.hpp>
#include <Helper/ThrowAssert.hpp>

#include "../DXRHelper/DXRHelper.h"
#include "../DXRHelper/nv_helpers_dx12/BottomLevelASGenerator.h"

// GPUメモリ内の頂点バッファーのリストとその頂点数に基づいて、
// 最下位レベルの加速構造を作成します。 ビルドは3つのステップで行われます。
// ジオメトリの収集、必要なバッファのサイズの計算、実際のASのビルドです。
SceneMain::AccelerationStructureBuffers SceneMain::CreateBottomLevelAS(
	const std::vector<std::pair<KGL::ComPtr<ID3D12Resource>, uint32_t>>& vertex_buffers)
{
	nv_helpers_dx12::BottomLevelASGenerator bottom_level_as;
	// すべての頂点バッファーを追加し、それらの位置を変換しません。
	for (const auto& buffer : vertex_buffers)
	{
		bottom_level_as.AddVertexBuffer(buffer.first.Get(), 0, buffer.second, sizeof(TriangleVertex), 0, 0);
	}

	// ASビルドには、一時的な情報を格納するためのスクラッチ領域が必要です。
	// スクラッチメモリの量は、シーンの複雑さに依存します。
	UINT64 scratch_size_in_bytes = 0;
	// 最後のASは、既存の頂点バッファーに加えて保存する必要もあります。 サイズはシーンの複雑さにも依存します。
	UINT64 result_size_in_bytes = 0;

	bottom_level_as.ComputeASBufferSizes(device5.Get(), false, &scratch_size_in_bytes, &result_size_in_bytes);

	// サイズが取得されると、アプリケーションは必要なバッファを割り当てる必要があります。
	// 生成全体がGPUで行われるため、デフォルトヒープに直接割り当てることができます。
	AccelerationStructureBuffers buffers;
	buffers.scratch.Attach(nv_helpers_dx12::CreateBuffer(device5.Get(), scratch_size_in_bytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
		nv_helpers_dx12::kDefaultHeapProps));
	buffers.result.Attach(nv_helpers_dx12::CreateBuffer(device5.Get(), result_size_in_bytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps));

	// 加速構造を構築します。 この呼び出しは生成されたASのバリアを統合するため、
	// このメソッドの直後にトップレベルのASを計算するために使用できることに注意してください。
	bottom_level_as.Generate(cmd_list4.Get(), buffers.scratch.Get(), buffers.result.Get(), false, nullptr);

	return buffers;
}

// シーンのすべてのインスタンスを保持するメインの加速構造を作成します。 
// 最下位のAS生成と同様に、インスタンスの収集、ASのメモリ要件の計算、
// およびAS自体の構築の3つのステップで行われます。
void SceneMain::CreateTopLevelAS(
	const std::vector<std::pair<KGL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>
	&instances // インスタンスの最下位ASとマトリックスのペア
)
{
	// すべてのインスタンスをビルダーヘルパーに収集します
	for (size_t i = 0; i < instances.size(); i++)
	{
		top_level_as_generator.AddInstance(
			instances[i].first.Get(), instances[i].second,
			KGL::SCAST<UINT>(i), 0u);
	}

	// 最下位のASと同様に、ASの構築には、実際のASに加えて一時データを格納するためのスクラッチ領域が必要です。
	// トップレベルASの場合、インスタンス記述子もGPUメモリに格納する必要があります。
	// この呼び出しは、アプリケーションが対応するメモリを割り当てることができるように、
	// それぞれのメモリ要件（スクラッチ、結果、インスタンス記述子）を出力します
	UINT64 scratch_size, result_size, instance_descs_size;
	top_level_as_generator.ComputeASBufferSizes(device5.Get(), true,
		&scratch_size, &result_size, &instance_descs_size);

	// スクラッチバッファと結果バッファを作成します。
	// ビルドはすべてGPUで行われるため、デフォルトのヒープに割り当てることができます
	top_level_buffers.scratch.Attach(nv_helpers_dx12::CreateBuffer(
		device5.Get(), scratch_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nv_helpers_dx12::kDefaultHeapProps));

	top_level_buffers.result.Attach(nv_helpers_dx12::CreateBuffer(
		device5.Get(), result_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps));

	// インスタンスを説明するバッファー：ID、シェーダーバインディング情報、マトリックス...
	// これらはマッピングを通じてヘルパーによってバッファーにコピーされるため、
	// アップロードヒープにバッファーを割り当てる必要があります。
	top_level_buffers.instance_desc.Attach(nv_helpers_dx12::CreateBuffer(
		device5.Get(), instance_descs_size, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nv_helpers_dx12::kUploadHeapProps));

	// すべてのバッファが割り当てられた後、または更新のみが必要な場合は、加速構造を構築できます。
	// 更新の場合、既存のASを「以前の」ASとしても渡すため、
	// 所定の位置に再フィットできることに注意してください。
	top_level_as_generator.Generate(cmd_list4.Get(),
		top_level_buffers.scratch.Get(),
		top_level_buffers.result.Get(),
		top_level_buffers.instance_desc.Get()
	);
}

// BLASビルドとTLASビルドを組み合わせて、シーンのレイトレースに必要な加速構造全体を構築します
void SceneMain::CreateAccelerationStructures(const std::shared_ptr<KGL::CommandQueue>& queue)
{
	// 三角形の頂点バッファーから下のASを作成します
	AccelerationStructureBuffers bottom_level_buffers =
		CreateBottomLevelAS({ {t_vert_res->Data().Get(), 3} });

	// 現時点では1つのインスタンスのみ
	instances = { { bottom_level_buffers.result, DirectX::XMMatrixIdentity() } };
	CreateTopLevelAS(instances);

	// コマンドリストをフラッシュし、終了するまで待ちます
	cmd_list4->Close();
	ID3D12CommandList* cmd_lists[] = { cmd_list4.Get() };
	queue->Data()->ExecuteCommandLists(1, cmd_lists);
	queue->Signal();
	queue->Wait();

	// コマンドリストの実行が完了したら、それをリセットしてレンダリングに再利用します
	cmd_allocator->Reset();
	cmd_list4->Reset(cmd_allocator.Get(), nullptr);

	// ASバッファーを保管します。 関数を終了すると、残りのバッファが解放されます
	bottom_level_as = bottom_level_buffers.result;
}

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
		t_vert_view.SizeInBytes = sizeof(TriangleVertex) * vertex.size();

		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		renderer_desc.vs_desc.hlsl = "./HLSL/2D/Triangle_vs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/Triangle_ps.hlsl";
		renderer_desc.vs_desc.version = "vs_5_1";
		renderer_desc.ps_desc.version = "ps_5_1";

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
		t_renderer = std::make_shared<KGL::BaseRenderer>(device, renderer_desc);
	}

	{
		// レイトレーシング用の加速構造（AS）をセットアップします。
		// ジオメトリを設定する場合、最下位の各ASには独自の変換行列があります。
		CreateAccelerationStructures(desc.app->GetQueue());
	}

	return hr;
}

HRESULT SceneMain::Init(const SceneDesc& desc)
{
	HRESULT hr = S_OK;

	raster = true;

	return hr;
}

HRESULT SceneMain::Update(const SceneDesc& desc, float elapsed_time)
{
	HRESULT hr = S_OK;

	const auto& input = desc.input;
	
	if (input->IsKeyPressed(KGL::KEYS::SPACE))
	{
		raster = !raster;
	}

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

	cmd_list4->RSSetViewports(1, &viewport);
	cmd_list4->RSSetScissorRects(1, &scissorrect);

	if (raster)
	{
		desc.app->SetRtvDsv(cmd_list4);
		cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtvDsv(cmd_list4, DirectX::XMFLOAT4(0.0f, 0.2f, 0.4f, 1.f));

		cmd_list4->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		t_renderer->SetState(cmd_list4);
		cmd_list4->IASetVertexBuffers(0, 1, &t_vert_view);
		cmd_list4->DrawInstanced(3, 1, 0, 0);

		cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));
	}
	else
	{
		desc.app->SetRtvDsv(cmd_list4);
		cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtvDsv(cmd_list4, DirectX::XMFLOAT4(0.6f, 0.8f, 0.4f, 1.f));

		cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));
	}

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