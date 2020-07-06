#pragma once
#include "../Base/VMD.hpp"
#include <filesystem>

namespace KGL
{
	inline namespace LOADER
	{
		class VMD_Loader
		{
		private:
			std::shared_ptr<const VMD::Desc> m_desc;
		public:
			explicit VMD_Loader(std::filesystem::path path) noexcept;
			const std::shared_ptr<const VMD::Desc>& GetDesc() const noexcept { return m_desc; }
		};
	}
}