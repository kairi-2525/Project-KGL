#pragma once

#include "../Shader.hpp"
#include "../../Helper/ComPtr.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class BaseRenderer
		{
		protected:
			ComPtr<ID3D12PipelineState>	m_pl_state;
			ComPtr<ID3D12RootSignature>	m_rootsig;
		protected:
			[[nodiscard]] std::unique_ptr<KGL::Shader> GetShaderDesc(
				const Shader::Desc& vs_desc, const Shader::Desc& ps_desc,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layouts,
				D3D12_GRAPHICS_PIPELINE_STATE_DESC* out_desc
			) noexcept;
		public:
			BaseRenderer() = default;
			virtual ~BaseRenderer() = default;
			void SetState(const ComPtr<ID3D12GraphicsCommandList>& cmd_list) const noexcept;
		};
	}
}