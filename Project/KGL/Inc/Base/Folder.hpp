#include <filesystem>
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

namespace KGL
{
	inline namespace BASE
	{
		using Files = std::vector<std::filesystem::path>;
		class Folder
		{
		private:
			std::filesystem::path	m_path;
			Files					m_files;
		private:
			HRESULT Load(const std::filesystem::path folder, std::filesystem::path in_folder = std::filesystem::path());
		public:
			Folder(const std::filesystem::path folder);
			HRESULT Reload();
			const Files& GetFiles() const { return m_files; };
			Files GetFiles(std::string extension, const std::filesystem::path& in_folder = std::filesystem::path());
		};
	}
}