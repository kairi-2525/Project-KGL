#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Shader.hpp"
#include "../../Helper/ComPtr.hpp"
#include "../Base/Renderer.hpp"
#include "../BlendState.hpp"

#include <vector>

namespace KGL
{
	inline namespace DX12
	{
		inline namespace _3D
		{
			class PMD_Renderer : public BaseRenderer
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
					"./HLSL/3D/PMDVertexShader.hlsl", "BasicVS", "vs_5_0"
				};
				static inline const Shader::Desc PS_DESC =
				{
					"./HLSL/3D/PMDPixelShader.hlsl", "BasicPS", "ps_5_0"
				};
			public:
				explicit PMD_Renderer(
					const ComPtr<ID3D12Device>& device,
					BDTYPE type = BDTYPE::DEFAULT,
					const Shader::Desc& vs_desc = VS_DESC, const Shader::Desc& ps_desc = PS_DESC,
					const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layouts = INPUT_LAYOUTS
				) noexcept;
			};
		}
	}
}