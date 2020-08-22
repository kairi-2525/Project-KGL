#include "../Hrd/Scene.hpp"
#include <Helper/Cast.hpp>
#include <DirectXTex/d3dx12.h>
#include <Dx12/Helper.hpp>
#include <Helper/ThrowAssert.hpp>

#include "../DXRHelper/DXRHelper.h"
#include "../DXRHelper/nv_helpers_dx12/BottomLevelASGenerator.h"
#include "../DXRHelper/nv_helpers_dx12/RaytracingPipelineGenerator.h"
#include "../DXRHelper/nv_helpers_dx12/RootSignatureGenerator.h"

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

	bottom_level_as.ComputeASBufferSizes(dxr_device.Get(), false, &scratch_size_in_bytes, &result_size_in_bytes);

	// サイズが取得されると、アプリケーションは必要なバッファを割り当てる必要があります。
	// 生成全体がGPUで行われるため、デフォルトヒープに直接割り当てることができます。
	AccelerationStructureBuffers buffers;
	buffers.scratch.Attach(nv_helpers_dx12::CreateBuffer(dxr_device.Get(), scratch_size_in_bytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
		nv_helpers_dx12::kDefaultHeapProps));
	buffers.result.Attach(nv_helpers_dx12::CreateBuffer(dxr_device.Get(), result_size_in_bytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps));

	// 加速構造を構築します。 この呼び出しは生成されたASのバリアを統合するため、
	// このメソッドの直後にトップレベルのASを計算するために使用できることに注意してください。
	bottom_level_as.Generate(dxr_cmd_list.Get(), buffers.scratch.Get(), buffers.result.Get(), false, nullptr);

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
	top_level_as_generator.ComputeASBufferSizes(dxr_device.Get(), true,
		&scratch_size, &result_size, &instance_descs_size);

	// スクラッチバッファと結果バッファを作成します。
	// ビルドはすべてGPUで行われるため、デフォルトのヒープに割り当てることができます
	top_level_buffers.scratch.Attach(nv_helpers_dx12::CreateBuffer(
		dxr_device.Get(), scratch_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nv_helpers_dx12::kDefaultHeapProps));

	top_level_buffers.result.Attach(nv_helpers_dx12::CreateBuffer(
		dxr_device.Get(), result_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps));

	// インスタンスを説明するバッファー：ID、シェーダーバインディング情報、マトリックス...
	// これらはマッピングを通じてヘルパーによってバッファーにコピーされるため、
	// アップロードヒープにバッファーを割り当てる必要があります。
	top_level_buffers.instance_desc.Attach(nv_helpers_dx12::CreateBuffer(
		dxr_device.Get(), instance_descs_size, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nv_helpers_dx12::kUploadHeapProps));

	// すべてのバッファが割り当てられた後、または更新のみが必要な場合は、加速構造を構築できます。
	// 更新の場合、既存のASを「以前の」ASとしても渡すため、
	// 所定の位置に再フィットできることに注意してください。
	top_level_as_generator.Generate(dxr_cmd_list.Get(),
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
	dxr_cmd_list->Close();
	ID3D12CommandList* cmd_lists[] = { dxr_cmd_list.Get() };
	queue->Data()->ExecuteCommandLists(1, cmd_lists);
	queue->Signal();
	queue->Wait();

	// コマンドリストの実行が完了したら、それをリセットしてレンダリングに再利用します
	cmd_allocator->Reset();
	dxr_cmd_list->Reset(cmd_allocator.Get(), nullptr);

	// ASバッファーを保管します。 関数を終了すると、残りのバッファが解放されます
	bottom_level_as = bottom_level_buffers.result;
}

// 光線生成シェーダーは2つのリソースにアクセスする必要があります：光線追跡出力と最上位の加速構造
KGL::ComPtr<ID3D12RootSignature> SceneMain::CreateRayGenSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter(
		{
			{
				0 /*u0*/, 1 /*1 descriptor*/, 0 /*暗黙のレジスタスペース0を使用する*/,
				D3D12_DESCRIPTOR_RANGE_TYPE_UAV /*出力バッファーを表すUAV*/,
				0 /*UAVが定義されているヒープスロット*/
			},
			{
				0 /*t0*/, 1, 0,
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*トップレベルの加速構造*/,
				1
			}
		}
	);
	KGL::ComPtr<ID3D12RootSignature> rs;
	rs.Attach(rsc.Generate(dxr_device.Get(), true));
	rs->SetName(L"RayGenSignature");
	return rs;
}

// ミスシェーダーはレイペイロードを介してのみ通信するため、リソースを必要としません
KGL::ComPtr<ID3D12RootSignature> SceneMain::CreateMissSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	KGL::ComPtr<ID3D12RootSignature> rs;
	rs.Attach(rsc.Generate(dxr_device.Get(), true));
	rs->SetName(L"MissSignature");
	return rs;
}

// ヒットシェーダーはレイペイロードを介してのみ通信するため、リソースを必要としません
KGL::ComPtr<ID3D12RootSignature> SceneMain::CreateHitSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV);
	KGL::ComPtr<ID3D12RootSignature> rs;
	rs.Attach(rsc.Generate(dxr_device.Get(), true));
	rs->SetName(L"HitSignature");
	return rs;
}


// レイトレーシングパイプラインは、シェーダーコード、ルートシグネチャ、パイプライン特性を、
// DXRがシェーダーを呼び出し、レイトレーシング中に一時メモリを管理するために使用する単一の構造にバインドします。
void SceneMain::CreateRaytracingPipeline()
{
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(dxr_device.Get());

	// パイプラインには、レイトレーシングプロセス中に潜在的に実行されるすべてのシェーダーのDXILコードが含まれています。
	// このセクションでは、HLSLコードを一連のDXILライブラリにコンパイルします。
	// 明確にするために、いくつかのライブラリのコードをセマンティック
	//（光線生成、ヒット、ミス）で分離することを選択しました。 任意のコードレイアウトを使用できます。
	ray_gen_library.Attach(nv_helpers_dx12::CompileShaderLibrary(L"./HLSL/DXR/RayGen.hlsl"));
	miss_library.Attach(nv_helpers_dx12::CompileShaderLibrary(L"./HLSL/DXR/Miss.hlsl"));
	hit_library.Attach(nv_helpers_dx12::CompileShaderLibrary(L"./HLSL/DXR/Hit.hlsl"));

	// DLLと同様に、各ライブラリはエクスポートされた多数のシンボルに関連付けられています。
	// これは、以下の行で明示的に行う必要があります。 
	// 単一のライブラリには任意の数のシンボルを含めることができ、
	// その意味は[shader（ "xxx"）]構文を使用してHLSLで指定されます。
	pipeline.AddLibrary(ray_gen_library.Get(), { L"RayGen" });
	pipeline.AddLibrary(miss_library.Get(), { L"Miss" });
	pipeline.AddLibrary(hit_library.Get(), { L"ClosestHit" });

	// 各DX12シェーダーを使用するには、アクセスするパラメーターとバッファーを定義するルート署名が必要です。
	ray_gen_signature = CreateRayGenSignature();
	miss_signature = CreateMissSignature();
	hit_signature = CreateHitSignature();

	// 3つの異なるシェーダーを呼び出して交差を取得できます。
	// 交差しないシェーダーは、非三角形ジオメトリのバウンディングボックスを押すと呼び出されます。
	// これは、このチュートリアルの範囲を超えています。 
	// ヒットの可能性があるシェーダーは、潜在的な交差で呼び出されます。 
	// このシェーダーは、たとえば、アルファテストを実行して一部の交差を破棄できます。
	// 最後に、最も近いヒットのプログラムは、レイの原点に最も近い交点で呼び出されます。
	// これら3つのシェーダーは1つのヒットグループにまとめられます。

	// 三角形のジオメトリの場合、交差シェーダーが組み込まれていることに注意してください。
	// 空の任意ヒットシェーダーもデフォルトで定義されているため、
	// 単純なケースでは、各ヒットグループには最も近いヒットシェーダーのみが含まれています。
	// エクスポートされたシンボルは上記で定義されているため、シェーダーは名前で簡単に参照できます。

	// シェーダーが頂点カラーを補間するだけで、三角形のヒットグループ
	pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");

	// 次のセクションでは、ルートシグネチャを各シェーダーに関連付けます。
	// 一部のシェーダーが同じルート署名を共有していることを明示的に示すことができることに注意してください
	//（例：MissとShadowMiss）。 
	// ヒットシェーダーは現在、ヒットグループとのみ呼ばれることに注意してください。
	// つまり、基になる交差、any-hit、およびnearest-hitシェーダーは同じルートシグネチャを共有します。
	pipeline.AddRootSignatureAssociation(ray_gen_signature.Get(), { L"RayGen" });
	pipeline.AddRootSignatureAssociation(miss_signature.Get(), { L"Miss" });
	pipeline.AddRootSignatureAssociation(hit_signature.Get(), { L"HitGroup" });

	// ペイロードサイズは、レイによって運ばれるデータの最大サイズを定義します。
	// HLSLコードのHitInfo構造など、シェーダー間で交換されるデータ。
	// 値が高すぎると、不必要なメモリ消費とキャッシュトラッシングが発生するため、
	// この値をできるだけ低く保つことが重要です。
	pipeline.SetMaxPayloadSize(4 * sizeof(float)); // RGB + distance

	// DXRは、サーフェスに当たると、そのヒットにいくつかの属性を提供できます。
	// サンプルでは、三角形の最後の2つの頂点の重みu、vで定義された重心座標を使用しています。
	// 実際の重心は、float3 barycentrics = float3（1.f-u-v、u、v）;を使用して取得できます。
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // 重心座標

	// レイトレーシングプロセスでは、既存のヒットポイントからレイを放つことができるため、
	// TraceRay呼び出しがネストされます。
	// サンプルコードは1次光線のみをトレースするため、トレース深度1が必要です。
	// 最高のパフォーマンスを得るには、この再帰深度を最小限に抑える必要があります。
	// パストレーシングアルゴリズムは、光線生成で単純なループに簡単にフラット化できます。
	pipeline.SetMaxRecursionDepth(1);

	// GPUで実行するためにパイプラインをコンパイルする
	rt_state_object.Attach(pipeline.Generate());

	// ステートオブジェクトをプロパティオブジェクトにキャストし、
	// 後で名前でシェーダーポインターにアクセスできるようにします
	HRESULT hr = rt_state_object->QueryInterface(IID_PPV_ARGS(rt_state_object_props.GetAddressOf()));
	RCHECK(FAILED(hr), "rt_state_object->QueryInterfaceに失敗");
	
}

// レイトレーシング出力を保持するバッファを、出力画像と同じサイズで割り当てます
void SceneMain::CreateRaytracingOutputBuffer(const DirectX::XMUINT2& screen_size)
{
	D3D12_RESOURCE_DESC res_desc = {};
	res_desc.DepthOrArraySize = 1;
	res_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	// バックバッファーは実際にはDXGI_FORMAT_R8G8B8A8_UNORM_SRGBですが、
	// sRGB形式はUAVで使用できません。 
	// 正確さのために、シェーダーで自分自身をsRGBに変換する必要があります
	res_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	res_desc.Width = screen_size.x;
	res_desc.Height = screen_size.y;
	res_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	res_desc.MipLevels = 1;
	res_desc.SampleDesc.Count = 1;

	HRESULT hr = dxr_device->CreateCommittedResource(
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &res_desc,
		D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
		IID_PPV_ARGS(output_resource.GetAddressOf())
	);
	RCHECK(FAILED(hr), "RaytracingOutputBufferのCreateCommittedResourceに失敗");


}

// シェーダーが使用するメインヒープを作成します。
// これにより、レイトレーシング出力と最上位の加速構造にアクセスできます。
void SceneMain::CreateShaderResourceHeap()
{
	// SRV / UAV / CBV記述子ヒープを作成します。 
	// 2つのエントリが必要です - レイトレーシング出力用の1 UAVとTLAS用の1 SRV
	srv_uav_heap.Attach(
		nv_helpers_dx12::CreateDescriptorHeap(dxr_device.Get(), 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true)
	);

	// 記述子を直接書き込むことができるように、CPU側のヒープメモリへのハンドルを取得します。
	D3D12_CPU_DESCRIPTOR_HANDLE srv_handle =
		srv_uav_heap->GetCPUDescriptorHandleForHeapStart();

	// UAVを作成します。 作成したルート署名に基づいて、
	// 最初のエントリです。 Create * Viewメソッドは、ビュー情報をsrvHandleに直接書き込みます
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	dxr_device->CreateUnorderedAccessView(output_resource.Get(), nullptr, &uav_desc,
		srv_handle);

	// レイトレーシング出力バッファーの直後にトップレベルAS SRVを追加する
	srv_handle.ptr += dxr_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc;
	srv_desc.Format = DXGI_FORMAT_UNKNOWN;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.RaytracingAccelerationStructure.Location =
		top_level_buffers.result->GetGPUVirtualAddress();
	// ヒープにアクセラレーション構造ビューを書き込む
	dxr_device->CreateShaderResourceView(nullptr, &srv_desc, srv_handle);
}

// シェーダーバインディングテーブル（SBT）は、レイトレーシングセットアップの基礎です。
// これは、GPU上のレイトレーサーによって解釈できるように、
// シェーダーリソースがシェーダーにバインドされる場所です。 
// レイアウトに関しては、SBTには一連のシェーダーIDとそのリソースポインターが含まれています。
// SBTには、光線生成シェーダー、ミスシェーダー、ヒットグループが含まれます。 
// ヘルパークラスを使用すると、それらを任意の順序で指定できます。
void SceneMain::CreateShaderBindingTable()
{
	// SBTヘルパークラスはAdd * Programの呼び出しを収集します。
	// 複数回呼び出される場合は、シェーダーを再度追加する前にヘルパーを空にする必要があります。
	sbt_helper.Reset();

	// ヒープの先頭へのポインターは、ルートパラメーターのないシェーダーで必要な唯一のパラメーターです
	D3D12_GPU_DESCRIPTOR_HANDLE srv_uav_heap_handle =
		srv_uav_heap->GetGPUDescriptorHandleForHeapStart();

	// ヘルパーはルートパラメータポインタとヒープポインタの両方をvoid *として扱いますが、
	// DX12はD3D12_GPU_DESCRIPTOR_HANDLEを使用してヒープポインタを定義します。
	// この構造体のポインターはUINT64で、ポインターとして再解釈する必要があります。
	auto heap_pointer = RCAST<UINT64*>(srv_uav_heap_handle.ptr);

	// 光線生成はヒープデータのみを使用します
	sbt_helper.AddRayGenerationProgram(L"RayGen", { (void*)heap_pointer });

	// ミスシェーダーとヒットシェーダーは外部リソースにアクセスせず、
	// 代わりにレイペイロードを通じて結果を伝達します。
	sbt_helper.AddMissProgram(L"Miss", {});	// カメラレイ用とシャドウレイ用のミスシェーダーがあるため
	sbt_helper.AddMissProgram(L"Miss", {});

	// トライアングルヒットシェーダーを追加する
	sbt_helper.AddHitGroup(L"HitGroup", { (void*)(t_vert_res->Data()->GetGPUVirtualAddress()) });

	// シェーダーとそのパラメーターの数を指定して、SBTのサイズを計算します。
	const UINT32 sbt_size = sbt_helper.ComputeSBTSize();

	// アップロードヒープにSBTを作成します。 
	// これは、ヘルパーがマッピングを使用してSBTコンテンツを書き込むために必要です。 
	// SBTのコンパイル後、パフォーマンス向上のためにデフォルトのヒープにコピーできます。
	sbt_storage.Attach(nv_helpers_dx12::CreateBuffer(
		dxr_device.Get(), sbt_size, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps
	));

	RCHECK(!sbt_storage, "シェーダーバインディングテーブルを割り当てられませんでした");

	// シェーダーとパラメーター情報からSBTをコンパイルする
	sbt_helper.Generate(sbt_storage.Get(), rt_state_object_props.Get());
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

	{
		// レイトレーシング用の加速構造（AS）をセットアップします。
		// ジオメトリを設定する場合、最下位の各ASには独自の変換行列があります。
		CreateAccelerationStructures(desc.app->GetQueue());

		// レイトレーシングパイプラインを作成し、
		// シェーダーコードをシンボル名とそのルートシグネチャに関連付け、
		// レイが運ぶメモリの量を定義します（レイペイロード）
		CreateRaytracingPipeline();

		// レイトレーシング出力を格納するバッファを、ターゲットイメージと同じサイズで割り当てます
		CreateRaytracingOutputBuffer(desc.window->GetClientSize());

		// レイトレーシングの結果を含むバッファーを作成し（常にUAVに出力）、
		// レイトレーシングで使用されるリソース（加速構造など）を参照するヒープを作成します。
		CreateShaderResourceHeap();

		// シェーダーバインディングテーブルを作成し、
		// ASのインスタンスごとに呼び出されるシェーダーを示します。
		CreateShaderBindingTable();
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
	
	if (input->IsKeyPressed(KGL::KEYS::ENTER))
	{
		SetNextScene<SceneOriginal>(desc);
	}
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
		dxr_cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		// 記述子ヒープをバインドして、トップレベルのアクセラレーション構造とレイトレーシング出力へのアクセスを提供します
		std::vector<ID3D12DescriptorHeap*> heaps = { srv_uav_heap.Get() };
		dxr_cmd_list->SetDescriptorHeaps(SCAST<UINT>(heaps.size()), heaps.data());

		// 最後のフレームでは、レイトレーシング出力がコピーソースとして使用され、
		// その内容がレンダーターゲットにコピーされました。 
		// 次に、シェーダーがUAVに書き込むことができるように、UAVに移行する必要があります。
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			output_resource.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		dxr_cmd_list->ResourceBarrier(1, &transition);

		// レイトレーシングタスクを設定します
		{
			D3D12_DISPATCH_RAYS_DESC ray_desc = {};
			// SBTのレイアウトは、光線生成シェーダー、ミスシェーダー、ヒットグループです。
			// CreateShaderBindingTableメソッドで説明されているように、
			// 特定のタイプのすべてのSBTエントリは、固定ストライドを可能にするために同じサイズを持っています。

			// 光線生成シェーダーは常にSBTの先頭にあります。
			D3D12_GPU_VIRTUAL_ADDRESS start_address = sbt_storage->GetGPUVirtualAddress();
			const UINT32 ray_generation_section_size_in_bytes = sbt_helper.GetRayGenSectionSize();
			ray_desc.RayGenerationShaderRecord.StartAddress = start_address;
			ray_desc.RayGenerationShaderRecord.SizeInBytes = ray_generation_section_size_in_bytes;

			start_address += ray_generation_section_size_in_bytes;

			// ミスシェーダーは、光線生成シェーダーの直後の2番目のSBTセクションにあります。 
			// カメラレイ用とシャドウレイ用の1つのミスシェーダーがあるため、
			// このセクションのサイズは2 * m_sbtEntrySizeです。 
			// 2つのミスシェーダー間のストライドも示します。これは、SBTエントリのサイズです。
			const UINT32 miss_section_size_in_bytes = sbt_helper.GetMissSectionSize();
			ray_desc.MissShaderTable.StartAddress = start_address;
			ray_desc.MissShaderTable.SizeInBytes = miss_section_size_in_bytes;
			ray_desc.MissShaderTable.StrideInBytes = sbt_helper.GetMissEntrySize();

			start_address += miss_section_size_in_bytes;

			// ヒットグループセクションは、ミスシェーダーの後に始まります。
			// このサンプルでは、三角形に1つの1ヒットグループがあります。
			const UINT32 hit_groups_section_size = sbt_helper.GetHitGroupSectionSize();
			ray_desc.HitGroupTable.StartAddress = start_address;
			ray_desc.HitGroupTable.SizeInBytes = hit_groups_section_size;
			ray_desc.HitGroupTable.StrideInBytes = sbt_helper.GetHitGroupEntrySize();

			// カーネル起動寸法と同じ、レンダリングする画像の寸法
			ray_desc.Width = window_size.x;
			ray_desc.Height = window_size.y;
			ray_desc.Depth = 1;

			// レイトレーシングパイプラインをバインドする
			dxr_cmd_list->SetPipelineState1(rt_state_object.Get());
			// 光線をディスパッチし、光線追跡出力に書き込みます
			dxr_cmd_list->DispatchRays(&ray_desc);
		}

		// レイトレーシング出力は、表示に使用される実際のレンダーターゲットにコピーする必要があります。 
		// そのためには、レイトレーシング出力をUAVからコピーソースに、
		// レンダーターゲットバッファーをコピー先に移行する必要があります。 
		// その後、レンダーターゲットバッファーをレンダーターゲットに移行する前に、
		// 実際のコピーを実行できます。これを使用して、イメージを表示します。
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			output_resource.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE
		);
		dxr_cmd_list->ResourceBarrier(1, &transition);
		const auto& back_buffer = desc.app->GetRtvBuffers().at(desc.app->GetSwapchain()->GetCurrentBackBufferIndex());
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			back_buffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_DEST
		);
		dxr_cmd_list->ResourceBarrier(1, &transition);

		dxr_cmd_list->CopyResource(back_buffer.Get(), output_resource.Get());
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			back_buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		dxr_cmd_list->ResourceBarrier(1, &transition);
		dxr_cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));
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

HRESULT SceneMain::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	HRESULT hr = S_OK;
	return hr;
}