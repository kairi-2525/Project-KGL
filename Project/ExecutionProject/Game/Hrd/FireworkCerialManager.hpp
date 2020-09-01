#pragma once

#include "CelealHelper.hpp"
#include "Effects.hpp"

#include <filesystem>
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <Base/Directory.hpp>


class FCManager
{
private:
	std::shared_ptr<KGL::Directory> directory;
public:
	FCManager(const std::filesystem::path& directory);
	HRESULT Load(const std::filesystem::path& directory) noexcept;
	HRESULT Load() noexcept { return Load(directory->GetPath()); }
};