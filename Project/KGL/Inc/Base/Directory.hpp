#pragma once
#include <filesystem>
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

namespace KGL
{
	inline namespace BASE
	{
		using Files = std::vector<std::filesystem::path>;
		class Directory
		{
		private:
			std::filesystem::path	m_path;
			Files					m_files;
		private:
			HRESULT Load(std::filesystem::path dir, std::filesystem::path sub_dir = std::filesystem::path());
		public:
			explicit Directory(const std::filesystem::path& dir);
			virtual ~Directory() = default;
			HRESULT Reload();
			const std::filesystem::path& GetPath() const noexcept { return m_path; }
			const Files& GetFiles() const noexcept { return m_files; };
			Files GetFiles(
				std::string extension,
				const std::filesystem::path& sub_dir = std::filesystem::path(),
				bool check_sub_dir = true
			);
		};
	}
}