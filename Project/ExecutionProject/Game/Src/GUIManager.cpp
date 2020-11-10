#include "../Hrd/GUIManager.h"
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
		imgui_srv_handles.push_back(handle);

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
			imgui_srv_desc->Free(*handle);
		}
	}
}

void GUIManager::Init()
{
	time_scale = 1.f;

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

void GUIManager::Update(const DirectX::XMUINT2& rt_resolution)
{
	static bool open_flg = true;
	if (ImGui::Begin("GUIManager", &open_flg, main_window_flag))
	{
		// ウィンドウの位置とサイズを固定
		ImGui::SetWindowPos(ImVec2(0.f, (SCAST<float>(rt_resolution.y) / 5) * 4), ImGuiCond_::ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(SCAST<float>(rt_resolution.x), SCAST<float>(rt_resolution.y) / 5), ImGuiCond_::ImGuiCond_Once);
	
		ImGui::Button(u8"Play");
		ImGui::SameLine();
		ImGui::Button(u8"Stop");
	}
	ImGui::End();
}