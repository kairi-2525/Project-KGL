#pragma once

#include "../Base/Model.hpp"
#include "../Base/PMD.hpp"
#include "Texture.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class PMD_Model
		{
		public:
			static constexpr size_t BUFFER_SIZE = (sizeof(MODEL::MaterialForHLSL) + 0xff) & ~0xff;
		private:
			struct TexturesResource
			{
				std::unique_ptr<Texture>			diffuse_buff;
				std::unique_ptr<Texture>			sph_buff;
				std::unique_ptr<Texture>			spa_buff;
				std::unique_ptr<Texture>			toon_buff;
			};
		private:
			TextureManager							m_tex_mgr;
			std::vector<TexturesResource>			m_textures;

			ComPtr<ID3D12Resource>					m_vert_buff;
			ComPtr<ID3D12Resource>					m_idx_buff;
			ComPtr<ID3D12Resource>					m_mtr_buff;

			ComPtr<ID3D12DescriptorHeap>			m_mtr_heap;
			D3D12_VERTEX_BUFFER_VIEW				m_vb_view;
			D3D12_INDEX_BUFFER_VIEW					m_ib_view;
			size_t									m_material_num;
			std::vector<UINT>						m_index_counts;
			std::vector<DirectX::XMMATRIX>			m_bone_matrices;
		private:
			HRESULT CreateVertexBuffers(ComPtr<ID3D12Device> device, const std::vector<UCHAR>& vert) noexcept;
			HRESULT CreateIndexBuffers(ComPtr<ID3D12Device> device, const std::vector<USHORT>& idx) noexcept;
			HRESULT CreateMaterialBuffers(ComPtr<ID3D12Device> device, const std::vector<PMD::Material>& mtr) noexcept;
			HRESULT CreateTextureBuffers(ComPtr<ID3D12Device> device, const std::vector<PMD::Material>& mtr,
				const std::filesystem::path& path, const std::filesystem::path& toon_folder,
				TextureManager* mgr) noexcept;
			HRESULT CreateMaterialHeap(ComPtr<ID3D12Device> device) noexcept;
			HRESULT CreateBoneMatrix(const std::map<std::string, PMD::BoneNode>& bone_node_table) noexcept;
		public:
			explicit PMD_Model(
				const ComPtr<ID3D12Device>& device,
				const PMD::Desc& desc,
				TextureManager* mgr = nullptr) noexcept :
				PMD_Model(device, desc, std::filesystem::path(), mgr) {}

			explicit PMD_Model(
				const ComPtr<ID3D12Device>& device,
				const PMD::Desc& desc,
				const std::filesystem::path& toon_folder = {},	// FOLDER/
				TextureManager* mgr = nullptr) noexcept;
			size_t GetMaterialCount() const noexcept { return m_material_num; }
			const std::vector<DirectX::XMMATRIX>& GetBoneMatrices() const { return m_bone_matrices; }
			HRESULT Render(
				const ComPtr<ID3D12Device>& device,
				const ComPtr<ID3D12GraphicsCommandList>& cmd_list
			) const noexcept;
		};
	}
}