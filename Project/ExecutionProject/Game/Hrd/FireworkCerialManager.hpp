#pragma once

#include "CelealHelper.hpp"
#include "Effects.hpp"

#include <filesystem>
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <Base/Directory.hpp>
#include <map>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <mutex>


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
	struct DemoData
	{
		static inline const float							FRAME_SECOND = 0.03f;
		struct World
		{
			DirectX::XMFLOAT3								position;
		};
		std::shared_ptr<FireworksDesc>						fw_desc;
		std::vector<std::vector<Particle>>					ptcs;
		std::shared_ptr<KGL::Resource<Particle>>			resource;
		std::shared_ptr<KGL::Resource<World>>				world_resource;
		std::shared_ptr<KGL::DescriptorManager>				world_dm;
		KGL::DescriptorHandle								world_handle;
		D3D12_VERTEX_BUFFER_VIEW							vbv;
		bool												draw_flg;
		bool												build_flg;
		std::mutex											build_mutex;
		bool												exist;

		DemoData(KGL::ComPtrC<ID3D12Device> device, std::shared_ptr<FireworksDesc> desc, UINT64 capacity);
		DemoData(const DemoData& data);
		DemoData& operator=(const DemoData& data);
		void SetResource(UINT num);
		void Build(const ParticleParent* p_parent, UINT set_frame_num);
		void Render(KGL::ComPtr<ID3D12GraphicsCommandList> cmd_list, UINT num) const noexcept;
	};
private:
	std::shared_ptr<KGL::Directory>								directory;
	std::string													select_name;
	std::shared_ptr<FireworksDesc>								select_desc;
	std::map<const std::string, std::shared_ptr<FireworksDesc>>	desc_list;
	std::vector<DemoData>										demo_data;
	UINT														select_demo_number;
	UINT														demo_frame_number;
	bool														demo_play;
	float														demo_play_frame;
	UINT														demo_build_count;
	std::mutex													demo_build_mutex;
private:
	HRESULT ReloadDesc() noexcept;
	bool ChangeName(std::string before, std::string after) noexcept;
	void Create() noexcept;
	void PlayDemo(UINT frame_num) noexcept;
	void StopDemo() noexcept;
	void UpdateDemo(float update_time) noexcept;
	void CreateDemo(KGL::ComPtrC<ID3D12Device> device, std::shared_ptr<FireworksDesc> desc, const ParticleParent* p_parent) noexcept;
	static void FWDescImGuiUpdate(FireworksDesc* desc);
	static float GetMaxTime(const FireworksDesc& desc);
public:
	void DescImGuiUpdate(KGL::ComPtrC<ID3D12Device> device, Desc* desc, const ParticleParent* p_parent);
	static HRESULT Export(const std::filesystem::path& path, const Desc& desc) noexcept;
	FCManager(const std::filesystem::path& directory);
	~FCManager() = default;
	HRESULT Load(const std::filesystem::path& directory) noexcept;
	HRESULT Load() noexcept { return Load(directory->GetPath()); }
	HRESULT ImGuiUpdate(KGL::ComPtrC<ID3D12Device> device, const ParticleParent* p_parent) noexcept;
	HRESULT Update(float update_time) noexcept;
	std::shared_ptr<FireworksDesc> GetSelectDesc() const noexcept { return select_desc; }
	void Render(KGL::ComPtr<ID3D12GraphicsCommandList> cmd_list) noexcept;
};

class FC
{
public:
	FC() = default;
	~FC() = default;
};