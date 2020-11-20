#pragma once

#include <DirectXMath.h>
#include <Dx12/Texture.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <imgui.h>
#include <map>
#include <array>
#include <Helper/Timer.hpp>
#include "Bloom.hpp"
#include "DepthOfField.hpp"
#include "DebugMsg.hpp"

class FCManager;
class ParticleManager;
class FSManager;
class SkyManager;
class MSAASelector;
class FXAAManager;
struct ParticleParent;
class Fireworks;
class DebugManager;
struct RenderTargetResource;

class GUIManager
{
public:
	struct Desc
	{
		std::shared_ptr<FCManager>					fc_mgr;
		std::shared_ptr<FSManager>					fs_mgr;
		std::shared_ptr<ParticleManager>			main_ptc_mgr;
		std::shared_ptr<ParticleManager>			player_ptc_mgr;
		std::shared_ptr<SkyManager>					sky_mgr;

		std::shared_ptr<MSAASelector>				msaa_selector;
		std::shared_ptr<std::vector<std::string>>	msaa_combo_texts;
		std::shared_ptr<FXAAManager>				fxaa_mgr;
		std::shared_ptr<DebugManager>				debug_mgr;
		std::shared_ptr<BloomGenerator>				bloom_generator;
		std::shared_ptr<DOFGenerator>				dof_generator;

		std::shared_ptr<std::vector<Fireworks>> fireworks;
		std::shared_ptr<std::vector<Fireworks>> player_fireworks;

		std::shared_ptr<std::vector<RenderTargetResource>>	rt_resources;
	};
	enum class SUB_WINDOW_TYPE : UINT
	{
		NONE,
		SKY,
		FW_EDITOR,
		FW_PARAM,
		FW_SPAWNER,
		RT,
		OPTION,
		DEBUG
	};
private:
	static inline const std::string										NAME_PLAY_BUTTON = "T_6_continue_";
	static inline const std::string										NAME_PAUSE_BUTTON = "T_8_pause_";
	static inline const std::string										NAME_LEFT_BUTTON = "T_19_previoust2_";
	static inline const std::string										NAME_RIGHT_BUTTON = "T_18_next2_";

	static inline const float											TIME_ADD_SCALE = 0.1f;
private:
	Desc																desc;
	ImGuiWindowFlags													main_window_flag;
	std::shared_ptr<KGL::DescriptorManager>								imgui_srv_desc;
	std::map<std::string, std::shared_ptr<KGL::DescriptorHandle>>		imgui_srv_handles;

	std::vector<std::shared_ptr<KGL::Texture>>	textures;
	std::array<SUB_WINDOW_TYPE, 2u>				sub_windows;

	float										time_scale;	// 時間倍速
	std::shared_ptr<DebugMsgMgr>				debug_msg_mgr;

	std::string									search_text;
	std::list<std::string>						search_tags;
public:
	bool				spawn_fireworks;
	bool				ptc_wire;
	bool				ptc_dof;
	bool				dof_flg;
	bool				use_gpu;
	bool				time_stop;
	bool				sky_draw;
	UINT				ptc_vt_type;


	std::array<KGL::DescriptorHandle, BloomGenerator::RTV_MAX>	bl_c_imgui_handles;
	std::array<KGL::DescriptorHandle, BloomGenerator::RTV_MAX>	bl_h_imgui_handles;
	std::array<KGL::DescriptorHandle, BloomGenerator::RTV_MAX>	bl_w_imgui_handles;
	KGL::DescriptorHandle										bl_bloom_imgui_handle;
	std::array<KGL::DescriptorHandle, DOFGenerator::RTV_MAX>	dof_imgui_handles;

	// タイムカウンター
	KGL::Timer											tm_update;
	KGL::Timer											tm_render;
	KGL::Timer											tm_ptc_sort;
	KGL::Timer											tm_ptc_update_gpu;
	KGL::Timer											tm_ptc_update_cpu;
	UINT64												ct_ptc_total;
	UINT64												ct_ptc_frame;
	UINT64												ct_fw;
	UINT64												ct_ptc_total_max;
	UINT64												ct_ptc_frame_max;
	UINT64												ct_fw_max;
private:
	bool SetSubWindow(SUB_WINDOW_TYPE type, bool force = true);
	void SetSubWindow(SUB_WINDOW_TYPE type, UINT num);
	bool EraseSubWindow(SUB_WINDOW_TYPE type);
	bool HasSubWindow(SUB_WINDOW_TYPE type);
	void PackSubWindow();	// 前に詰める
	void SkyRender();
	bool BeginSubWindow(const DirectX::XMUINT2& rt_resolution, UINT num, ImGuiWindowFlags flags, std::string title = "");
	
	// ウィンドウ更新
	void UpdatePtcOption(const DirectX::XMUINT2& rt_resolution);
	void UpdateRtOption();
public:
	static void HelperZoomImage(ImTextureID image, ImVec2 tex_size);
	static void HelperTimer(const std::string& title, const KGL::Timer& timer, KGL::Timer::SEC sec_type = KGL::Timer::SEC::MICRO);
	static void HelperCounter(const std::string& title, UINT64 count, UINT64* max_count);
public:
	GUIManager(ComPtrC<ID3D12Device> device, std::shared_ptr<KGL::DescriptorManager> imgui_descriptor);
	~GUIManager();
	void Init();
	void Update(
		const DirectX::XMUINT2& rt_resolution,
		const ParticleParent* p_parent,
		const std::vector<KGL::DescriptorHandle>& srv_gui_handles
	);
	void CounterReset();

	void SetDesc(Desc desc) { this->desc = desc; }
	float GetTimeScale() const
	{
		if (time_stop)
		{
			return 0.f;
		}
		return time_scale;
	}
	// ウィンドウの無い領域のサイズを返す(左上詰め)
	DirectX::XMUINT2 GetNoWindowSpace(const DirectX::XMUINT2& rt_resolution) const;
};