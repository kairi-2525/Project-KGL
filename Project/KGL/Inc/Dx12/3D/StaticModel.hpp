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
			};
		}
	}
}