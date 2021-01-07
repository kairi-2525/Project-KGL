#pragma once

#include "../3D/StaticModel.hpp"

namespace KGL
{
	inline namespace DX12
	{
		namespace DXR
		{
			class StaticModel : public _3D::StaticModel
			{
			private:
				struct ASResources
				{
					std::shared_ptr<KGL::Resource<CHAR>>	scratch;		// ASビルダーのスクラッチメモリ
					std::shared_ptr<KGL::Resource<CHAR>>	result;			// ASの場所
					std::shared_ptr<KGL::Resource<CHAR>>	instance_desc;	// インスタンスの行列を保持する
				};
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
					std::shared_ptr<const StaticModelLoader> loader,
					std::shared_ptr<TextureManager> tex_mgr = nullptr,
					std::shared_ptr<DescriptorManager> descriptor_mgr = nullptr
				) noexcept;

				void Render(ComPtrC<ID3D12GraphicsCommandList> cmd_list) const noexcept;
				const std::unordered_map<std::string, Material>& GetMaterials() const noexcept
				{
					return m_materials;
				}
				const std::filesystem::path& GetPath() const noexcept { return m_path; }
			};
		}
	}
}