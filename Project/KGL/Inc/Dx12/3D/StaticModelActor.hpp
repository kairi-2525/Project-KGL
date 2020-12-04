#pragma once

#include "StaticModel.hpp"
#include "../../Base/Model/Actor.hpp"

namespace KGL
{
	inline namespace DX12
	{
		inline namespace _3D
		{
			class StaticModelActor : public BASE::Actor
			{
				using ModelBuffer = StaticModel::ModelBuffer;
				using ModelBufferResource =
					std::shared_ptr<KGL::Resource<ModelBuffer>>;
				using MaterialBuffer = StaticModel::MaterialBuffer;
				using MaterialBufferResource =
					std::shared_ptr<KGL::Resource<MaterialBuffer>>;
			private:
				struct Material
				{
					MaterialBufferResource resource;
					std::shared_ptr<KGL::DescriptorHandle> handle;
				};
			private:
				std::shared_ptr<const StaticModel>			m_model;
				ModelBufferResource							m_model_buffer;
				std::shared_ptr<KGL::DescriptorHandle>		m_model_buffer_handle;
				std::unordered_map<std::string, Material>	m_mtl_buffers;
				std::shared_ptr<KGL::DescriptorManager>		m_descriptor;
			public:
				explicit StaticModelActor(
					ComPtrC<ID3D12Device> device,
					std::shared_ptr<const StaticModel> model
				) noexcept;

				std::shared_ptr<KGL::Resource<MaterialBuffer>> GetMTLResource(
					const std::string mtl_name) noexcept;
				const std::unordered_map<std::string, Material>& GetMaterials() noexcept {
					return m_mtl_buffers;
				};

				void UpdateBuffer(DirectX::CXMMATRIX view_proj);
				void Render(ComPtrC<ID3D12GraphicsCommandList> cmd_list) const noexcept;
				std::shared_ptr<const StaticModel> GetModel() const noexcept { return m_model; }
				
				StaticModelActor(const StaticModelActor& m) noexcept { *this = m; }
				StaticModelActor& operator=(const StaticModelActor& m) noexcept;
			};
		}
	}
}