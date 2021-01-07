#pragma once

#include "../3D/StaticModel.hpp"
#include "AccelerationStructure.hpp"

namespace KGL
{
	inline namespace DX12
	{
		namespace DXR
		{
			class StaticModel : public _3D::StaticModel
			{
			private:
				std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_vertex_geometry_descs;
				ASResources									m_as_resources;
			private:
				// AS生成に必要な頂点バッファの各サイズを算出
				void ComputeVertexBufferSize(
					ComPtrC<ID3D12Device5> device,
					std::shared_ptr<const StaticModelLoader> loader,
					UINT64* scratch_size_in_bytes,
					UINT64* result_size_in_bytes
				) noexcept;
			public:
				explicit StaticModel(
					ComPtrC<ID3D12Device5> device,
					ComPtrC<ID3D12GraphicsCommandList4> cmd_list,
					std::shared_ptr<const StaticModelLoader> loader,
					std::shared_ptr<TextureManager> tex_mgr = nullptr,
					std::shared_ptr<DescriptorManager> descriptor_mgr = nullptr
				) noexcept;

				void Render(ComPtrC<ID3D12GraphicsCommandList> cmd_list) const noexcept;

				// Bottom Level Acceleration Structure
				BLAS GetBLAS() const noexcept 
				{
					return { m_as_resources.result, DirectX::XMMatrixIdentity() };
				}
			};
		}
	}
}