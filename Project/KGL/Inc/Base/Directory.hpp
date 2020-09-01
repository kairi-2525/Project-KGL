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
			HRESULT Load(const std::filesystem::path dir, std::filesystem::path sub_dir = std::filesystem::path());
		public:
			Directory(const std::filesystem::path dir);
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