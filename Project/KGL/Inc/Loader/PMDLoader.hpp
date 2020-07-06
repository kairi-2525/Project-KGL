#pragma once


#include "../Base/PMD.hpp"

namespace KGL
{
	inline namespace LOADER
	{
		class PMD_Loader
		{
		private:
			std::shared_ptr<const PMD::Desc> m_desc;
		public:
			explicit PMD_Loader(std::filesystem::path path) noexcept;
			const std::shared_ptr<const PMD::Desc>& GetDesc() const noexcept { return m_desc; }
		};
	}
}