#include <Loader/Loader.hpp>
#include <algorithm>
#include <Helper/Cast.hpp>
#include <Helper/Convert.hpp>

using namespace KGL;

bool Loader::IsExtensiton(const std::filesystem::path& path, const std::string& extension) noexcept
{
	auto e = CONVERT::StrToUpper(extension);
	auto p = CONVERT::StrToUpper(path.extension().string());
	return e == p;
}

void Loader::CheckExtensiton(const std::filesystem::path& path, const std::string& extension) noexcept(false)
{
	if (!IsExtensiton(path, extension))
	{
		throw std::runtime_error("[ " + path.string() + " ] ‚Ì“Ç‚İ‚İ‚É¸”s@”ñ‘Î‰‚ÌŠg’£q‚Å‚·B");
	}
}