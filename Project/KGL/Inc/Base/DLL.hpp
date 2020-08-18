#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <filesystem>
#include <map>
#include <unordered_map>

namespace KGL
{
	inline namespace BASE
	{
		class DLL
		{
		protected:
			HMODULE								m_dll;
			std::map<std::string, FARPROC>		m_name_funcs;
			std::unordered_map<int, FARPROC>	m_num_funcs;
		public:
			explicit DLL(const std::filesystem::path& dll_path) noexcept;
			virtual ~DLL() noexcept;
			HRESULT Load(std::string func_name, int num = -1) noexcept;
		};
	}
}
