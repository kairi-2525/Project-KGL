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
		throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s�@��Ή��̊g���q�ł��B");
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

	// �Ǝ��`���t�@�C���̑��݂��m�F���A���݂����ꍇ�͂������ǂݍ���
	if (IsExtension(m_path, EXTENSION))
	{	// �g���q�Ō`����Ǝ��`���t�@�C���𔻒f���ǂݍ���
		m_materials = StaticModelLoad(m_path);
		Loaded();
	}
	else
	{	// �g���q�����ύX���ē����̓Ǝ��`���t�@�C���̑��݂��m�F����
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