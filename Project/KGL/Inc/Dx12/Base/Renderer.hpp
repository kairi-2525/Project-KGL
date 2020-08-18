#pragma once

#include "../Shader.hpp"
#include "../../Helper/ComPtr.hpp"
#include "../BlendState.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class BaseRenderer
		{
		public:
			struct OtherDesc
			{
				D3D12_PRIMITIVE_TOPOLOGY_TYPE			topology_type;
				DXGI_SAMPLE_DESC						sample_desc;
				DXGI_FORMAT								dsv_format;
				D3D12_INDEX_BUFFER_STRIP_CUT_VALUE		index_cut_value;
			};
			struct Desc
			{
				BDTYPES									blend_types;
				SHADER::Desc							vs_desc, ps_desc, ds_desc, hs_desc, gs_desc;
				std::vector<D3D12_INPUT_ELEMENT_DESC>	input_layouts;
				std::vector<D3D12_ROOT_PARAMETER>		root_params;
				std::vector<D3D12_STATIC_SAMPLER_DESC>	static_samplers;
				std::vector<DXGI_FORMAT>				render_targets;
				D3D12_DEPTH_STENCIL_DESC				depth_desc;
				D3D12_RASTERIZER_DESC					rastarizer_desc;
				OtherDesc								other_desc;
			};
		protected:
			ComPtr<ID3D12PipelineState>	m_pl_state;
			ComPtr<ID3D12RootSignature>	m_rootsig;
		protected:
			[[nodiscard]] std::unique_ptr<KGL::Shader> GetShaderDesc(
				const std::shared_ptr<DXC>& dxc,
				const SHADER::Desc& vs_desc, const SHADER::Desc& ps_desc,
				const SHADER::Desc& ds_desc, const SHADER::Desc& hs_desc, const SHADER::Desc& gs_desc,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layouts,
				D3D12_GRAPHICS_PIPELINE_STATE_DESC* out_desc
			) noexcept;
		protected:
			BaseRenderer() = default;
			HRESULT Create(const ComPtr<ID3D12Device>& device, const std::shared_ptr<DXC>& dxc, const Desc& desc) noexcept;
		public:
			explicit BaseRenderer(const ComPtr<ID3D12Device>& device, const std::shared_ptr<DXC>& dxc, const Desc& desc) noexcept
			{ Create(device, dxc, desc); }
			virtual ~BaseRenderer() = default;
			void SetState(const ComPtr<ID3D12GraphicsCommandList>& cmd_list) const noexcept;
			void SetName(const std::filesystem::path& name) const noexcept;
		};
	}
}