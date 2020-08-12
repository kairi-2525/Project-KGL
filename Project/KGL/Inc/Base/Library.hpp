#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <filesystem>

namespace KGL
{
	inline namespace BASE
	{
		class Library
		{
		private:
			HMODULE m_lib;
		public:
			explicit Library(const std::filesystem::path& name) noexcept;
		};
	}
}
