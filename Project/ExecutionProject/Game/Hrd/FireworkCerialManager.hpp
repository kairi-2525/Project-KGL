#pragma once

#include <Helper/Cast.hpp>
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
#include <Dx12/Texture.hpp>
#include <mutex>
#include <thread>
#include <list>

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

#define EV_0_EFDC_ARCHIVE \
	KGL_NVP("start_time", m.start_time), \
	KGL_NVP("time", m.time), \
	KGL_NVP("start_accel", m.start_accel), \
	KGL_NVP("end_accel", m.end_accel), \
	KGL_NVP("alive_time", m.alive_time), \
	KGL_NVP("late", m.late), \
	KGL_NVP("late_update_time", m.late_update_time), \
	KGL_NVP("speed", m.speed), \
	KGL_NVP("base_speed", m.base_speed), \
	KGL_NVP("scale", m.scale), \
	KGL_NVP("scale_front", m.scale_front), \
	KGL_NVP("scale_back", m.scale_back), \
	KGL_NVP("angle", m.angle), \
	KGL_NVP("spawn_space", m.spawn_space), \
	KGL_NVP("begin_color", m.begin_color), \
	KGL_NVP("end_color", m.end_color), \
	KGL_NVP("erase_color", m.erase_color), \
	KGL_NVP("resistivity", m.resistivity), \
	KGL_NVP("scale_resistivity", m.scale_resistivity), \
	KGL_NVP("bloom", m.bloom), \
	KGL_NVP("has_child", m.has_child), \
	KGL_NVP("child", m.child)

#define EV_1_EFDC_ARCHIVE \
	EV_0_EFDC_ARCHIVE, \
	KGL_NVP("name", m.name)

#define EV_2_EFDC_ARCHIVE \
	EV_1_EFDC_ARCHIVE, \
	KGL_NVP("texture_name", m.texture_name)

#define EV_0_FWDC_ARCHIVE \
	KGL_NVP("mass", m.mass), \
	KGL_NVP("resistivity", m.resistivity), \
	KGL_NVP("speed", m.speed), \
	KGL_NVP("effects", m.effects)

template<class Archive>
void serialize(Archive& archive,
	EffectDesc& m, std::uint32_t const version)
{
	using namespace DirectX;
	switch (SCAST<EFFECT_VERSION>(version))
	{
		case EFFECT_VERSION::EV_0:
		{
			archive(EV_0_EFDC_ARCHIVE);
			break;
		}
		case EFFECT_VERSION::EV_1:
		{
			archive(EV_1_EFDC_ARCHIVE);
			break;
		}
		case EFFECT_VERSION::EV_2:
		{
			archive(EV_2_EFDC_ARCHIVE);
			break;
		}
	}
}
CEREAL_CLASS_VERSION(EffectDesc, SCAST<UINT>(EFFECT_VERSION::EV_2));

template<class Archive>
void serialize(Archive& archive,
	FireworksDesc& m, std::uint32_t const version)
{
	using namespace DirectX;
	switch (SCAST<FIREWORKS_VERSION>(version))
	{
		case FIREWORKS_VERSION::FV_0:
		{
			archive(EV_0_FWDC_ARCHIVE);
			break;
		}
		case FIREWORKS_VERSION::FV_1:
		{
			archive(
				EV_0_FWDC_ARCHIVE,
				KGL_NVP("original_name", m.original_name)
			);
			break;
		}
	}
}
CEREAL_CLASS_VERSION(FireworksDesc, SCAST<UINT>(FIREWORKS_VERSION::FV_1));

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

		DemoData(
			KGL::ComPtrC<ID3D12Device> device,
			std::shared_ptr<FireworksDesc> desc,
			UINT64 capacity
		);
		DemoData(const DemoData& data);
		DemoData& operator=(const DemoData& data);
		void SetResource(UINT num);
		void Build(const ParticleParent* p_parent, UINT set_frame_num,
			const std::vector<AffectObjects>& affect_objects
		);
		void Render(KGL::ComPtr<ID3D12GraphicsCommandList> cmd_list, UINT num) const noexcept;
		size_t Size(UINT num) const;
	};
	enum FWDESC_STATE
	{
		NONE,
		LOOP,
		ERASE
	};
private:
	std::shared_ptr<KGL::Directory>								directory;
	std::string													select_name;
	std::shared_ptr<FireworksDesc>								select_desc;
	std::map<const std::string, std::shared_ptr<FireworksDesc>>	desc_list;
	std::list<DemoData>											demo_data;
	std::list<std::shared_ptr<FireworksDesc>>					add_demo_data;
	std::unique_ptr<std::thread>								demo_mg_th;
	std::mutex													demo_mg_mutex;
	std::mutex													demo_mg_stop_mutex;
	bool														demo_mg_stop;
	UINT														select_demo_number;
	UINT														demo_frame_number;
	bool														demo_play;
	float														demo_play_frame;

	std::mutex													demo_select_mutex;
	std::mutex													demo_select_stop_mutex;
	std::mutex													demo_select_clear_mutex;
	bool														demo_select_stop;
	std::list<DemoData>											demo_select_data;
	std::list<std::shared_ptr<FireworksDesc>>					add_demo_select_data;
	std::unique_ptr<std::thread>								demo_select_th;
	std::list<DemoData>::iterator								demo_select_itr;

	std::unique_ptr<std::pair<const std::string, FireworksDesc>> cpy_fw_desc;
	std::unique_ptr<EffectDesc>									cpy_ef_desc;
	std::vector<std::shared_ptr<KGL::Texture>>					textures;
public:
	std::vector<AffectObjects>									affect_objects;
private:
	HRESULT ReloadDesc() noexcept;
	bool ChangeName(std::string before, std::string after) noexcept;
	void DemoDataManaged();
	void Create(const std::string& name = "none", const FireworksDesc* desc = &FIREWORK_EFFECTS::FW_DEFAULT) noexcept;
	void PlayDemo(UINT frame_num) noexcept;
	void StopDemo() noexcept;
	void UpdateDemo(float update_time) noexcept;
	static void CreateDemo(
		KGL::ComPtrC<ID3D12Device> device, const ParticleParent p_parent,
		std::mutex* mt_lock, std::mutex* mt_stop_lock,
		std::list<std::shared_ptr<FireworksDesc>>* p_add_demo_data,
		std::list<DemoData>* p_demo_data,
		const UINT* frame_number,
		const bool* stop_flg,
		std::mutex* mt_clear,
		std::vector<AffectObjects> affect_objects
	) noexcept;
	bool FWDescImGuiUpdate(FireworksDesc* desc, const std::vector<KGL::DescriptorHandle>& srv_gui_handles);
	static float GetMaxTime(const FireworksDesc& desc);
public:
	FWDESC_STATE DescImGuiUpdate(
		KGL::ComPtrC<ID3D12Device> device, Desc* desc, const ParticleParent* p_parent, bool* edited,
		const std::vector<KGL::DescriptorHandle>& srv_gui_handles);
	static HRESULT Export(const std::filesystem::path& path, std::string file_name, std::shared_ptr<FireworksDesc> desc) noexcept;
	void CreateSelectDemo(KGL::ComPtrC<ID3D12Device> device, const ParticleParent* p_parent);
	FCManager(const std::filesystem::path& directory, const std::vector<std::shared_ptr<KGL::Texture>>& textures);
	~FCManager();
	HRESULT Load(const std::filesystem::path& directory) noexcept;
	HRESULT Load() noexcept { return Load(directory->GetPath()); }
	HRESULT ImGuiUpdate(KGL::ComPtrC<ID3D12Device> device, const ParticleParent* p_parent,
		const std::vector<KGL::DescriptorHandle>& srv_gui_handles) noexcept;
	HRESULT Update(float update_time) noexcept;
	std::shared_ptr<FireworksDesc> GetSelectDesc() const noexcept { return select_desc; }
	void Render(KGL::ComPtr<ID3D12GraphicsCommandList> cmd_list) noexcept;
	size_t Size() const;
	const std::map<const std::string, std::shared_ptr<FireworksDesc>>& GetDescList() const noexcept { return desc_list; }
};