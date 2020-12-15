#include <Loader/Loader.hpp>
#include <algorithm>
#include <Helper/Cast.hpp>
#include <Helper/Convert.hpp>
#include <fstream>

using namespace KGL;

bool Loader::IsExtension(const std::filesystem::path& path, const std::filesystem::path& extension) noexcept
{
	auto e = CONVERT::StrToUpper(extension.string());
	auto p = CONVERT::StrToUpper(path.extension().string());
	return e == p;
}

void Loader::CheckExtension(const std::filesystem::path& path, const std::filesystem::path& extension) noexcept(false)
{
	if (!IsExtension(path, extension))
	{
		throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗　非対応の拡張子です。");
	}
}

void Loader::ReplaceExtension(const std::filesystem::path& extension) noexcept
{
	m_path.replace_extension(extension);
}

std::shared_ptr<S_MODEL::Materials> StaticModelLoad(
	const std::filesystem::path& path
) {
	auto data = std::make_shared<S_MODEL::Materials>();

}

StaticModelLoader::StaticModelLoader(
	const std::filesystem::path& path
) noexcept :
	Loader(path)
{
	auto m_path = GetPath();

	// 独自形式ファイルの存在を確認し、存在した場合はそちらを読み込む
	if (IsExtension(m_path, EXTENSION))
	{	// 拡張子で形式を独自形式ファイルを判断し読み込む
		m_materials = StaticModelLoad(m_path);
		Loaded();
	}
	else
	{	// 拡張子だけ変更して同名の独自形式ファイルの存在を確認する
		m_path.replace_extension(EXTENSION);
		{
			std::ifstream ifs(m_path);
			if (!ifs.is_open()) return;
		}
		ReplaceExtension(EXTENSION);
		m_materials = StaticModelLoad(m_path);
		Loaded();
	}
}