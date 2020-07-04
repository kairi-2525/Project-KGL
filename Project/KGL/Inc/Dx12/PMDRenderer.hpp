#pragma once

#include "Shader.hpp"

#include "../Helper/ComPtr.hpp"

#include <vector>

namespace KGL
{
	inline namespace DX12
	{
		class PMD_Renderer
		{
		public:
			static inline const std::vector<D3D12_INPUT_ELEMENT_DESC> INPUT_LAYOUTS =
			{
				{
					"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
					D3D12_APPEND_ALIGNED_ELEMENT,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
				},
				{
					"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
					D3D12_APPEND_ALIGNED_ELEMENT,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
				},
				{
					"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
					D3D12_APPEND_ALIGNED_ELEMENT,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
				},
				{
					"BONE_NO", 0, DXGI_FORMAT_R16G16_UINT, 0,
					D3D12_APPEND_ALIGNED_ELEMENT,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
				},
				{
					"WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0,
					D3D12_APPEND_ALIGNED_ELEMENT,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
				},
				{
					"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT, 0,
					D3D12_APPEND_ALIGNED_ELEMENT,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
				}
			};
			static inline const Shader::Desc VS_DESC =
			{
				"./HLSL/PMDVertexShader.hlsl", "BasicVS", "vs_5_0"
			};
			static inline const Shader::Desc PS_DESC =
			{
				"./HLSL/PMDPixelShader.hlsl", "BasicPS", "ps_5_0"
			};
		private:
			ComPtr<ID3D12PipelineState>	m_pl_state;
			ComPtr<ID3D12RootSignature>	m_rootsig;
		public:
			explicit PMD_Renderer(
				const ComPtr<ID3D12Device>& device,
				const Shader::Desc& vs_desc = VS_DESC, const Shader::Desc& ps_desc = PS_DESC,
				const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layouts = INPUT_LAYOUTS
			) noexcept;
			void SetState(const ComPtr<ID3D12GraphicsCommandList>& cmd_list) const noexcept;
		};
	}
}