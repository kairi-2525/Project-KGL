#include "../Hrd/FireworkCerialManager.hpp"
#include "../Hrd/Fireworks.hpp"
#include <algorithm>
#include <Cereal/archives/binary.hpp>
#include <Cereal/types/vector.hpp>
#include <Cereal/types/memory.hpp>
#include <imgui.h>
#include <fstream>
#include "../Hrd/Particle.hpp"

static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

FCManager::FCManager(const std::filesystem::path& directory)
{
	Load(directory);
}

HRESULT FCManager::Load(const std::filesystem::path& directory) noexcept
{
	this->directory = std::make_shared<KGL::Directory>(directory);
	HRESULT hr = ReloadDesc();
#if 1
	desc_list["A"] = std::make_shared<FireworksDesc>(FIREWORK_EFFECTS::Get(0));
	desc_list["B"] = std::make_shared<FireworksDesc>(FIREWORK_EFFECTS::Get(1));
	desc_list["C"] = std::make_shared<FireworksDesc>(FIREWORK_EFFECTS::Get(2));
	desc_list["D"] = std::make_shared<FireworksDesc>(FIREWORK_EFFECTS::Get(3));
#endif
	return hr;
}

HRESULT FCManager::ReloadDesc() noexcept
{
	desc_list.clear();
	select_desc = nullptr;

	const auto& files = directory->GetFiles(".bin");
	const size_t files_size = files.size();
	for (const auto& file : files)
	{
		std::ifstream ifs(directory->GetPath().string() + file.string(), std::ios::binary);
		if (ifs.is_open())
		{
			const auto& file_name = file.filename().stem();
			auto& desc = desc_list[file_name.string()];
			cereal::BinaryInputArchive i_archive(ifs);
			i_archive(KGL_NVP(file_name.string(), desc));
		}
		//std::cout << ss.str() << std::endl;
	}

	return S_OK;
}

HRESULT FCManager::Export(const std::filesystem::path& path, const Desc& desc) noexcept
{
	const auto& file_name = desc.first + ".bin";
	std::ofstream ofs(path.string() + file_name, std::ios::binary);
	if (ofs.is_open())
	{
		cereal::BinaryOutputArchive o_archive(ofs);
		o_archive(KGL_NVP(desc.first, desc.second));
	}
	return S_OK;
}

void FCManager::Create() noexcept
{
	const std::string title = "none";
	std::string name;
	UINT num = 0u;
	while (1)
	{
		num++;
		name = title + "_" + std::to_string(num);
		if (desc_list.count(name) == 0u)
			break;
	}
	select_desc = desc_list[name] = std::make_shared<FireworksDesc>();
	select_name = name;
}

float FCManager::GetMaxTime(const FireworksDesc& desc)
{
	float max_time_count = 0.f;
	for (const auto& effect : desc.effects)
	{
		max_time_count = std::max(max_time_count, effect.start_time + effect.time + effect.alive_time.y);
		if (effect.has_child)
		{
			max_time_count = std::max(max_time_count, GetMaxTime(effect.child));
		}
	}
	return max_time_count;
}

void FCManager::CreateDemo(const ParticleParent* p_parent) noexcept
{
	if (select_desc)
	{
		const float max_time_count = GetMaxTime(*select_desc);
		std::vector<Fireworks> fws;
		fws.reserve(1000u);
		fws.emplace_back(*select_desc);
		const float frame_time = max_time_count / 99u;
		auto& data = demo_data.emplace_back();
		data.fw_desc = select_desc;
		data.ptcs.resize(100u);
		for (UINT i = 0u; i < 100u; i++)
		{
			const float update_time = frame_time * i;
			if (i >= 1u)
			{
				// Particle Update
				data.ptcs[i] = data.ptcs[i - 1u];
				for (auto& ptc : data.ptcs[i])
				{
					if (ptc.Alive())
					{
						ptc.Update(update_time, p_parent);
					}
				}

				// Particle Sort
				const auto size = data.ptcs[i].size();
				constexpr Particle clear_ptc_value = {};
				UINT64 alive_count = 0;
				for (auto pdx = 0; pdx < size; pdx++)
				{
					if (data.ptcs[i][pdx].Alive())
					{
						if (pdx > alive_count)
						{
							data.ptcs[i][alive_count] = data.ptcs[i][pdx];
							data.ptcs[i][pdx] = clear_ptc_value;
						}
						alive_count++;
					}
				}
			}
			data.ptcs[i].reserve(10000u);

			for (UINT idx = 0; idx < fws.size(); idx++)
			{
				// Fireworks Sort
				if (!fws[idx].Update(update_time, &data.ptcs[i], p_parent, &fws))
				{
					fws[idx] = fws.back();
					fws.pop_back();
					idx--;
				}
			}
		}
	}
}

HRESULT FCManager::ImGuiUpdate() noexcept
{
	if (ImGui::Begin("Fireworks Editor", nullptr, ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::Button(u8"�쐬"))
				{
					Create();
				}
				if (ImGui::Button(u8"�ǂݍ���"))
				{
					ReloadDesc();
				}
				if (ImGui::Button(u8"�����o��"))
				{
					if (select_desc)
					{
						Export(directory->GetPath(), { select_name, select_desc });
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		for (auto& desc : desc_list)
		{
			DescImGuiUpdate(&desc);
		}
	}
	ImGui::End();
	return S_OK;
}

#define MINMAX_TEXT u8"�����_���ŕϓ�����(x = min, y = max)"

void FCManager::DescImGuiUpdate(Desc* desc)
{
	if (desc && desc->second)
	{
		if (ImGui::TreeNode(desc->first.c_str()))
		{
			if (desc->second == select_desc)
			{
				ImGui::Text(u8"�I��");
			}
			else if (ImGui::Button(u8"�I��"))
			{
				select_desc = desc->second;
				select_name = desc->first;
			}
			FWDescImGuiUpdate(desc->second.get());
			ImGui::TreePop();
		}
	}
}

void FCManager::FWDescImGuiUpdate(FireworksDesc* desc)
{
	if (desc)
	{
		ImGui::InputFloat(u8"����", &desc->mass);
		ImGui::InputFloat(u8"����", &desc->resistivity);
		ImGui::InputFloat(u8"�X�s�[�h", &desc->speed);
		ImGui::SameLine(); HelpMarker(u8"�v���C���[����ˏo�����ꍇ�̑��x(����)�B");

		if (ImGui::Button(u8"�G�t�F�N�g�ǉ�"))
		{
			desc->effects.emplace_back();
		}
		if (ImGui::TreeNode(u8"�G�t�F�N�g�폜"))
		{
			if (ImGui::Button(u8"�폜����"))
			{
				desc->effects.pop_back();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode(u8"�G�t�F�N�g"))
		{
			UINT idx = 0;
			for (auto& effect : desc->effects)
			{
				if (ImGui::TreeNode(std::string("[" + std::to_string(idx) + "]").c_str()))
				{
					if (ImGui::TreeNode(u8"�p�����[�^�["))
					{
						ImGui::InputFloat(u8"�J�n����(s)", &effect.start_time);
							ImGui::SameLine(); HelpMarker(u8"�J�n���Ԃ��o�ߌ�o�����܂��B");

							ImGui::InputFloat(u8"�\������(s)", &effect.time);
							ImGui::SameLine(); HelpMarker(u8"�o�����Ă�����ł���܂ł̎���");

							ImGui::InputFloat(u8"�J�n�������{��", &effect.start_accel);
							ImGui::SameLine(); HelpMarker(
								u8"�o�������Ƃ��ɐe�̑��x�ɂ��̒l��������B\n"
								u8"(���x��ς��Ȃ��ꍇ��[1.0f])"
							);
						ImGui::InputFloat(u8"���Ŏ������{��", &effect.end_accel);
						ImGui::SameLine(); HelpMarker(
							u8"���ł����Ƃ��ɐe�̑��x�ɂ��̒l��������B\n"
							u8"(���x��ς��Ȃ��ꍇ��[1.0f])"
						);

						ImGui::InputFloat2(u8"�p�[�e�B�N���̕\������", (float*)&effect.alive_time);
						ImGui::SameLine(); HelpMarker(MINMAX_TEXT);

						ImGui::InputFloat2(u8"�������[�g(s)", (float*)&effect.late);
						ImGui::SameLine(); HelpMarker(u8"��b�ԂɃ��[�g�̃p�[�e�B�N�����������܂�\n" MINMAX_TEXT);

						ImGui::InputFloat(u8"�������[�g�X�V�p�x(s)", (float*)&effect.late_update_time);

						ImGui::InputFloat2(u8"�p�[�e�B�N���ˏo���x(m/s)", (float*)&effect.speed);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���ˏo���̑��x\n" MINMAX_TEXT);
						ImGui::InputFloat2(u8"�ˏo�����x�̉e���x", (float*)&effect.base_speed);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���ˏo���̎ˏo�����x�̉e���x\n" MINMAX_TEXT);

						ImGui::InputFloat2(u8"�p�[�e�B�N���ˏo�T�C�Y(m)", (float*)&effect.scale);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���ˏo���̑傫��\n" MINMAX_TEXT);

						ImGui::InputFloat(u8"�ړ������ւ̃T�C�Y(�O)", &effect.scale_front);
						ImGui::SameLine(); HelpMarker(u8"�ړ������O���֑��x�ɉe�����󂯕ϓ�����T�C�Y\n" MINMAX_TEXT);
						ImGui::InputFloat(u8"�ړ������ւ̃T�C�Y(��)", &effect.scale_back);
						ImGui::SameLine(); HelpMarker(u8"�ړ���������֑��x�ɉe�����󂯕ϓ�����T�C�Y\n" MINMAX_TEXT);

						DirectX::XMFLOAT2 def_angle = { DirectX::XMConvertToDegrees(effect.angle.x), DirectX::XMConvertToDegrees(effect.angle.y) };
						if (ImGui::InputFloat2(u8"�ˏo�p�x(def)", (float*)&def_angle))
							effect.angle = { DirectX::XMConvertToRadians(def_angle.x), DirectX::XMConvertToRadians(def_angle.y) };
						ImGui::SameLine(); HelpMarker(
							u8"min(x)����max(y)�̊Ԃ̊p�x�Ń����_���Ɏˏo����\n"
							u8"�ˏo�����֔�΂��ꍇ�� 0\n"
							u8"�ˏo�������΂֔�΂��ꍇ�� deg(180)"
						);

						ImGui::InputFloat2(u8"�ˏo�����X�y�[�X", (float*)&effect.spawn_space);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���ˏo������spawn_space���ʒu�����炵�܂��B\n" MINMAX_TEXT);

						ImGui::ColorEdit4(u8"�G�t�F�N�g�������J���[", (float*)&effect.begin_color);
						ImGui::SameLine(); HelpMarker(
							u8"�G�t�F�N�g�������̃p�[�e�B�N���������̃J���[�ł��B\n"
							u8"���Ŏ��J���[�Ɍ������ĕω����Ă����܂�"
						);
						ImGui::ColorEdit4(u8"�G�t�F�N�g���Ŏ��J���[", (float*)&effect.end_color);
						ImGui::SameLine(); HelpMarker(u8"�G�t�F�N�g���Ŏ��̃p�[�e�B�N���������̃J���[�ł��B");

						ImGui::ColorEdit4(u8"�p�[�e�B�N�����Ŏ��J���[", (float*)&effect.erase_color);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N�����Ŏ��̃J���[�ł��B");

						ImGui::InputFloat(u8"����", &effect.resistivity);
						ImGui::SameLine(); HelpMarker(u8"�s�̉e���x");
						ImGui::InputFloat(u8"����S", &effect.resistivity);
						ImGui::SameLine(); HelpMarker(u8"�s�̉e���x�i�X�P�[������e�����󂯂�j");

						ImGui::Checkbox(u8"�u���[��", &effect.bloom);

						ImGui::TreePop();
					}

					ImGui::Checkbox(u8"�q��", &effect.has_child);
					ImGui::SameLine(); HelpMarker(
						u8"�q���̃G�t�F�N�g�������ǂ���\n"
						u8"(���̃t���O�����ꍇ�p�[�e�B�N���ł͂Ȃ��G�t�F�N�g���ˏo���܂�)"
					);
					if (effect.has_child)
					{
						FWDescImGuiUpdate(&effect.child);
					}

					ImGui::TreePop();
				}
				idx++;
			}

			ImGui::TreePop();
		}
	}
}