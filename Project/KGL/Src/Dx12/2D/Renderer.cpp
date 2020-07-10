#include <Dx12/2D/Renderer.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

Renderer::Renderer(
	const ComPtr<ID3D12Device>& device,
	BDTYPE type,
	const Shader::Desc& vs_desc, const Shader::Desc& ps_desc,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layouts,
	const std::vector<D3D12_DESCRIPTOR_RANGE>& add_range,
	const std::vector<D3D12_ROOT_PARAMETER>& add_root_param,
	const std::vector<D3D12_STATIC_SAMPLER_DESC>& add_smp_desc
) noexcept
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipe_desc = {};

	const auto& shader = GetShaderDesc(vs_desc, ps_desc, input_layouts, &gpipe_desc);

	BLEND::SetBlend(type, &gpipe_desc.BlendState);

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
	gpipe_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;			// 0~1に正規化されたRGBA
#endif
	gpipe_desc.SampleDesc.Count = 1;								// サンプリングは１ピクセルにつき１
	gpipe_desc.SampleDesc.Quality = 0;								// クオリティは最低

	gpipe_desc.DepthStencilState.DepthEnable = true;							// 深度バッファを使う
	gpipe_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpipe_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// 小さいほうを書き込む
	gpipe_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//CD3DX12_DESCRIPTOR_RANGE desc_tbl_ranges[1] = {};					// テクスチャと定数の２つ
	std::vector<D3D12_DESCRIPTOR_RANGE> desc_tbl_ranges(1);
	// テクスチャ用
	desc_tbl_ranges[0] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	desc_tbl_ranges.reserve(desc_tbl_ranges.size() + add_range.size());

	std::copy(add_range.cbegin(), add_range.cend(), std::back_inserter(desc_tbl_ranges));

	std::vector<D3D12_ROOT_PARAMETER> root_params(1);
	{
		CD3DX12_ROOT_PARAMETER def_param = {};
		def_param.InitAsDescriptorTable(desc_tbl_ranges.size(), desc_tbl_ranges.data());
		root_params[0] = def_param;
	}
	root_params.reserve(root_params.size() + add_root_param.size());
	std::copy(add_root_param.cbegin(), add_root_param.cend(), std::back_inserter(root_params));

	std::vector<D3D12_STATIC_SAMPLER_DESC> sampler_desc(1);
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

	sampler_desc.reserve(sampler_desc.size() + add_smp_desc.size());
	std::copy(add_smp_desc.cbegin(), add_smp_desc.cend(), std::back_inserter(sampler_desc));

	CD3DX12_ROOT_SIGNATURE_DESC rootsig_desc = {};
	rootsig_desc.Init(
		root_params.size(), root_params.data(),
		sampler_desc.size(), sampler_desc.data(),
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