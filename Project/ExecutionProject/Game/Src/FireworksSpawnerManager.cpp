#include "../Hrd/FireworksSpawnerManager.hpp"
#include "../Hrd/Fireworks.hpp"
#include <Cereal/archives/binary.hpp>
#include <Cereal/types/vector.hpp>
#include <Cereal/types/memory.hpp>
#include <fstream>
#include <imgui.h>
#include <imgui_helper.h>
#include <random>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Color.hpp>

#define MINMAX_TEXT u8"�����_���ŕϓ�����(x = min, y = max)"

void FS_Obj::Init()
{
	std::random_device rd;
	std::mt19937 mt(rd());
	using rmd_float = std::uniform_real_distribution<float>;

	start_time = rmd_float(obj_desc.start_time.x, obj_desc.start_time.y)(mt);
}

void FS_Obj::Init(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	if (desc_list.count(obj_desc.fw_name) == 1u)
	{
		fw_desc = desc_list.at(obj_desc.fw_name);
	}
	Init();
}

void FS_Obj::SetRandomColor(FireworksDesc* desc)
{
	RCHECK(!desc, "FS_Obj::SetRandomColor �� desc �� nullptr");
	std::random_device rd;
	std::mt19937 mt(rd());
	using rmd_float = std::uniform_real_distribution<float>;
	static rmd_float rmd_color_h(0.f, 360.f);

	for (auto& effect : desc->effects)
	{
		if (effect.has_child)
		{
			SetRandomColor(&effect.child);
		}
		else
		{
			auto hsv_bc = KGL::ConvertToHSL(effect.begin_color);
			auto hsv_ec = KGL::ConvertToHSL(effect.end_color);
			auto hsv_erc = KGL::ConvertToHSL(effect.erase_color);

			float add_h = rmd_color_h(mt);

			hsv_bc.x += add_h;
			if (360.f <= hsv_bc.x)
				hsv_bc.x -= 360.f;
			hsv_ec.x += add_h;
			if (360.f <= hsv_ec.x)
				hsv_ec.x -= 360.f;
			hsv_erc.x += add_h;
			if (360.f <= hsv_erc.x)
				hsv_erc.x -= 360.f;

			effect.begin_color = KGL::ConvertToRGB(hsv_bc);
			effect.end_color = KGL::ConvertToRGB(hsv_ec);
			effect.erase_color = KGL::ConvertToRGB(hsv_erc);
		}
	}
}

// ������Fireworks�𐶐�����
void FS_Obj::Update(float update_time, std::vector<Fireworks>* pout_fireworks)
{
	std::random_device rd;
	std::mt19937 mt(rd());
	using rmd_float = std::uniform_real_distribution<float>;
	using namespace DirectX;

	const bool before_not_started = !(start_time < 0.f);
	start_time -= update_time;
	if (start_time <= 0.f)
	{
		if (before_not_started)
		{
			time = rmd_float(obj_desc.time.x, obj_desc.time.y)(mt);
			update_time = -start_time;
			spawn_late = 1.f / rmd_float(obj_desc.spawn_late.x, obj_desc.spawn_late.y)(mt);
			spawn_late = (std::max)(spawn_late, FLT_MIN);
			counter = 0.f;
		}
		// ���̊֐����Ŏg�p����X�V����(�]�蕪�͍ċA�����ŏ�������)
		float fnc_update_time = update_time;
		if (time < update_time && !obj_desc.infinity)
		{
			fnc_update_time = time;

			start_time = rmd_float(obj_desc.wait_time.x, obj_desc.wait_time.y)(mt);
		}
		update_time -= fnc_update_time;
		time -= fnc_update_time;
		counter += fnc_update_time;

		if (counter < spawn_late)
		{
			return;
		}

		auto desc = *fw_desc;

		while (counter >= spawn_late)
		{
			// ��񕪂̐������Ԃ����炷
			counter -= spawn_late;
			
			desc.pos = { 0.f, 0.f, 0.f };

			rmd_float rmd_spawn_power(obj_desc.spawn_power.x, obj_desc.spawn_power.y);
			desc.velocity = { 0.f, desc.speed * rmd_spawn_power(mt), 0.f };

			rmd_float rmdpos(-obj_desc.spawn_radius, +obj_desc.spawn_radius);
			desc.pos.x += rmdpos(mt);
			desc.pos.z += rmdpos(mt);

			// �ˏo�p�x�̃����_���l������
			XMFLOAT2 nm_angle;
			nm_angle.x = XMConvertToRadians(obj_desc.spawn_angle.x) / XM_PI;
			nm_angle.y = XMConvertToRadians(obj_desc.spawn_angle.y) / XM_PI;
			rmd_float rmdangle(nm_angle.x, nm_angle.y);
			static rmd_float rmdangle360(0.f, XM_2PI);
			constexpr float radian90f = XMConvertToRadians(90.f);

			// �����_���Ȏˏo�x�N�g�����v�Z
			XMVECTOR right_axis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
			float side_angle = asinf((2.f * rmdangle(mt)) - 1.f) + radian90f;
			XMVECTOR side_axis;
			XMVECTOR velocity = XMLoadFloat3(&desc.velocity);
			XMVECTOR axis = XMVector3Normalize(velocity);

			if (XMVector3Length(XMVectorSubtract(right_axis, axis)).m128_f32[0] <= FLT_EPSILON)
				side_axis = XMVector3Normalize(XMVector3Cross(axis, XMVectorSet(0.f, 1.f, 0.f, 0.f)));
			else
				side_axis = XMVector3Normalize(XMVector3Cross(axis, right_axis));
			XMMATRIX R = XMMatrixRotationAxis(side_axis, side_angle);
			R *= XMMatrixRotationAxis(axis, rmdangle360(mt));
			XMVECTOR spawn_v = XMVector3Transform(axis, R);
			XMStoreFloat3(&desc.velocity, spawn_v * XMVector3Length(velocity));

			// �����_���ȐF���Z�b�g(HSV��Ԃ�H�̂ݕύX)
			SetRandomColor(&desc);

			pout_fireworks->emplace_back(desc);
		}

		// �ċA����
		return Update(update_time, pout_fireworks);
	}
}

bool FS_Obj::GUIUpdate(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	bool result = true;
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
		if (ImGui::Button(u8"�폜"))
		{
			result = false;
		}

		// �G�t�F�N�g�ύX
		{
			// 2�Ԗڂ̃p�����[�^�[�́A�R���{���J���O�Ƀv���r���[����郉�x���ł��B
			if (ImGui::BeginCombo((u8"�G�t�F�N�g�ύX##" + std::to_string(RCAST<intptr_t>(this))).c_str(), obj_desc.fw_name.c_str()))
			{
				for (const auto& desc : desc_list)
				{
					// �I�u�W�F�N�g�̊O���܂��͓����ɁA�I����e���D���Ȃ悤�ɕۑ��ł��܂�
					bool is_selected = (obj_desc.fw_name == desc.first);
					if (ImGui::Selectable(desc.first.c_str(), is_selected))
					{
						obj_desc.fw_name = desc.first;
						fw_desc = desc.second;
					}
					if (is_selected)
						// �R���{���J���Ƃ��ɏ����t�H�[�J�X��ݒ�ł��܂��i�L�[�{�[�h�i�r�Q�[�V�����T�|�[�g�̏ꍇ��+���X�N���[�����܂��j
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}

		// �e��p�����[�^�[�ύX
		ImGui::InputFloat2(u8"�����J�n����", (float*)&obj_desc.start_time);
		obj_desc.start_time.y = (std::max)(obj_desc.start_time.x, obj_desc.start_time.y);
		ImGui::SameLine(); HelpMarker(u8"���̎��Ԃ��o�ߌ�p�[�e�B�N���̐������J�n���܂��B\n" MINMAX_TEXT);

		if (obj_desc.infinity)
		{
			ImGui::Checkbox(u8"����", &obj_desc.infinity);
			ImGui::SameLine(); HelpMarker(u8"�i���Ƀp�[�e�B�N���𐶐��������܂��B");
		}
		else
		{
			ImGui::InputFloat2(u8"��������", (float*)&obj_desc.time);
			obj_desc.time.y = (std::max)(obj_desc.time.x, obj_desc.time.y);
			if (obj_desc.time.y < 1.f / obj_desc.spawn_late.y)
			{
				obj_desc.spawn_late.y = 1.f / obj_desc.time.y;
			}
			ImGui::SameLine(); ImGui::Checkbox(u8"����", &obj_desc.infinity);
			ImGui::SameLine(); HelpMarker(u8"���̎��Ԃ����p�[�e�B�N���𐶐����܂��B\n" MINMAX_TEXT);
		}

		ImGui::InputFloat2(u8"�ҋ@����", (float*)&obj_desc.wait_time);
		obj_desc.wait_time.y = (std::max)(obj_desc.wait_time.x, obj_desc.wait_time.y);
		ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���̐������ĊJ����܂ł̎��ԁB\n" MINMAX_TEXT);

		ImGui::InputFloat(u8"�����͈�", (float*)&obj_desc.spawn_radius);
		obj_desc.spawn_radius = (std::max)(0.f, obj_desc.spawn_radius);
		ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���𐶐����锼�a");

		ImGui::InputFloat2(u8"�ˏo�p�x", (float*)&obj_desc.spawn_angle);
		obj_desc.spawn_angle.y = (std::max)(obj_desc.spawn_angle.x, obj_desc.spawn_angle.y);
		ImGui::SameLine(); HelpMarker(u8"�^�ォ�炸�炷�p�x(deg)\n" MINMAX_TEXT);

		ImGui::InputFloat2(u8"�������[�g", (float*)&obj_desc.spawn_late);
		obj_desc.spawn_late.y = (std::max)(obj_desc.spawn_late.x, obj_desc.spawn_late.y);
		obj_desc.time.y = (std::max)(obj_desc.time.y, 1.f / obj_desc.spawn_late.y);
		obj_desc.time.x = (std::min)(obj_desc.time.x, obj_desc.time.y);
		ImGui::SameLine(); HelpMarker(u8"1�b�Ԃɉ��������邩�B\n(�ҋ@���Ԃ�����ōX�V)\n" MINMAX_TEXT);

		ImGui::InputFloat2(u8"�ˏo���x�X�P�[��", (float*)&obj_desc.spawn_power);
		obj_desc.spawn_power.y = (std::max)(obj_desc.spawn_power.x, obj_desc.spawn_power.y);
		ImGui::SameLine(); HelpMarker(u8"���̎ˏo���x���X�P�[���{���܂�\n(���̑��x��Fireworks�������Ă���)\n" MINMAX_TEXT);

		ImGui::TreePop();
	}

	return result;
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
	it.Init();
}

void FS::Update(float update_time, std::vector<Fireworks>* pout_fireworks)
{
	for (auto& obj : objects)
	{
		obj.Update(update_time, pout_fireworks);
	}
}

bool FS::GUIUpdate(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	bool result = true;
	const std::string ptr_str0 = std::to_string(RCAST<intptr_t>(this));
	const std::string ptr_str1 = std::to_string(1 + RCAST<intptr_t>(this));
	if (ImGui::TreeNode((name + "##" + ptr_str0).c_str()))
	{
		// ���O�ύX
		if (set_name.size() < 64u)
			set_name.resize(64u);
		ImGui::InputText(("##" + ptr_str0).c_str(), set_name.data(), set_name.size());
		ImGui::SameLine();
		if (ImGui::Button(u8"���O�ύX") && !set_name.empty())
		{
			name = set_name;
		}

		if (ImGui::Button(u8"�폜"))
		{
			result = false;
		}

		// �V�K�쐬
		{
			// 2�Ԗڂ̃p�����[�^�[�́A�R���{���J���O�Ƀv���r���[����郉�x���ł��B
			if (ImGui::BeginCombo(("##" + ptr_str1).c_str(), create_name.c_str()))
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
				for (auto itr = objects.begin(); itr != objects.end();)
				{
					if (!itr->GUIUpdate(desc_list))
					{
						itr = objects.erase(itr);
					}
					else
					{
						itr++;
					}
				}
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}

	return result;
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
			i_archive(KGL_NVP(file_name.stem().string(), desc));
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

//	�I�𒆂�FS�������o��
void FSManager::Export()
{
	if (select_fs)
	{
		const std::string dir_path = GetPath().string();

		auto file_name = select_fs->name;

		auto pos = file_name.find('\0');
		if (std::string::npos != pos)
			file_name.erase(pos, file_name.size());

		auto file_pass = dir_path + file_name + ".bin";

		std::ofstream ofs(file_pass, std::ios::binary);
		if (ofs.is_open())
		{
			cereal::BinaryOutputArchive o_archive(ofs);
			o_archive(KGL_NVP(select_fs->name, select_fs));
		}
	}
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
				if (select_fs)
				{
					if (ImGui::BeginMenu(u8"�����o��"))
					{
						if (ImGui::Button(("Export " + select_fs->name).c_str()))
						{
							Export();
						}
						ImGui::EndMenu();
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		if (!fs_list.empty())
		{
			if (ImGui::TreeNode(u8"�ꗗ"))
			{
				UINT i = 0u;
				for (auto itr = fs_list.begin(); itr != fs_list.end();)
				{
					if ((*itr) == select_fs)
					{
						if (ImGui::Button((u8"����##" + std::to_string(i)).c_str()))
						{
							select_fs = nullptr;
						}
					}
					else
					{
						if (ImGui::Button((u8"�I��##" + std::to_string(i)).c_str()))
						{
							select_fs = (*itr);
						}
					}
					ImGui::SameLine();
					if ((*itr)->GUIUpdate(desc_list))
					{
						itr++;
					}
					else
					{
						itr = fs_list.erase(itr);
					}
					i++;
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
}