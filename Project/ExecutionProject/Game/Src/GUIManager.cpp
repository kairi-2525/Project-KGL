#include "../Hrd/GUIManager.hpp"
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
}

#define FULL_UV { 0.f, 0.f }, { 1.f, 1.f }

void GUIManager::Update(const DirectX::XMUINT2& rt_resolution)
{
	static bool open_flg = true;
	if (ImGui::Begin("GUIManager", &open_flg, main_window_flag))
	{
		UINT idx = 0u;

		// ウィンドウの位置とサイズを固定
		ImGui::SetWindowPos(ImVec2(0.f, (SCAST<float>(rt_resolution.y) / 5) * 4), ImGuiCond_::ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(SCAST<float>(rt_resolution.x), SCAST<float>(rt_resolution.y) / 5), ImGuiCond_::ImGuiCond_Once);
		
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
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, pause_b_tint_col))
			{
				time_stop = false;
			}
		}
		// Play Button
		else if (imgui_srv_handles.count(NAME_PLAY_BUTTON) == 1u)
		{
			auto tex_id = (ImTextureID)imgui_srv_handles.at(NAME_PLAY_BUTTON)->Gpu().ptr;
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, play_b_tint_col))
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
		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::Text("List");
		ImVec2 ss_window_size = { 200.f, 200.f };
		if (ImGui::BeginChild("scrolling"), ss_window_size, true, ImGuiWindowFlags_NoTitleBar)
		{
			if (ImGui::Button("Sky"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::SKY))
					EraseSubWindow(SUB_WINDOW_TYPE::SKY);
				else
					SetSubWindow(SUB_WINDOW_TYPE::SKY);
			}
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
			ImGui::Button("Sky");
		}
		ImGui::EndChild();
		ImGui::EndGroup();
	}
	ImGui::End();

	// サブウィンドウ
	for (auto& sub_window : sub_windows)
	{
		switch (sub_window)
		{
			case SUB_WINDOW_TYPE::SKY:
			{
				if (BeginSubWindow(rt_resolution, 0))
				{
					desc.sky_mgr->UpdateGui();
				}
				ImGui::End();
			}
		}
	}
}

// サブウィンドウを座標などをセットした状態でBeginする
bool GUIManager::BeginSubWindow(const DirectX::XMUINT2& rt_resolution, UINT num)
{
	static bool open_flg = true;
	const ImVec2 base_size = { SCAST<float>(rt_resolution.x) / 5, SCAST<float>(rt_resolution.y) / 5.f };
	switch (num)
	{
		case 0u:
		{
			if (ImGui::Begin("GUIManager Sub 0", &open_flg, main_window_flag))
			{
				ImGui::SetWindowPos(ImVec2(base_size.x * 4, 0.f), ImGuiCond_::ImGuiCond_Once);
				ImGui::SetWindowSize(ImVec2(base_size.x * 1, base_size.y * 4), ImGuiCond_::ImGuiCond_Once);
				return true;
			}
		}
	}
	return false;
}

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