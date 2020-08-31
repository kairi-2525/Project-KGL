#pragma once

#include "CelealHelper.hpp"
#include "Effects.hpp"

#include <filesystem>
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX


class FCManager
{
private:
	std::filesystem::path folder;
public:
	FCManager(const std::filesystem::path& folder);
	HRESULT Load(const std::filesystem::path& folder) noexcept;
	HRESULT Load() noexcept { return Load(folder); };
};