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
		// ���O�ύX
		if (obj_desc.set_name.size() < 64u)
			obj_desc.set_name.resize(64u);
		ImGui::InputText(("##" + std::to_string(RCAST<intptr_t>(this))).c_str(), obj_desc.set_name.data(), obj_desc.set_name.size());
		ImGui::SameLine();
		if (ImGui::Button(u8"���O�ύX") && !obj_desc.set_name.empty())
		{
			obj_desc.name = obj_desc.set_name;
		}

		// �G�t�F�N�g�ύX
		{
			// 2�Ԗڂ̃p�����[�^�[�́A�R���{���J���O�Ƀv���r���[����郉�x���ł��B
			if (ImGui::BeginCombo((u8"�G�t�F�N�g�ύX##" + std::to_string(RCAST<intptr_t>(this))).c_str(), obj_desc.set_name.c_str()))
			{
				for (const auto& desc : desc_list)
				{
					// �I�u�W�F�N�g�̊O���܂��͓����ɁA�I����e���D���Ȃ悤�ɕۑ��ł��܂�
					bool is_selected = (obj_desc.set_name == desc.first);
					if (ImGui::Selectable(desc.first.c_str(), is_selected))
					{
						obj_desc.set_name = desc.first;
						fw_desc = desc.second;
					}
					if (is_selected)
						// �R���{���J���Ƃ��ɏ����t�H�[�J�X��ݒ�ł��܂��i�L�[�{�[�h�i�r�Q�[�V�����T�|�[�g�̏ꍇ��+���X�N���[�����܂��j
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
		// ���O�ύX
		if (set_name.size() < 64u)
			set_name.resize(64u);
		ImGui::InputText(("##" + std::to_string(RCAST<intptr_t>(this))).c_str(), set_name.data(), set_name.size());
		ImGui::SameLine();
		if (ImGui::Button(u8"���O�ύX") && !set_name.empty())
		{
			name = set_name;
		}

		// �V�K�쐬
		{
			// 2�Ԗڂ̃p�����[�^�[�́A�R���{���J���O�Ƀv���r���[����郉�x���ł��B
			if (ImGui::BeginCombo(("##" + std::to_string(RCAST<intptr_t>(this))).c_str(), create_name.c_str()))
			{
				for (const auto& desc : desc_list)
				{
					// �I�u�W�F�N�g�̊O���܂��͓����ɁA�I����e���D���Ȃ悤�ɕۑ��ł��܂�
					bool is_selected = (create_name == desc.first);
					if (ImGui::Selectable(desc.first.c_str(), is_selected))
					{
						create_name = desc.first;
					}
					if (is_selected)
						// �R���{���J���Ƃ��ɏ����t�H�[�J�X��ݒ�ł��܂��i�L�[�{�[�h�i�r�Q�[�V�����T�|�[�g�̏ꍇ��+���X�N���[�����܂��j
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		ImGui::SameLine();
		if (desc_list.count(create_name) == 1u)
		{
			if (ImGui::Button(u8"�V�K�쐬"))
			{
				Create(create_name, desc_list.at(create_name));
				create_name.clear();
			}
		}
		else
		{
			ImGui::Text(u8"�V�K�쐬");
		}

		if (!objects.empty())
		{
			if (ImGui::TreeNode(u8"�ꗗ"))
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

	// spawner��ǂݍ���
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
	// �I�𒆂�FS����Fireworks�𔭐�������
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
			if (ImGui::BeginMenu(u8"�t�@�C��"))
			{
				if (ImGui::BeginMenu(u8"�V�K�쐬"))
				{
					if (set_name.size() < 64u)
						set_name.resize(64u);
					ImGui::InputText(("##" + std::to_string(RCAST<intptr_t>(this))).c_str(), set_name.data(), set_name.size());
					ImGui::SameLine();
					// �V�KFS��ǉ�
					if (ImGui::Button(u8"�쐬") && !set_name.empty())
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
			if (ImGui::TreeNode(u8"�ꗗ"))
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