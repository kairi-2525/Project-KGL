#pragma once

#include "CelealHelper.hpp"
#include "Effects.hpp"

#include <filesystem>
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <Base/Directory.hpp>
#include <map>

//struct VF2
//{
//	float x;
//	float y;
//};
//template<class Archive>
//void serialize(Archive& archive,
//	const VF2& m)
//{
//	archive(KGL_NVP("x", m.x), KGL_NVP("y", m.y));
//}

template<class Archive>
void serialize(Archive& archive,
	EffectDesc& m, std::uint32_t const version)
{
	using namespace DirectX;
	archive(
		KGL_NVP("start_time", m.start_time),
		KGL_NVP("time", m.time),
		KGL_NVP("start_accel", m.start_accel),
		KGL_NVP("end_accel", m.end_accel),
		KGL_NVP("alive_time", m.alive_time),
		KGL_NVP("late", m.late),
		KGL_NVP("late_update_time", m.late_update_time),
		KGL_NVP("speed", m.speed),
		KGL_NVP("base_speed", m.base_speed),
		KGL_NVP("scale", m.scale),
		KGL_NVP("scale_front", m.scale_front),
		KGL_NVP("scale_back", m.scale_back),
		KGL_NVP("angle", m.angle),
		KGL_NVP("spawn_space", m.spawn_space),
		KGL_NVP("begin_color", m.begin_color),
		KGL_NVP("end_color", m.end_color),
		KGL_NVP("erase_color", m.erase_color),
		KGL_NVP("resistivity", m.resistivity),
		KGL_NVP("scale_resistivity", m.scale_resistivity),
		KGL_NVP("bloom", m.bloom),
		KGL_NVP("has_child", m.has_child),
		KGL_NVP("child", m.child)					// パーティクルの代わりにFireworksを作成する場合ここに指定します。
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
	archive(
		KGL_NVP("mass", m.mass),
		KGL_NVP("resistivity", m.resistivity),
		KGL_NVP("speed", m.speed),
		KGL_NVP("effects", m.effects)
	);
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
	std::string													select_name;
	std::shared_ptr<FireworksDesc>								select_desc;
	std::map<const std::string, std::shared_ptr<FireworksDesc>>	desc_list;
private:
	HRESULT ReloadDesc() noexcept;
	void Create() noexcept;
	static void FWDescImGuiUpdate(FireworksDesc* desc);
public:
	void DescImGuiUpdate(Desc* desc);
	static HRESULT Export(const std::filesystem::path& path, const Desc& desc) noexcept;
	FCManager(const std::filesystem::path& directory);
	~FCManager() = default;
	HRESULT Load(const std::filesystem::path& directory) noexcept;
	HRESULT Load() noexcept { return Load(directory->GetPath()); }
	HRESULT ImGuiUpdate() noexcept;
	std::shared_ptr<FireworksDesc> GetSelectDesc() const noexcept { return select_desc; }
};

class FC
{
public:
	FC() = default;
	~FC() = default;
};