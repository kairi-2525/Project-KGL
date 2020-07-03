#include "../Hrd/SceneGame.hpp"
#include <DirectXTex/d3dx12.h>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>

HRESULT SceneGame::Load(const SceneDesc& desc)
{
	HRESULT hr;

	const auto& device = desc.app->GetDevice();
	texture = std::make_shared<KGL::Texture>(device, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff);
	pmd_data = std::make_shared<KGL::PMD_Loader>("./Assets/Models/鏡音リン.pmd");
	pmd_model = std::make_shared<KGL::PMD_Model>(device, pmd_data->GetDesc());

	const std::vector<D3D12_INPUT_ELEMENT_DESC> input_desc =
	{
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"BONE_NO", 0, DXGI_FORMAT_R16G16_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipe_desc = {};
	{
		KGL::Shader::Desc vs_desc, ps_desc;
		vs_desc.hlsl = "./HLSL/PMDVertexShader.hlsl";
		vs_desc.entry_point = "BasicVS";
		vs_desc.version = "vs_5_0";

		ps_desc.hlsl = "./HLSL/PMDPixelShader.hlsl";
		ps_desc.entry_point = "BasicPS";
		ps_desc.version = "ps_5_0";

		shader = std::make_shared<KGL::Shader>(
			vs_desc, ps_desc, input_desc,
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
#ifdef _DEBUG
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
			0
#endif
			);
	}

	shader->GetDesc(&gpipe_desc);

	gpipe_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// アンチエイリアス
	gpipe_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipe_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;		// カリングしない

	KGL::BLEND::SetBlend(KGL::BDTYPE::DEFAULT, &gpipe_desc.BlendState);

	gpipe_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpipe_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipe_desc.NumRenderTargets = 1;
#ifdef USE_SRGB
	gpipe_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;		// 0~1に正規化されたSRGBA
#else
	gpipe_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;			// 0~1に正規化されたRGBA
#endif
	gpipe_desc.SampleDesc.Count = 1;								// サンプリングは１ピクセルにつき１
	gpipe_desc.SampleDesc.Quality = 0;								// クオリティは最低

	gpipe_desc.DepthStencilState.DepthEnable = true;							// 深度バッファを使う
	gpipe_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpipe_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// 小さいほうを書き込む
	gpipe_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	CD3DX12_DESCRIPTOR_RANGE desc_tbl_ranges[4] = {};					// テクスチャと定数の２つ
		// テクスチャ用レジスター0
	desc_tbl_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// 定数用
	desc_tbl_ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	// マテリアル定数用
	desc_tbl_ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	// テクスチャ4つ レジスター1から
	desc_tbl_ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1);

	CD3DX12_ROOT_PARAMETER root_params[2] = {};
	root_params[0].InitAsDescriptorTable(2, &desc_tbl_ranges[0]);

	root_params[1].InitAsDescriptorTable(2, &desc_tbl_ranges[2]);

	CD3DX12_STATIC_SAMPLER_DESC sampler_desc[2] = {};
	sampler_desc[0].Init(0);
	sampler_desc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

	sampler_desc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler_desc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;	// 線形補間
	sampler_desc[0].MaxLOD = D3D12_FLOAT32_MAX;
	sampler_desc[0].MinLOD = 0.0f;
	sampler_desc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	sampler_desc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // リサンプリングしない
	sampler_desc[0].ShaderRegister = 0;

	sampler_desc[1].Init(1,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	CD3DX12_ROOT_SIGNATURE_DESC rootsig_desc = {};
	rootsig_desc.Init(
		_countof(root_params), root_params,
		_countof(sampler_desc), sampler_desc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	KGL::ComPtr<ID3D10Blob> rootsig_blob, error_blob;
	hr = D3D12SerializeRootSignature(
		&rootsig_desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		rootsig_blob.GetAddressOf(),
		error_blob.GetAddressOf()
	);
	RCHECK(FAILED(hr), "D3D12SerializeRootSignatureに失敗", hr);

	hr = device->CreateRootSignature(
		0,	// nodemask
		rootsig_blob->GetBufferPointer(),
		rootsig_blob->GetBufferSize(),
		IID_PPV_ARGS(rootsig.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateRootSignatureに失敗", hr);

	gpipe_desc.pRootSignature = rootsig.Get();

	hr = device->CreateGraphicsPipelineState(&gpipe_desc, IID_PPV_ARGS(pl_state.ReleaseAndGetAddressOf()));
	RCHECK(FAILED(hr), "CreateGraphicsPipelineStateに失敗", hr);

	// マテリアルヒープの作成
	{
		D3D12_DESCRIPTOR_HEAP_DESC mat_heap_desc = {};
		mat_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		mat_heap_desc.NodeMask = 0;
		mat_heap_desc.NumDescriptors = pmd_model->GetMaterialCount() * 5; // マテリアル + SRV + スフィアマップ用SRV x2 + ToonSRV
		mat_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		hr = device->CreateDescriptorHeap(
			&mat_heap_desc, IID_PPV_ARGS(material_heap.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateDescriptorHeapに失敗", hr);

		hr = pmd_model->HeapSet(device, material_heap->GetCPUDescriptorHandleForHeapStart());
		RCHECK(FAILED(hr), "HeapSetに失敗", hr);
	}

	// シーンのコンスタントバッファ
	{
		// 定数バッファの作成
		hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(const_buff.ReleaseAndGetAddressOf())
		);
		RCHECK(FAILED(hr), "CreateCommittedResourceに失敗", hr);
		hr = const_buff->Map(0, nullptr, (void**)&map_buffer);
		RCHECK(FAILED(hr), "const_buff->Mapに失敗", hr);

		D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc = {};
		// シェーダーから見えるように
		desc_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc_heap_desc.NodeMask = 0;
		// CBV
		desc_heap_desc.NumDescriptors = 1;
		// シェーダーリソースビュー用
		desc_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		hr = device->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(basic_desc_heap.ReleaseAndGetAddressOf()));
		RCHECK(FAILED(hr), "CreateDescriptorHeapに失敗", hr);
		D3D12_CPU_DESCRIPTOR_HANDLE basic_heap_handle(basic_desc_heap->GetCPUDescriptorHandleForHeapStart());
		
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = const_buff->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = const_buff->GetDesc().Width;
		device->CreateConstantBufferView(&cbv_desc, basic_heap_handle);
	}

	return hr;
}

HRESULT SceneGame::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();

	camera.eye = { 0.f, 10.f, -15.f };
	camera.focus_vec = { 0.f, 0.f, 15.f };
	camera.up = { 0.f, 1.f, 0.f };

	map_buffer->eye = camera.eye;
	map_buffer->view = KGL::CAMERA::GetView(camera);
	map_buffer->proj = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(window_size.x) / static_cast<float>(window_size.y),	// アスペクト比
		1.0f, 100.0f // near, far
	);
	return S_OK;
}

HRESULT SceneGame::Update(const SceneDesc& desc)
{
	using namespace DirectX;
	static float s_angle = 0.f;
	map_buffer->world = XMMatrixRotationY(s_angle);
	map_buffer->wvp = map_buffer->world * map_buffer->view * map_buffer->proj;
	return S_OK;
}

HRESULT SceneGame::Render(const SceneDesc& desc)
{
	using KGL::SCAST;
	auto cmd_list = desc.cmd_list;

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

	cmd_list->RSSetViewports(1, &viewport);
	cmd_list->RSSetScissorRects(1, &scissorrect);

	cmd_list->SetPipelineState(pl_state.Get());
	cmd_list->SetGraphicsRootSignature(rootsig.Get());

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	{	// シーンコンスタントバッファ
		cmd_list->SetDescriptorHeaps(1, basic_desc_heap.GetAddressOf());
		auto heap_handle = basic_desc_heap->GetGPUDescriptorHandleForHeapStart();
		cmd_list->SetGraphicsRootDescriptorTable(
			0,	// ルートパラメーターインデックス
			heap_handle
		);
	}

	cmd_list->SetDescriptorHeaps(1, material_heap.GetAddressOf());
	auto hr = pmd_model->Render(
		desc.app->GetDevice(),
		cmd_list,
		material_heap->GetGPUDescriptorHandleForHeapStart()
	);
	RCHECK(FAILED(hr), "pmd_model->Renderに失敗", hr);

	return hr;
}

HRESULT SceneGame::UnInit(const SceneDesc& desc)
{
	return S_OK;
}