#include <Dx12/Base/Renderer.hpp>
#include <Dx12/BlendState.hpp>
#include <DirectXTex/d3dx12.h>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

std::unique_ptr<KGL::Shader> BaseRenderer::GetShaderDesc(
	const Shader::Desc& vs_desc, const Shader::Desc& ps_desc,
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layouts,
	D3D12_GRAPHICS_PIPELINE_STATE_DESC* out_desc
) noexcept
{
	RCHECK(!out_desc, "out_desc ‚ª nullptr", {});

	std::unique_ptr<KGL::Shader> shader;

	shader = std::make_unique<KGL::Shader>(
		vs_desc, ps_desc, input_layouts,
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
#ifdef _DEBUG
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
		0
#endif
		);
	shader->GetDesc(out_desc);
	return shader;
}

void BaseRenderer::SetState(const ComPtr<ID3D12GraphicsCommandList>& cmd_list) const noexcept
{
	cmd_list->SetPipelineState(m_pl_state.Get());
	cmd_list->SetGraphicsRootSignature(m_rootsig.Get());
}