#pragma once

#include "CelealHelper.hpp"
#include "Effects.hpp"

#include <filesystem>
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <Base/Directory.hpp>
#include <map>

template<class Archive>
void serialize(Archive& archive,
	EffectDesc& m, std::uint32_t const version)
{
	archive(
		m.start_time,
		m.time,
		m.start_accel,
		m.end_accel,
		m.alive_time,
		m.late,
		m.late_update_time,
		m.speed,
		m.base_speed,
		m.scale,
		m.scale_front, m.scale_back,
		m.angle,
		m.spawn_space,
		m.begin_color,
		m.end_color,
		m.erase_color,
		m.resistivity,
		m.scale_resistivity,
		m.bloom,
		m.has_child,
		m.child					// パーティクルの代わりにFireworksを作成する場合ここに指定します。
	);
	if (1 <= version) {
		// archive(...);
	}
}
CEREAL_CLASS_VERSION(EffectDesc, 1);

template<class Archive>
void serialize(Archive& archive,
	FireworksDesc& m, std::uint32_t const version)
{
	archive(m.mass, m.resistivity, m.effects);
	if (1 <= version) {
		// archive(...);
	}
}
CEREAL_CLASS_VERSION(FireworksDesc, 1);

class FCManager
{
private:
	using Desc = std::pair<const std::string, std::shared_ptr<FireworksDesc>>;
private:
	std::shared_ptr<KGL::Directory>								directory;
	std::shared_ptr<Desc>										select_desc;
	std::map<const std::string, std::shared_ptr<FireworksDesc>>	desc_list;
private:
	HRESULT ReloadDesc() noexcept;
public:
	static void DescImGuiUpdate(std::shared_ptr<Desc> desc);
	static HRESULT Export(const Desc& desc);

	FCManager(const std::filesystem::path& directory);
	HRESULT Load(const std::filesystem::path& directory) noexcept;
	HRESULT Load() noexcept { return Load(directory->GetPath()); }
	HRESULT ImGuiUpdate()
};