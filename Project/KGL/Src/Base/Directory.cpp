#include <Base/Directory.hpp>
#include <Helper/ThrowAssert.hpp>
#include <algorithm>

using namespace KGL;

Directory::Directory(const std::filesystem::path dir)
{
	m_path = dir;
	Reload();
}

HRESULT Directory::Reload()
{
	HRESULT hr = S_OK;
	m_files.clear();
	return Load(m_path);
}

HRESULT Directory::Load(const std::filesystem::path dir, std::filesystem::path sub_dir)
{
	HRESULT hr = S_OK;
	m_files.clear();
	using namespace std::filesystem;
	std::for_each(
		directory_iterator(dir), directory_iterator(),
		[&](const path& p) {
			if (is_regular_file(p)) { // ファイルなら...
				m_files.push_back(sub_dir.string() + p.filename().string());
			}
			else if (is_directory(p)) { // ディレクトリなら...
				std::string directory = sub_dir.string() + p.string();
				hr = Load(dir.string() + p.string(), directory);
				RCHECK_HRSTR(hr, directory + "の読み込みに失敗", "フォルダの読み込みに失敗");
			}
		}
	);
	return S_OK;
}

Files Directory::GetFiles(
	std::string extension,
	const std::filesystem::path& sub_dir,
	bool check_sub_dir
)
{
	Files files;
	std::transform(extension.begin(), extension.end(), extension.begin(), std::toupper);

	files.reserve(m_files.size());
	for (const auto& file : m_files)
	{
		if (!file.has_extension()) continue;
		std::string file_ext = file.extension().string();
		std::transform(file_ext.begin(), file_ext.end(), file_ext.begin(), std::toupper);
		if (file_ext == extension)
		{
			std::filesystem::path file_name_less = file;
			file_name_less.remove_filename();
			
			if (sub_dir.empty())
			{
				if (file_name_less.empty())
				{
					files.push_back(file);
				}
			}
			else
			{
				auto fnl_str = file_name_less.string();
				auto str_pos = fnl_str.find(sub_dir.string());
				if (str_pos == 0)
				{
					files.push_back(file);
				}
				else if (str_pos == std::string::npos)
				{
					if (file.parent_path().empty())
					{
						files.push_back(file);
					}
				}
			}
		}
	}
	return files;
}