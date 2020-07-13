#include <Dx12/3D/PMDRenderer.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

#include <vector>

using namespace KGL;

PMD_Renderer::PMD_Renderer(
	const ComPtr<ID3D12Device>& device,
	const Desc& desc
) noexcept
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipe_desc = {};

	const auto& shader = GetShaderDesc(desc.vs_desc, desc.ps_desc, desc.input_layouts, &gpipe_desc);

	BLEND::SetBlend(desc.blend_type, &gpipe_desc.BlendState);

	gpipe_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// アンチエイリアス
	gpipe_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipe_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;		// カリングしない
	gpipe_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	gpipe_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipe_desc.NumRenderTargets = 1;
#ifdef USE_SRGB
	gpipe_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;		// 0~1に正規化されたSRGBA
#else
	gpipe_desc.NumRenderTargets = (std::min)(SCAST<UINT>(desc.render_targets.size()), 8u);
	for (UINT i = 0u; i < gpipe_desc.NumRenderTargets; i++)
	{
		gpipe_desc.RTVFormats[i] = desc.render_targets[i];
	}
#endif
	gpipe_desc.SampleDesc.Count = 1;								// サンプリングは１ピクセルにつき１
	gpipe_desc.SampleDesc.Quality = 0;								// クオリティは最低

	gpipe_desc.DepthStencilState.DepthEnable = true;							// 深度バッファを使う
	gpipe_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpipe_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// 小さいほうを書き込む
	gpipe_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	CD3DX12_DESCRIPTOR_RANGE desc_tbl_ranges[3] = {};					// テクスチャと定数の２つ

	// 定数用
	std::vector<D3D12_DESCRIPTOR_RANGE> scene_tbl_range(1);
	scene_tbl_range[0] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	scene_tbl_range.reserve(scene_tbl_range.size() + desc.add_range.size());
	std::copy(desc.add_range.cbegin(), desc.add_range.cend(), std::back_inserter(scene_tbl_range));

	desc_tbl_ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	// マテリアル定数用
	desc_tbl_ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	// テクスチャ4つ レジスター0から
	desc_tbl_ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	std::vector<D3D12_ROOT_PARAMETER> root_params(3);
	{
		CD3DX12_ROOT_PARAMETER def_param[3];
		def_param[0].InitAsDescriptorTable(SCAST<UINT>(scene_tbl_range.size()), scene_tbl_range.data());
		def_param[1].InitAsDescriptorTable(1, &desc_tbl_ranges[0]);
		def_param[2].InitAsDescriptorTable(2, &desc_tbl_ranges[1]);

		root_params[0] = def_param[0];
		root_params[1] = def_param[1];
		root_params[2] = def_param[2];
	}
	root_params.reserve(root_params.size() + desc.add_root_param.size());
	std::copy(desc.add_root_param.cbegin(), desc.add_root_param.cend(), std::back_inserter(root_params));

	std::vector<D3D12_STATIC_SAMPLER_DESC> sampler_desc(2);
	//CD3DX12_STATIC_SAMPLER_DESC sampler_desc[2] = {};
	sampler_desc[0] = CD3DX12_STATIC_SAMPLER_DESC(0);
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

	sampler_desc[1] = CD3DX12_STATIC_SAMPLER_DESC(1,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);
	sampler_desc.reserve(sampler_desc.size() + desc.add_smp_desc.size());
	std::copy(desc.add_smp_desc.cbegin(), desc.add_smp_desc.cend(), std::back_inserter(sampler_desc));

	CD3DX12_ROOT_SIGNATURE_DESC rootsig_desc = {};
	rootsig_desc.Init(
		SCAST<UINT>(root_params.size()), root_params.data(),
		SCAST<UINT>(sampler_desc.size()), sampler_desc.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	KGL::ComPtr<ID3D10Blob> rootsig_blob, error_blob;
	auto hr = D3D12SerializeRootSignature(
		&rootsig_desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		rootsig_blob.GetAddressOf(),
		error_blob.GetAddressOf()
	);
	RCHECK(FAILED(hr), "D3D12SerializeRootSignatureに失敗");

	hr = device->CreateRootSignature(
		0,	// nodemask
		rootsig_blob->GetBufferPointer(),
		rootsig_blob->GetBufferSize(),
		IID_PPV_ARGS(m_rootsig.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateRootSignatureに失敗");

	gpipe_desc.pRootSignature = m_rootsig.Get();

	hr = device->CreateGraphicsPipelineState(&gpipe_desc, IID_PPV_ARGS(m_pl_state.ReleaseAndGetAddressOf()));
	RCHECK(FAILED(hr), "CreateGraphicsPipelineStateに失敗");
}