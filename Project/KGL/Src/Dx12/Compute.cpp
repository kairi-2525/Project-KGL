#include <Dx12/Compute.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

ComputePipline::ComputePipline(
	ComPtrC<ID3D12Device> device,
	const std::shared_ptr<DXC> dxc,
	const Desc& desc) noexcept
{
	CD3DX12_ROOT_SIGNATURE_DESC root_desc = {};
	root_desc.Init(
		SCAST<UINT>(desc.root_params.size()), desc.root_params.data(),
		SCAST<UINT>(desc.static_samplars.size()), desc.static_samplars.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	KGL::ComPtr<ID3D10Blob> rootsig_blob, error_blob;
	auto hr = D3D12SerializeRootSignature(
		&root_desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		rootsig_blob.GetAddressOf(),
		error_blob.GetAddressOf()
	);
	RCHECK(FAILED(hr), "D3D12SerializeRootSignature‚ÉŽ¸”s");

	hr = device->CreateRootSignature(
		0,	// nodemask
		rootsig_blob->GetBufferPointer(),
		rootsig_blob->GetBufferSize(),
		IID_PPV_ARGS(m_rootsig.ReleaseAndGetAddressOf())
	);
	RCHECK(FAILED(hr), "CreateRootSignature‚ÉŽ¸”s");


	D3D12_COMPUTE_PIPELINE_STATE_DESC pipe_desc{};

	ShaderCS shader(dxc, desc.cs_desc/*, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
#ifdef _DEBUG
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
		0
#endif*/
	);
	shader.GetDesc(&pipe_desc);

	pipe_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	pipe_desc.NodeMask = 0;
	pipe_desc.pRootSignature = m_rootsig.Get();

	hr = device->CreateComputePipelineState(&pipe_desc, IID_PPV_ARGS(&m_pl_state));
	return;
}

void ComputePipline::SetState(const ComPtr<ID3D12GraphicsCommandList>& cmd_list) const noexcept
{
	cmd_list->SetComputeRootSignature(m_rootsig.Get());
	cmd_list->SetPipelineState(m_pl_state.Get());
}