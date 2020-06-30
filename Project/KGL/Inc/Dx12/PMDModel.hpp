#pragma once

#include "../Base/PMD.hpp"
#include "Texture.hpp"

namespace KGL
{
	inline namespace DX12
	{
		class PMD_Model
		{
			std::unique_ptr<Texture> m_tex_white;
			std::unique_ptr<Texture> m_tex_black;
		public:
			explicit PMD_Model(ComPtr<ID3D12Device> device,
				const PMD::Desc& desc, TextureManager* mgr) noexcept;
		};
	}
}