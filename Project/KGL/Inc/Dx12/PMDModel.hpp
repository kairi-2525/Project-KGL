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
			TextureManager							m_tex_mgr;
			std::unique_ptr<Texture>				m_tex_white;
			std::unique_ptr<Texture>				m_tex_black;
			std::unique_ptr<Texture>				m_tex_gradation;

			std::vector<ComPtr<ID3D12Resource>>		m_texture_resources;
			std::vector<ComPtr<ID3D12Resource>>		m_sph_resources;
			std::vector<ComPtr<ID3D12Resource>>		m_spa_resources;
			std::vector<ComPtr<ID3D12Resource>>		m_toon_resources;

			ComPtr<ID3D12Resource>					m_vert_buff;
			ComPtr<ID3D12Resource>					m_idx_buff;
			ComPtr<ID3D12Resource>					m_mtr_buff;

			D3D12_VERTEX_BUFFER_VIEW				m_vb_view;
			D3D12_INDEX_BUFFER_VIEW					m_ib_view;
		private:
			HRESULT CreateVertexBuffers(ComPtr<ID3D12Device> device, const std::vector<UCHAR>& vert) noexcept;
			HRESULT CreateIndexBuffers(ComPtr<ID3D12Device> device, const std::vector<USHORT>& idx) noexcept;
			HRESULT CreateMaterialBuffers(ComPtr<ID3D12Device> device, const std::vector<PMD::Material>& mtr) noexcept;
		public:
			explicit PMD_Model(ComPtr<ID3D12Device> device,
				const PMD::Desc& desc, TextureManager* mgr = nullptr) noexcept;
		};
	}
}