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

			D3D12_VERTEX_BUFFER_VIEW				m_vb_view;
			D3D12_INDEX_BUFFER_VIEW					m_ib_view;
		private:
			HRESULT CreateVertexBuffers(ComPtr<ID3D12Device> device, const std::vector<UCHAR>& vert) noexcept;
			HRESULT CreateIndexBuffers(ComPtr<ID3D12Device> device, const std::vector<USHORT>& idx) noexcept;
			HRESULT CreateMaterialBuffers(ComPtr<ID3D12Device> device, const std::vector<PMD::Material>& mtr) noexcept;
			HRESULT CreateTextureBuffers(ComPtr<ID3D12Device> device, const std::vector<PMD::Material>& mtr,
				const std::filesystem::path& path, const std::filesystem::path& toon_folder,
				TextureManager* mgr) noexcept;

		public:
			explicit PMD_Model(ComPtr<ID3D12Device> device,
				const PMD::Desc& desc,
				const std::filesystem::path& toon_folder = {},	// FOLDER/
				TextureManager* mgr = nullptr) noexcept;
		};
	}
}