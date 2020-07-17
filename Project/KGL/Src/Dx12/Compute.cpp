#include <Dx12/Compute.hpp>
#include <DirectXTex/d3dx12.h>

using namespace KGL;

ComputePipline::ComputePipline(ComPtrC<ID3D12Device> device, const Desc& desc) noexcept
{
	CD3DX12_ROOT_SIGNATURE_DESC root_desc = {};
	root_desc.Init(
		desc.root_params.size(), desc.root_params.data(),
		desc.static_samplars.size(), desc.static_samplars.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);


	D3D12_COMPUTE_PIPELINE_STATE_DESC pipe_desc{};

	ShaderCS shader(desc.cs_desc, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
#ifdef _DEBUG
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
		0
#endif
	);
	shader.GetDesc(&pipe_desc);

	pipe_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	pipe_desc.NodeMask = 0;
	pipe_desc.pRootSignature = m_rootsig.Get();

	auto hr = device->CreateComputePipelineState(&pipe_desc, IID_PPV_ARGS(&m_pl_state));
	return;
}