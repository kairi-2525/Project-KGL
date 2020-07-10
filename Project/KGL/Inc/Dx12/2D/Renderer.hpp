#pragma once

#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include "../Shader.hpp"

#include <DirectXMath.h>
#include <vector>
#include "../Base/Renderer.hpp"
#include "../BlendState.hpp"

namespace KGL
{
	inline namespace DX12
	{
		inline namespace _2D
		{
			class Renderer : public BaseRenderer
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
						"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
						D3D12_APPEND_ALIGNED_ELEMENT,
						D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
					}
				};
				static inline const Shader::Desc VS_DESC =
				{
					"./HLSL/2D/Sprite_vs.hlsl", "VSMain", "vs_5_0"
				};
				static inline const Shader::Desc PS_DESC =
				{
					"./HLSL/2D/Sprite_ps.hlsl", "PSMain", "ps_5_0"
				};
			private:

			public:
				explicit Renderer(
					const ComPtr<ID3D12Device>& device,
					BDTYPE type = BDTYPE::DEFAULT,
					const Shader::Desc& vs_desc = VS_DESC, const Shader::Desc& ps_desc = PS_DESC,
					const std::vector<D3D12_INPUT_ELEMENT_DESC>& input_layouts = INPUT_LAYOUTS,
					const std::vector<D3D12_DESCRIPTOR_RANGE>& add_range = {},
					const std::vector<D3D12_ROOT_PARAMETER>& add_root_param = {},
					const std::vector<D3D12_STATIC_SAMPLER_DESC>& add_smp_desc = {}
				) noexcept;
			};
		}
	}
}