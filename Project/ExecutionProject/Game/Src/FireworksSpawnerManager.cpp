#include "../Hrd/FireworksSpawnerManager.hpp"
#include <Cereal/archives/binary.hpp>
#include <Cereal/types/vector.hpp>
#include <Cereal/types/memory.hpp>
#include <fstream>
#include <imgui.h>

void FS_Obj::Init(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	if (desc_list.count(obj_desc.fw_name) == 1u)
	{
		fw_desc = desc_list.at(obj_desc.fw_name);
	}
}

void FS_Obj::Update(float update_time, std::vector<Fireworks>* pout_fireworks)
{

}

void FS_Obj::GUIUpdate(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	if (ImGui::TreeNode((obj_desc.name + "##" + std::to_string(RCAST<intptr_t>(this))).c_str()))
	{
		// 名前変更
		if (obj_desc.set_name.size() < 64u)
			obj_desc.set_name.resize(64u);
		ImGui::InputText(("##" + std::to_string(RCAST<intptr_t>(this))).c_str(), obj_desc.set_name.data(), obj_desc.set_name.size());
		ImGui::SameLine();
		if (ImGui::Button(u8"名前変更") && !obj_desc.set_name.empty())
		{
			obj_desc.name = obj_desc.set_name;
		}

		// エフェクト変更
		{
			// 2番目のパラメーターは、コンボを開く前にプレビューされるラベルです。
			if (ImGui::BeginCombo((u8"エフェクト変更##" + std::to_string(RCAST<intptr_t>(this))).c_str(), obj_desc.set_name.c_str()))
			{
				for (const auto& desc : desc_list)
				{
					// オブジェクトの外側または内側に、選択内容を好きなように保存できます
					bool is_selected = (obj_desc.set_name == desc.first);
					if (ImGui::Selectable(desc.first.c_str(), is_selected))
					{
						obj_desc.set_name = desc.first;
						fw_desc = desc.second;
					}
					if (is_selected)
						// コンボを開くときに初期フォーカスを設定できます（キーボードナビゲーションサポートの場合は+をスクロールします）
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}

		ImGui::TreePop();
	}
}

void FS::Init(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	for (auto& obj : objects)
	{
		obj.Init(desc_list);
	}
}

void FS::Create(std::string name, std::shared_ptr<FireworksDesc> desc)
{
	auto& it = objects.emplace_back();
	it.obj_desc.fw_name = name;
	it.obj_desc.name = name + "'s spawner";
	it.obj_desc.set_name = it.obj_desc.name;
	it.SetDesc(desc);
}

void FS::Update(float update_time, std::vector<Fireworks>* pout_fireworks)
{
	for (auto& obj : objects)
	{
		obj.Update(update_time, pout_fireworks);
	}
}

void FS::GUIUpdate(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	if (ImGui::TreeNode((name + "##" + std::to_string(RCAST<intptr_t>(this))).c_str()))
	{
		// 名前変更
		if (set_name.size() < 64u)
			set_name.resize(64u);
		ImGui::InputText(("##" + std::to_string(RCAST<intptr_t>(this))).c_str(), set_name.data(), set_name.size());
		ImGui::SameLine();
		if (ImGui::Button(u8"名前変更") && !set_name.empty())
		{
			name = set_name;
		}

		// 新規作成
		{
			// 2番目のパラメーターは、コンボを開く前にプレビューされるラベルです。
			if (ImGui::BeginCombo(("##" + std::to_string(RCAST<intptr_t>(this))).c_str(), create_name.c_str()))
			{
				for (const auto& desc : desc_list)
				{
					// オブジェクトの外側または内側に、選択内容を好きなように保存できます
					bool is_selected = (create_name == desc.first);
					if (ImGui::Selectable(desc.first.c_str(), is_selected))
					{
						create_name = desc.first;
					}
					if (is_selected)
						// コンボを開くときに初期フォーカスを設定できます（キーボードナビゲーションサポートの場合は+をスクロールします）
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		ImGui::SameLine();
		if (desc_list.count(create_name) == 1u)
		{
			if (ImGui::Button(u8"新規作成"))
			{
				Create(create_name, desc_list.at(create_name));
				create_name.clear();
			}
		}
		else
		{
			ImGui::Text(u8"新規作成");
		}

		if (!objects.empty())
		{
			if (ImGui::TreeNode(u8"一覧"))
			{
				for (auto& obj : objects)
				{
					obj.GUIUpdate(desc_list);
				}
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}
}

FSManager::FSManager(const std::filesystem::path& path, const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list) noexcept :
	KGL::Directory(path)
{
	KGL::Files files = GetFiles(".bin");
	const std::string dir_path = GetPath().string();
	const size_t file_size = files.size();

	// spawnerを読み込む
	for (size_t i = 0u; i < file_size; i++)
	{
		std::ifstream ifs(dir_path + files[i].string(), std::ios::binary);
		if (ifs.is_open())
		{
			const auto& file_name = files[i].filename().stem();
			std::shared_ptr<FS> desc;
			cereal::BinaryInputArchive i_archive(ifs);
			i_archive(KGL_NVP(file_name.string(), desc));
			desc->Init(desc_list);
			fs_list.push_back(desc);
		}
	}
}

void FSManager::Create(std::string name)
{
	select_fs = std::make_shared<FS>();
	select_fs->name = name;
	select_fs->set_name = name;
	fs_list.push_back(select_fs);
}

void FSManager::Update(float update_time, std::vector<Fireworks>* pout_fireworks)
{
	// 選択中のFSからFireworksを発生させる
	if (select_fs)
	{
		select_fs->Update(update_time, pout_fireworks);
	}
}

void FSManager::GUIUpdate(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	if (ImGui::Begin("Fireworks Spawner", nullptr, ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu(u8"ファイル"))
			{
				if (ImGui::BeginMenu(u8"新規作成"))
				{
					if (set_name.size() < 64u)
						set_name.resize(64u);
					ImGui::InputText(("##" + std::to_string(RCAST<intptr_t>(this))).c_str(), set_name.data(), set_name.size());
					ImGui::SameLine();
					// 新規FSを追加
					if (ImGui::Button(u8"作成") && !set_name.empty())
					{
						Create(set_name);
						set_name.clear();
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		if (!fs_list.empty())
		{
			if (ImGui::TreeNode(u8"一覧"))
			{
				for (auto& fs : fs_list)
				{
					fs->GUIUpdate(desc_list);
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
}