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
				static inline const SHADER::Desc VS_DESC =
				{
					"./HLSL/3D/PMDVertexShader.hlsl", "VSMain", "vs_5_1"
				};
				static inline const SHADER::Desc PS_DESC =
				{
					"./HLSL/3D/PMDPixelShader.hlsl", "PSMain", "ps_5_1"
				};
				static inline const SHADER::Desc DS_DESC =
				{
					{}, "DSMain", "ds_5_1"
				};
				static inline const SHADER::Desc HS_DESC =
				{
					{}, "HSMain", "hs_5_1"
				};
				static inline const SHADER::Desc GS_DESC =
				{
					{}, "GSMain", "gs_5_1"
				};
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
					{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1u, 2u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
					{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4u, 0u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
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
				static inline const std::vector<D3D12_STATIC_SAMPLER_DESC> STATIC_SAMPLERS =
				{
					{
						D3D12_FILTER_MIN_MAG_MIP_LINEAR,
						D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
						0.f, 16u, D3D12_COMPARISON_FUNC_NEVER, D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
						0.f, D3D12_FLOAT32_MAX, 0u, 0u, D3D12_SHADER_VISIBILITY_PIXEL
					},
					{
						D3D12_FILTER_ANISOTROPIC,
						D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
						0.f, 16u, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
						0.f, D3D12_FLOAT32_MAX, 1u, 0u, D3D12_SHADER_VISIBILITY_PIXEL
					}
				};
				static inline const std::vector<DXGI_FORMAT> RENDER_TARGETS =
				{
					{ DXGI_FORMAT_R8G8B8A8_UNORM }
				};
				static inline const D3D12_DEPTH_STENCIL_DESC DEPTH_DESC =
				{
					true, D3D12_DEPTH_WRITE_MASK_ALL,
					D3D12_COMPARISON_FUNC_LESS, // è¨Ç≥Ç¢ÇŸÇ§ÇèëÇ´çûÇﬁ

				};
				static inline const D3D12_RASTERIZER_DESC RASTARIZER_DESC =
				{
					D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_NONE, FALSE,
					D3D12_DEFAULT_DEPTH_BIAS, D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
					D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
					TRUE, FALSE, FALSE, 0u, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
				};
				static inline const OtherDesc OTHER_DESC =
				{
					D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
					{ 1u, 0u },
					DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
					D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED
				};
				static inline const Desc DEFAULT_DESC =
				{
					{ BDTYPE::DEFAULT },
					VS_DESC, PS_DESC, DS_DESC, HS_DESC, GS_DESC,
					INPUT_LAYOUTS,
					ROOT_PARAMS,
					STATIC_SAMPLERS,
					RENDER_TARGETS,
					DEPTH_DESC,
					RASTARIZER_DESC,
					OTHER_DESC
				};
			public:
				explicit PMD_Renderer(
					const ComPtr<ID3D12Device>& device,
					const std::shared_ptr<DXC> dxc,
					const Desc& desc = DEFAULT_DESC
				) noexcept {
					Create(device, dxc, desc);
				}
			};
		}
	}
}