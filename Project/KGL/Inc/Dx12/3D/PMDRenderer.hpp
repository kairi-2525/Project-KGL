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
				static inline const SHADER::Desc VS_DESC =
				{
					"./HLSL/3D/PMDVertexShader.hlsl", "VSMain", "vs_5_1"
				};
				static inline const SHADER::Desc PS_DESC =
				{
					"./HLSL/3D/PMDPixelShader.hlsl", "PSMain", "ps_5_1"
				};
				static inline const Desc DEFAULT_DESC =
				{
					BDTYPE::DEFAULT,
					VS_DESC, PS_DESC,
					INPUT_LAYOUTS,
					{}, {}, {},
					std::vector<DXGI_FORMAT>(1, DXGI_FORMAT_R8G8B8A8_UNORM)
				};
			public:
				explicit PMD_Renderer(
					const ComPtr<ID3D12Device>& device,
					const Desc& desc = DEFAULT_DESC
				) noexcept;
			};
		}
	}
}