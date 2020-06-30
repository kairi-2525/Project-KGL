#pragma once

#define NOMINMAX
#include "../Base/PMD.hpp"

namespace KGL
{
	inline namespace LOADER
	{
		class PMDLoader
		{
		private:
			PMD::Desc m_desc;
		public:
			explicit PMDLoader(std::filesystem::path path) noexcept;
			const PMD::Desc& GetDesc() const noexcept { return m_desc; }
		};
	}
}