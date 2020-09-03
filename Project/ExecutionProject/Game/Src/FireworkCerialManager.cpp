#include "../Hrd/FireworkCerialManager.hpp"
#include <algorithm>
#include <Cereal/archives/binary.hpp>
#include <Cereal/types/vector.hpp>
#include <imgui.h>
#include <fstream>

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
	return ReloadDesc();
}

HRESULT FCManager::ReloadDesc() noexcept
{
	desc_list.clear();
	select_desc = nullptr;

	const auto& files = directory->GetFiles(".bin");
	const size_t files_size = files.size();
	for (const auto& file : files)
	{
		std::stringstream ss;
		{
			const auto& file_name = file.filename();
			auto& desc = desc_list[file_name.string()];
			cereal::BinaryInputArchive i_archive(ss);
			i_archive(cereal::make_nvp(file_name.string(), desc.get()));
		}
		std::cout << ss.str() << std::endl;
	}

	return S_OK;
}

HRESULT FCManager::Export(const Desc& desc) noexcept
{
	const auto& file_name = desc.first + ".bin";
	std::ofstream ofs(file_name, std::ios::binary);
	if (ofs.is_open())
	{
		cereal::BinaryOutputArchive o_archive(ofs);
		o_archive(cereal::make_nvp(desc.first, desc.second.get()));
	}
	return S_OK;
}

HRESULT FCManager::ImGuiUpdate()
{
	if (ImGui::Begin("Fireworks Editor"))
	{

	}
	return S_OK;
}

#define MINMAX_TEXT "�����_���ŕϓ�����(x = min, y = max)"

void FCManager::DescImGuiUpdate(std::shared_ptr<Desc> desc)
{
	if (desc)
	{
		if (ImGui::TreeNode(desc->first.c_str()))
		{
			auto& fdesc = desc->second;
			ImGui::InputFloat("����", &fdesc->mass);
			ImGui::InputFloat("����", &fdesc->resistivity);
			if (ImGui::TreeNode("�G�t�F�N�g"))
			{
				UINT idx = 0;
				for (auto& effect : fdesc->effects)
				{
					if (ImGui::TreeNode(std::string("[" + std::to_string(idx) + "]").c_str()))
					{
						ImGui::InputFloat("�J�n����(s)", &effect.start_time);
						ImGui::SameLine(); HelpMarker("�J�n���Ԃ��o�ߌ�o�����܂��B");

						ImGui::InputFloat("�\������(s)", &effect.time);
						ImGui::SameLine(); HelpMarker("�o�����Ă�����ł���܂ł̎���");

						ImGui::InputFloat("�J�n�������{��", &effect.start_accel);
						ImGui::SameLine(); HelpMarker(
							"�o�������Ƃ��ɐe�̑��x�ɂ��̒l��������B\n"
							"(���x��ς��Ȃ��ꍇ��[1.0f])"
						);
						ImGui::InputFloat("���Ŏ������{��", &effect.end_accel);
						ImGui::SameLine(); HelpMarker(
							"���ł����Ƃ��ɐe�̑��x�ɂ��̒l��������B\n"
							"(���x��ς��Ȃ��ꍇ��[1.0f])"
						);

						ImGui::InputFloat2("�p�[�e�B�N���̕\������", (float*)&effect.alive_time);
						ImGui::SameLine(); HelpMarker(MINMAX_TEXT);

						ImGui::InputFloat2("�������[�g(s)", (float*)&effect.late);
						ImGui::SameLine(); HelpMarker("��b�ԂɃ��[�g�̃p�[�e�B�N�����������܂�\n" MINMAX_TEXT);

						ImGui::InputFloat("�������[�g�X�V�p�x(s)", (float*)&effect.late_update_time);

						ImGui::InputFloat2("�p�[�e�B�N���ˏo���x(m/s)", (float*)&effect.speed);
						ImGui::SameLine(); HelpMarker("�p�[�e�B�N���ˏo���̑��x\n" MINMAX_TEXT);
						ImGui::InputFloat2("�ˏo�����x�̉e���x", (float*)&effect.base_speed);
						ImGui::SameLine(); HelpMarker("�p�[�e�B�N���ˏo���̎ˏo�����x�̉e���x\n" MINMAX_TEXT);

						ImGui::InputFloat2("�p�[�e�B�N���ˏo�T�C�Y(m)", (float*)&effect.scale);
						ImGui::SameLine(); HelpMarker("�p�[�e�B�N���ˏo���̑傫��\n" MINMAX_TEXT);

						ImGui::InputFloat("�ړ������ւ̃T�C�Y(�O)", &effect.scale_front);
						ImGui::SameLine(); HelpMarker("�ړ������O���֑��x�ɉe�����󂯕ϓ�����T�C�Y\n" MINMAX_TEXT);
						ImGui::InputFloat("�ړ������ւ̃T�C�Y(��)", &effect.scale_back);
						ImGui::SameLine(); HelpMarker("�ړ���������֑��x�ɉe�����󂯕ϓ�����T�C�Y\n" MINMAX_TEXT);

						ImGui::InputFloat2("�ˏo�p�x(rad)", (float*)&effect.angle);
						ImGui::SameLine(); HelpMarker(
							"min(x)����max(y)�̊Ԃ̊p�x�Ń����_���Ɏˏo����\n"
							"�ˏo�����֔�΂��ꍇ�� 0\n"
							"�ˏo�������΂֔�΂��ꍇ�� deg(180)"
						);

						ImGui::InputFloat2("�ˏo�����X�y�[�X", (float*)&effect.spawn_space);
						ImGui::SameLine(); HelpMarker("�p�[�e�B�N���ˏo������spawn_space���ʒu�����炵�܂��B\n" MINMAX_TEXT);

						ImGui::ColorEdit4("�������J���[", (float*)&effect.begin_color);
						ImGui::SameLine(); HelpMarker(
							"�p�[�e�B�N���������̃J���[�ł�\n"
							"���Ŏ��J���[�Ɍ������ĕω����Ă����܂�"
						);
						ImGui::ColorEdit4("���Ŏ��J���[", (float*)&effect.end_color);
						ImGui::SameLine(); HelpMarker("�p�[�e�B�N�����Ŏ��̃J���[�ł�");

						ImGui::TreePop();
					}
					idx++;
				}

				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
	}
}