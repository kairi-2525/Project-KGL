#pragma once

#include <DirectXMath.h>
#include <Dx12/Texture.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <imgui.h>
#include <map>
#include <array>

class FCManager;
class ParticleManager;
class FSManager;
class SkyManager;
class MSAASelector;
class FXAAManager;

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

	float	time_scale;	// ŽžŠÔ”{‘¬
	bool	time_stop;

	std::array<SUB_WINDOW_TYPE, 2u>	sub_windows;
public:
	bool	spawn_fireworks;
private:
	bool SetSubWindow(SUB_WINDOW_TYPE type, bool force = true);
	bool EraseSubWindow(SUB_WINDOW_TYPE type);
	bool HasSubWindow(SUB_WINDOW_TYPE type);
	void SkyRender();
	bool BeginSubWindow(const DirectX::XMUINT2& rt_resolution, UINT num);
public:
	GUIManager(ComPtrC<ID3D12Device> device, std::shared_ptr<KGL::DescriptorManager> imgui_descriptor);
	~GUIManager();
	void Init();
	void Update(const DirectX::XMUINT2& rt_resolution);

	void SetDesc(Desc desc) { this->desc = desc; }
	float GetTimeScale() const
	{
		if (time_stop)
		{
			return 0.f;
		}
		return time_scale;
	}
};