#pragma once

#include "Shader.hpp"
#include "../Helper/ComPtr.hpp"
#include "../Helper/Cast.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class ComputePipline
		{
		private:
			struct Desc
			{
				SHADER::Desc cs_desc;
				std::vector<D3D12_ROOT_PARAMETER> root_params;
				std::vector<D3D12_STATIC_SAMPLER_DESC> static_samplars;
			};
		public:
			static inline const SHADER::Desc DEFAULT_SHADER_DESC
			{
				"./HLSL/3D/Particle_cs.hlsl", "CSMain", "cs_5_1"
			};
			static inline const std::vector<D3D12_DESCRIPTOR_RANGE> DEFAULT_DESCRIPTOR_RANGES =
			{
				{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1u, 0u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
			};
			static inline const std::vector<D3D12_ROOT_PARAMETER> DEFAULT_ROOT_PARAMS =
			{
				{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				{ { SCAST<UINT>(DEFAULT_DESCRIPTOR_RANGES.size()), DEFAULT_DESCRIPTOR_RANGES.data() } },
				D3D12_SHADER_VISIBILITY_ALL }
			};
			static inline const std::vector<D3D12_STATIC_SAMPLER_DESC> DEFAULT_SAMPLERS =
			{
			};
			static inline const Desc DEFAULT_DESC =
			{
				{ "", "", "" },
				DEFAULT_ROOT_PARAMS,
				DEFAULT_SAMPLERS
			};
		private:
			ComPtr<ID3D12PipelineState>	m_pl_state;
			ComPtr<ID3D12RootSignature>	m_rootsig;
		public:
			explicit ComputePipline(ComPtrC<ID3D12Device> device, const Desc& desc = DEFAULT_DESC) noexcept;
		};
	}
}