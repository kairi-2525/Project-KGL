#include "../Hrd/FireworkCerialManager.hpp"
#include <algorithm>

FCManager::FCManager(const std::filesystem::path& directory)
{
	Load(directory);
}

HRESULT FCManager::Load(const std::filesystem::path& directory) noexcept
{
	this->directory = std::make_shared<KGL::Directory>(directory);
	return S_OK;
}