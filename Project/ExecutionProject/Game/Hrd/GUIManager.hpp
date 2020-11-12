#pragma once

#include <DirectXMath.h>
#include <Dx12/Texture.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <imgui.h>
#include <map>
#include <array>
#include <Helper/Timer.hpp>

class FCManager;
class ParticleManager;
class FSManager;
class SkyManager;
class MSAASelector;
class FXAAManager;
struct ParticleParent;

class GUIManager
{
public:
	struct Desc
	{
		std::shared_ptr<FCManager>			fc_mgr;
		std::shared_ptr<FSManager>			fs_mgr;
		std::shared_ptr<ParticleManager>	main_ptc_mgr;
		std::shared_ptr<ParticleManager>	player_ptc_mgr;
		std::shared_ptr<SkyManager>			sky_mgr;
		std::shared_ptr<MSAASelector>		msaa_selector;
		std::shared_ptr<FXAAManager>		fxaa_mgr;
	};
	enum class SUB_WINDOW_TYPE : UINT
	{
		NONE,
		SKY,
		FW_EDITOR,
		FW_PARAM
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

	std::vector<std::shared_ptr<KGL::Texture>> textures;
	std::array<SUB_WINDOW_TYPE, 2u>	sub_windows;

	float	time_scale;	// 時間倍速
	bool	time_stop;
public:
	bool	spawn_fireworks;

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
public:
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