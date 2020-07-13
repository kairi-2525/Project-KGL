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
				static inline const Desc DEFAULT_DESC =
				{
					BDTYPE::DEFAULT,
					VS_DESC, PS_DESC,
					INPUT_LAYOUTS,
					{}, {}, {},
					std::vector<DXGI_FORMAT>(1, DXGI_FORMAT_R8G8B8A8_UNORM)
				};
			private:

			public:
				explicit Renderer(
					const ComPtr<ID3D12Device>& device,
					const Desc& desc = DEFAULT_DESC
				) noexcept;
			};
		}
	}
}