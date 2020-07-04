#pragma once

namespace KGL
{
	inline namespace LOADER
	{
		class VMD_Loader
		{
		private:

		public:
			explicit PMD_Loader(std::filesystem::path path) noexcept;
			const PMD::Desc& GetDesc() const noexcept { return m_desc; }
		};
	}
}