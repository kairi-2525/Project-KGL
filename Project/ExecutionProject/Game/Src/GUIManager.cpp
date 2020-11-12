#include "../Hrd/GUIManager.hpp"
#include "../Hrd/Fireworks.hpp"
#include "../Hrd/FireworkCerialManager.hpp"
#include "../Hrd/FireworksSpawnerManager.hpp"
#include "../Hrd/ParticleManager.hpp"
#include "../Hrd/SkyMap.hpp"
#include "../Hrd/MSAA.hpp"
#include "../Hrd/FXAA.hpp"

#include <Helper/Cast.hpp>
#include <Base/Directory.hpp>

GUIManager::GUIManager(ComPtrC<ID3D12Device> device, std::shared_ptr<KGL::DescriptorManager> imgui_descriptor)
{
	imgui_srv_desc = imgui_descriptor;

	KGL::Directory dir("./Assets/Textures/UI");

	const auto path = dir.GetPath().string() + "\\";
	const KGL::Files& files = dir.GetFiles(".png");

	textures.reserve(files.size());
	for (const auto& file_name : files)
	{
		auto tex = std::make_shared<KGL::Texture>(device, path + file_name.string());
		textures.push_back(tex);
		auto handle = std::make_shared<KGL::DescriptorHandle>(imgui_srv_desc->Alloc());
		imgui_srv_handles[file_name.stem().string()] = handle;

		// SRVを作成
		tex->CreateSRVHandle(handle);
	}

	Init();
}

GUIManager::~GUIManager()
{
	if (imgui_srv_desc)
	{
		for (auto& handle : imgui_srv_handles)
		{
			imgui_srv_desc->Free(*handle.second);
		}
	}
}

void GUIManager::Init()
{
	time_scale = 1.f;
	time_stop = false;
	spawn_fireworks = false;

	main_window_flag = ImGuiWindowFlags_::ImGuiWindowFlags_None;

	// 動かさない
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoMove;
	// サイズを変更させない
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoResize;
	// タイトルバー非表示
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar;
	// ダブルクリックによる最小化の無効化
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse;
	// スクロールバー非表示
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar;

	for (auto& sub_window : sub_windows)
	{
		sub_window = SUB_WINDOW_TYPE::NONE;
	}

	CounterReset();
}

#define FULL_UV { 0.f, 0.f }, { 1.f, 1.f }

void GUIManager::Update(
	const DirectX::XMUINT2& rt_resolution,
	const ParticleParent* p_parent,
	const std::vector<KGL::DescriptorHandle>& srv_gui_handles
)
{
	static bool open_flg = true;
	if (ImGui::Begin("GUIManager", &open_flg, main_window_flag))
	{
		UINT idx = 0u;

		// ウィンドウの位置とサイズを固定
		ImGui::SetWindowPos(ImVec2(0.f, (SCAST<float>(rt_resolution.y) / 5) * 4), ImGuiCond_::ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(SCAST<float>(rt_resolution.x), SCAST<float>(rt_resolution.y) / 5), ImGuiCond_::ImGuiCond_Once);
		
		ImGui::BeginGroup();
		const ImVec2 window_size = ImGui::GetWindowSize();
		const ImVec2 play_b_size = { window_size.x / 50, window_size.y / 10 };
		const ImVec4 pause_b_tint_col = { 0.4f, 1.0f, 0.4f, 1.f };
		const ImVec4 play_b_tint_col = { 1.f, 1.f, 1.f, 1.f };
		const ImVec4 play_b_bg_col = { 0.f, 0.f, 0.f, 0.f };

		// Left Button
		if (imgui_srv_handles.count(NAME_LEFT_BUTTON) == 1u)
		{
			auto tex_id = (ImTextureID)imgui_srv_handles.at(NAME_LEFT_BUTTON)->Gpu().ptr;
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, play_b_tint_col))
			{
				time_scale = (std::max)(0.f, time_scale - TIME_ADD_SCALE);
			}
		}
		ImGui::SameLine();
		// Pause Button
		if (time_stop && imgui_srv_handles.count(NAME_PAUSE_BUTTON) == 1u)
		{
			auto tex_id = (ImTextureID)imgui_srv_handles.at(NAME_PAUSE_BUTTON)->Gpu().ptr;
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, play_b_tint_col))
			{
				time_stop = false;
			}
		}
		// Play Button
		else if (imgui_srv_handles.count(NAME_PLAY_BUTTON) == 1u)
		{
			auto tex_id = (ImTextureID)imgui_srv_handles.at(NAME_PLAY_BUTTON)->Gpu().ptr;
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, pause_b_tint_col))
			{
				time_stop = true;
			}
		}
		ImGui::SameLine();
		// Right Button
		if (imgui_srv_handles.count(NAME_RIGHT_BUTTON) == 1u)
		{
			auto tex_id = (ImTextureID)imgui_srv_handles.at(NAME_RIGHT_BUTTON)->Gpu().ptr;
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, play_b_tint_col))
			{
				time_scale += TIME_ADD_SCALE;
			}
		}
		ImGui::SameLine();
		// Time Scale
		ImGui::Text("Time "); ImGui::SameLine();
		ImGui::PushItemWidth(play_b_size.x * 2.f);
		ImGui::InputFloat(("##" + std::to_string(idx++)).c_str(), &time_scale, 0.f, 1.f);
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Spawn Fireworks
		ImGui::Text("Spawn Fireworks"); ImGui::SameLine();
		ImGui::Checkbox(("##" + std::to_string(idx++)).c_str(), &spawn_fireworks);
		
		const ImVec2 info_window_size = { window_size.x / 4, -1.f };
		if (ImGui::BeginChild("Info Window", info_window_size, true, ImGuiWindowFlags_NoTitleBar))
		{
			ImGui::Text("FPS : %d", SCAST<int>(ImGui::GetIO().Framerate));
			HelperTimer("Update Count Total ", tm_update);
			HelperTimer("Render Count Total ", tm_render);
			HelperTimer("Particle Update Gpu", tm_ptc_update_gpu);
			HelperTimer("Particle Update Cpu", tm_ptc_update_cpu);
			HelperTimer("Particle Update Sort", tm_ptc_sort);

			HelperCounter("Particle Count Total", ct_ptc_total, &ct_ptc_total_max);
			HelperCounter("Firework Count Total", ct_fw, &ct_fw_max);
			HelperCounter("Particle Count Frame", ct_ptc_frame, &ct_ptc_frame_max);
		}
		ImGui::EndChild();
		ImGui::EndGroup();

		ImGui::SameLine();
		ImGui::NextColumn();

		// サブウィンドウ一覧
		const ImVec2 param_window_size = { window_size.x / 10, -1.f };
		if (ImGui::BeginChild("Param Window", param_window_size, true, ImGuiWindowFlags_NoTitleBar))
		{
			if (ImGui::Button("Sky"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::SKY))
					EraseSubWindow(SUB_WINDOW_TYPE::SKY);
				else
				{
					SetSubWindow(SUB_WINDOW_TYPE::SKY, 0u);
					SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
				}
			}
			if (ImGui::Button("Particle"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::FW_EDITOR))
				{
					EraseSubWindow(SUB_WINDOW_TYPE::FW_EDITOR);
					EraseSubWindow(SUB_WINDOW_TYPE::FW_PARAM);
				}
				else
				{
					SetSubWindow(SUB_WINDOW_TYPE::FW_EDITOR, 0u);
					SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
				}
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::NextColumn();

		const ImVec2 ptc_demo_window_size = { -1.f, -1.f };
		if (ImGui::BeginChild("Ptc Demo Window", ptc_demo_window_size, true, ImGuiWindowFlags_NoTitleBar))
		{
			desc.fc_mgr->UpdateDemoGui();
		}
		ImGui::EndChild();
	}
	ImGui::End();

	// サブウィンドウを前に詰める
	PackSubWindow();
	// サブウィンドウ
	UINT sub_window_idx = 0u;

	ComPtr<ID3D12Device> device;
	imgui_srv_desc->Heap()->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));

	for (auto& sub_window : sub_windows)
	{
		switch (sub_window)
		{
			case SUB_WINDOW_TYPE::SKY:
			{
				if (BeginSubWindow(rt_resolution, sub_window_idx, main_window_flag))
				{
					desc.sky_mgr->UpdateGui();
				}
				ImGui::End(); break;
			}
			case SUB_WINDOW_TYPE::FW_EDITOR:
			{
				auto use_flg = main_window_flag;
				// メニューバーを表示
				use_flg |= ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar;
				if (BeginSubWindow(rt_resolution, sub_window_idx, use_flg))
				{
					desc.fc_mgr->UpdateGui(device, p_parent, srv_gui_handles);

					auto fc_select = desc.fc_mgr->GetSelectDesc();
					if (fc_select)
					{
						SetSubWindow(SUB_WINDOW_TYPE::FW_PARAM, 1u);
					}
					else
					{
						SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
					}
				}
				ImGui::End(); break;
			}
			case SUB_WINDOW_TYPE::FW_PARAM:
			{
				auto use_flg = main_window_flag;
				// タイトルバーを戻す
				use_flg &= (~ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
				// 横スクロールバーを追加
				use_flg |= ImGuiWindowFlags_::ImGuiWindowFlags_HorizontalScrollbar;
				// スクロールバー非表示を戻す
				use_flg &= (~ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar);
				auto fc_select = desc.fc_mgr->GetSelectDesc();
				if (fc_select)
				{
					auto title = desc.fc_mgr->GetName(fc_select);
					if (BeginSubWindow(rt_resolution, sub_window_idx, use_flg, title))
					{
						if (desc.fc_mgr->FWDescImGuiUpdate(fc_select.get(), srv_gui_handles))
						{
							desc.fc_mgr->CreateSelectDemo(device, p_parent);
						}
					}
					ImGui::End();
				}
				break;
			}
		}
		sub_window_idx++;
	}
}

// サブウィンドウを座標などをセットした状態でBeginする
bool GUIManager::BeginSubWindow(
	const DirectX::XMUINT2& rt_resolution,
	UINT num,
	ImGuiWindowFlags flags,
	std::string title)
{
	static bool open_flg = true;
	const ImVec2 base_size = { SCAST<float>(rt_resolution.x) / 5, SCAST<float>(rt_resolution.y) / 5.f };
	const ImVec2 window_size = ImVec2(base_size.x * 1, base_size.y * 4);
	UINT offset = 4 - num;

	ImGui::SetNextWindowContentSize(ImVec2(window_size.x * 2, 0.f));
	if (ImGui::Begin((title + "##" + std::to_string(num)).c_str(), &open_flg, flags))
	{
		ImGui::SetWindowPos(ImVec2(base_size.x * offset, 0.f), ImGuiCond_::ImGuiCond_Once);
		ImGui::SetWindowSize(window_size, ImGuiCond_::ImGuiCond_Once);
		return true;
	}
	return false;
}

// 空いている場所を探してサブウィンドウをセットする
// force で最後のウィンドウを強制的に置き換える
bool GUIManager::SetSubWindow(SUB_WINDOW_TYPE type, bool force)
{
	for (auto& sub_window : sub_windows)
	{
		if (sub_window == type)
		{
			return false;
		}
		if (sub_window == SUB_WINDOW_TYPE::NONE)
		{
			sub_window = type;
			return true;
		}
	}
	if (force)
	{
		sub_windows.back() = type;
		return true;
	}

	return false;
}

// num 番目のウィンドウを無理やり置き換える
void GUIManager::SetSubWindow(SUB_WINDOW_TYPE type, UINT num)
{
	if (sub_windows.size() > SCAST<size_t>(num))
	{
		sub_windows[num] = type;
	}
}

bool GUIManager::EraseSubWindow(SUB_WINDOW_TYPE type)
{
	for (auto& sub_window : sub_windows)
	{
		if (sub_window == type)
		{
			sub_window = SUB_WINDOW_TYPE::NONE;
			return true;
		}
	}
	return false;
}

// 指定したタイプが存在するか確認する
bool GUIManager::HasSubWindow(SUB_WINDOW_TYPE type)
{
	for (auto& sub_window : sub_windows)
	{
		if (sub_window == type)
		{
			return true;
		}
	}
	return false;
}

// NONEを見つけて前に詰める
void GUIManager::PackSubWindow()
{
	SUB_WINDOW_TYPE* p_none_window = nullptr;
	for (auto& sub_window : sub_windows)
	{
		if (sub_window == SUB_WINDOW_TYPE::NONE)
		{
			p_none_window = &sub_window;
		}
		else if (p_none_window)
		{
			*p_none_window = sub_window;
			sub_window = SUB_WINDOW_TYPE::NONE;
			p_none_window = &sub_window;
		}
	}
}

void GUIManager::CounterReset()
{
	ct_fw = ct_fw_max = 0u;
	ct_ptc_total = ct_ptc_total_max = 0u;
	ct_ptc_frame = ct_ptc_frame_max = 0u;

	tm_update.Clear();
	tm_render.Clear();
	tm_ptc_update_gpu.Clear();
	tm_ptc_update_cpu.Clear();
	tm_ptc_sort.Clear();
}

// Imgui Timerクラスヘルパー
void GUIManager::HelperTimer(const std::string& title, const KGL::Timer& timer, KGL::Timer::SEC sec_type)
{
	const std::string title_text = (title + " [ %5d ][ %5d ][ %5d ][ %5d ]");
	switch (sec_type)
	{
		case KGL::Timer::SEC::MICRO:
			ImGui::Text(title_text.c_str(), timer.Last().micro, timer.Average().micro, timer.Min().micro, timer.Max().micro);
			break;
		case KGL::Timer::SEC::NANO:
			ImGui::Text(title_text.c_str(), timer.Last().nano, timer.Average().nano, timer.Min().nano, timer.Max().nano);
			break;
		default:
			ImGui::Text(title_text.c_str(), timer.Last().milli, timer.Average().milli, timer.Min().milli, timer.Max().milli);
			break;
	}
}
// Imgui Counter ヘルパー
void GUIManager::HelperCounter(const std::string& title, UINT64 count, UINT64* max_count)
{
	if (max_count)
	{
		const std::string title_text = (title + " [ %5d ] : [ %5d ]");
		*max_count = (std::max)(count, *max_count);
		ImGui::Text(title_text.c_str(), count, *max_count);
	}
}

DirectX::XMUINT2 GUIManager::GetNoWindowSpace(const DirectX::XMUINT2& rt_resolution) const
{
	const ImVec2 base_size = { SCAST<float>(rt_resolution.x) / 5, SCAST<float>(rt_resolution.y) / 5 };
	auto result = rt_resolution;
	UINT offset = 0u;
	UINT i = 0u;
	for (const auto& sub_window : sub_windows)
	{
		i++;
		if (sub_window != SUB_WINDOW_TYPE::NONE)
			offset = i;
	}

	result.x -= (base_size.x * offset);
	result.y -= (base_size.y);

	return result;
}