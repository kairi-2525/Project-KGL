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
			static inline const SHADER::Desc SHADER_DESC
			{
				"./HLSL/3D/Particle_cs.hlsl", "CSMain", "cs_5_1"
			};
			static inline const std::vector<D3D12_DESCRIPTOR_RANGE> DESCRIPTOR_RANGES0 =
			{
				{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1u, 0u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
			};
			static inline const std::vector<D3D12_DESCRIPTOR_RANGE> DESCRIPTOR_RANGES1 =
			{
				{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1u, 1u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
			};
			static inline const std::vector<D3D12_DESCRIPTOR_RANGE> DESCRIPTOR_RANGES2 =
			{
				{ D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1u, 0u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
			};
			static inline const std::vector<D3D12_ROOT_PARAMETER> ROOT_PARAMS =
			{
				{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				{ { SCAST<UINT>(DESCRIPTOR_RANGES0.size()), DESCRIPTOR_RANGES0.data() } },
				D3D12_SHADER_VISIBILITY_ALL },
				{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				{ { SCAST<UINT>(DESCRIPTOR_RANGES1.size()), DESCRIPTOR_RANGES1.data() } },
				D3D12_SHADER_VISIBILITY_ALL },
				{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				{ { SCAST<UINT>(DESCRIPTOR_RANGES2.size()), DESCRIPTOR_RANGES2.data() } },
				D3D12_SHADER_VISIBILITY_ALL }
			};
			static inline const std::vector<D3D12_STATIC_SAMPLER_DESC> SAMPLERS =
			{
			};
			static inline const Desc DEFAULT_DESC =
			{
				SHADER_DESC,
				ROOT_PARAMS,
				SAMPLERS
			};
		private:
			ComPtr<ID3D12PipelineState>	m_pl_state;
			ComPtr<ID3D12RootSignature>	m_rootsig;
		public:
			explicit ComputePipline(ComPtrC<ID3D12Device> device, const std::shared_ptr<DXC> dxc, const Desc& desc = DEFAULT_DESC) noexcept;
			void SetState(const ComPtr<ID3D12GraphicsCommandList>& cmd_list) const noexcept;
		};
	}
}