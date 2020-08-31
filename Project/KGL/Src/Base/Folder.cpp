#include <Base/Folder.hpp>
#include <Helper/ThrowAssert.hpp>
#include <algorithm>

using namespace KGL;

Folder::Folder(const std::filesystem::path folder)
{
	m_path = folder;
	Reload();
}

HRESULT Folder::Reload()
{
	HRESULT hr = S_OK;
	m_files.clear();
	return Load(m_path);
}

HRESULT Folder::Load(const std::filesystem::path folder, std::filesystem::path in_folder)
{
	HRESULT hr = S_OK;
	m_files.clear();
	using namespace std::filesystem;
	std::for_each(
		directory_iterator(folder), directory_iterator(),
		[&](const path& p) {
			if (is_regular_file(p)) { // ファイルなら...
				m_files.push_back(in_folder.string() + p.filename().string());
			}
			else if (is_directory(p)) { // ディレクトリなら...
				hr = Load(folder.string() + p.string(), in_folder.string() + p.string());
				RCHECK_HR_STR(hr, "");
			}
		}
	);
	return S_OK;
}