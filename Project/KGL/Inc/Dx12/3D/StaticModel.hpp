#pragma once

#include "../../Loader/Loader.hpp"
#include "../ConstantBuffer.hpp"
#include "../DescriptorHeap.hpp"
#include "../Texture.hpp"
#include <unordered_map>
#include <array>

namespace KGL
{
	inline namespace DX12
	{
		inline namespace _3D
		{
			class StaticModel
			{
			public:
				using ParameterHandles = std::unordered_map<std::string, std::shared_ptr<DescriptorHandle>>;
			private:
				static inline const size_t REFLECTIONS_SIZE = 7u;
				static inline const size_t TEXTURE_SIZE = REFLECTIONS_SIZE + 8u;
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
					},
					{
						"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
						D3D12_APPEND_ALIGNED_ELEMENT,
						D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
					},
					{
						"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
						D3D12_APPEND_ALIGNED_ELEMENT,
						D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
					},
					{
						"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
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
					{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1u, 2u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
				};
				static inline const std::vector<D3D12_DESCRIPTOR_RANGE> DESCRIPTOR_RANGES3 =
				{
					{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 7u, 0u, 0u, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
				};
				static inline const std::vector<D3D12_ROOT_PARAMETER> ROOT_PARAMS =
				{
					{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					{ { SCAST<UINT>(DESCRIPTOR_RANGES0.size()), DESCRIPTOR_RANGES0.data() } },
					D3D12_SHADER_VISIBILITY_ALL },
					{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					{ { SCAST<UINT>(DESCRIPTOR_RANGES1.size()), DESCRIPTOR_RANGES1.data() } },
					D3D12_SHADER_VISIBILITY_VERTEX },
					{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					{ { SCAST<UINT>(DESCRIPTOR_RANGES2.size()), DESCRIPTOR_RANGES2.data() } },
					D3D12_SHADER_VISIBILITY_PIXEL },
					{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					{ { SCAST<UINT>(DESCRIPTOR_RANGES3.size()), DESCRIPTOR_RANGES3.data() } },
					D3D12_SHADER_VISIBILITY_PIXEL },
				};
			public:
				struct ModelBuffer
				{
					DirectX::XMFLOAT4X4 world;
					DirectX::XMFLOAT4X4 wvp;
				};
				using MaterialBuffer = S_MODEL::Material::Parameter;
			private:
				enum REFLECTIONS_TYPE : UINT
				{
					SPHERE,
					CUBE_TOP,
					CUBE_BOTTOM,
					CUBE_FRONT,
					CUBE_BACK,
					CUBE_LEFT,
					CUBE_RIGHT
				};

				struct HandleTexture
				{
					std::shared_ptr<Texture>			tex;
					std::shared_ptr<DescriptorHandle>	handle;
				};

				struct Material
				{
					using ReflectionsTexture = std::array<HandleTexture, REFLECTIONS_SIZE>;
					std::shared_ptr<Resource<S_MODEL::Material::Parameter>>	rs_param;
					std::shared_ptr<DescriptorHandle>						param_cbv_handle;
					std::shared_ptr<Resource<S_MODEL::Vertex>>				rs_vertices;
					D3D12_VERTEX_BUFFER_VIEW								vertex_buffer_view;

					HandleTexture											ambient;
					HandleTexture											diffuse;
					HandleTexture											specular;
					HandleTexture											specular_highlights;
					HandleTexture											dissolve;
					HandleTexture											bump;
					HandleTexture											displacement;
					HandleTexture											stencil_decal;
					ReflectionsTexture										reflections;
				};
			private:
				std::unordered_map<std::string, Material>	m_materials;
				std::shared_ptr<TextureManager>				m_tex_mgr;
				std::shared_ptr<DescriptorManager>			m_descriptor_mgr;
				ParameterHandles							m_param_handles;
			public:
				explicit StaticModel(
					ComPtrC<ID3D12Device> device,
					std::shared_ptr<const StaticModelLoader> loader,
					std::shared_ptr<TextureManager> tex_mgr = nullptr,
					std::shared_ptr<DescriptorManager> descriptor_mgr = nullptr
				) noexcept;

				void Render(ComPtrC<ID3D12GraphicsCommandList> cmd_list) const noexcept;
				const std::unordered_map<std::string, Material>& GetMaterials() const noexcept
				{
					return m_materials;
				}
			};
		}
	}
}