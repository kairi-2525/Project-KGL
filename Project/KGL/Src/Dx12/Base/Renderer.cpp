#include <Dx12/Base/Renderer.hpp>
#include <Dx12/BlendState.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>
#include <Dx12/SetName.hpp>

using namespace KGL;

HRESULT BaseRenderer::Create(
	const ComPtr<ID3D12Device>& device,
	const std::shared_ptr<DXC>& dxc,
	const Desc& desc
) noexcept
{
	HRESULT hr = S_OK;

	CD3DX12_ROOT_SIGNATURE_DESC rootsig_desc = {};
	rootsig_desc.Init(
		SCAST<UINT>(desc.root_params.size()), desc.root_params.data(),
		SCAST<UINT>(desc.static_samplers.size()), desc.static_samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	KGL::ComPtr<ID3D10Blob> rootsig_blob, error_blob;
	hr = D3D12SerializeRootSignature(
		&rootsig_desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		rootsig_blob.GetAddressOf(),
		error_blob.GetAddressOf()
	);
	RCHECK_HR(hr, "D3D12SerializeRootSignature‚ÉŽ¸”s");
	hr = device->CreateRootSignature(
		0,	// nodemask
		rootsig_blob->GetBufferPointer(),
		rootsig_blob->GetBufferSize(),
		IID_PPV_ARGS(m_rootsig.ReleaseAndGetAddressOf())
	);
	RCHECK_HR(hr, "CreateRootSignature‚ÉŽ¸”s");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipe_desc = {};
	const auto& shader = GetShaderDesc(dxc, desc.vs_desc, desc.ps_desc, desc.ds_desc, desc.hs_desc, desc.gs_desc, desc.input_layouts, &gpipe_desc);
	BLEND::SetBlend(desc.blend_types, &gpipe_desc.BlendState);

	gpipe_desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// ƒAƒ“ƒ`ƒGƒCƒŠƒAƒX
	gpipe_desc.RasterizerState = desc.rastarizer_desc;
	gpipe_desc.IBStripCutValue = desc.other_desc.index_cut_value;

	gpipe_desc.PrimitiveTopologyType = desc.other_desc.topology_type;

	gpipe_desc.NumRenderTargets = 1;
	gpipe_desc.NumRenderTargets = (std::min)(SCAST<UINT>(desc.render_targets.size()), 8u);
	for (UINT i = 0u; i < gpipe_desc.NumRenderTargets; i++)
	{
		gpipe_desc.RTVFormats[i] = desc.render_targets[i];
	}
	gpipe_desc.SampleDesc = desc.other_desc.sample_desc;

	gpipe_desc.DepthStencilState = desc.depth_desc;
	gpipe_desc.DSVFormat = desc.other_desc.dsv_format;

	gpipe_desc.pRootSignature = m_rootsig.Get();



	hr = device->CreateGraphicsPipelineState(&gpipe_desc, IID_PPV_ARGS(m_pl_state.ReleaseAndGetAddressOf()));
	RCHECK_HR(hr, "CreateGraphicsPipelineState‚ÉŽ¸”s");

	return hr;
}

std::unique_ptr<KGL::Shader> BaseRenderer::GetShaderDesc(
	const std::shared_ptr<DXC>& dxc,
	const SHADER::Desc& vs_desc, const SHADER::Desc& ps_desc,
	const SHADER::Desc& ds_desc, const SHADER::Desc& hs_desc, const SHADER::Desc& gs_desc,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layouts,
	D3D12_GRAPHICS_PIPELINE_STATE_DESC* out_desc
) noexcept
{
	RCHECK(!out_desc, "out_desc ‚ª nullptr", {});

	std::unique_ptr<KGL::Shader> shader;

	shader = std::make_unique<KGL::Shader>(
		dxc,
		vs_desc, ps_desc, ds_desc, hs_desc, gs_desc,
		input_layouts/*,
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
#ifdef _DEBUG
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
		0
#endif*/
		);
	shader->GetDesc(out_desc);
	return shader;
}

void BaseRenderer::SetState(const ComPtr<ID3D12GraphicsCommandList>& cmd_list) const noexcept
{
	cmd_list->SetPipelineState(m_pl_state.Get());
	cmd_list->SetGraphicsRootSignature(m_rootsig.Get());
}

void BaseRenderer::SetName(const std::filesystem::path& name) const noexcept
{
	DX12::SetName<ID3D12PipelineState>(m_pl_state, RCAST<INT_PTR>(this), name.wstring());
	DX12::SetName<ID3D12RootSignature>(m_rootsig, RCAST<INT_PTR>(this), name.wstring());
}