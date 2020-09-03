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

#define MINMAX_TEXT "ランダムで変動する(x = min, y = max)"

void FCManager::DescImGuiUpdate(std::shared_ptr<Desc> desc)
{
	if (desc)
	{
		if (ImGui::TreeNode(desc->first.c_str()))
		{
			auto& fdesc = desc->second;
			ImGui::InputFloat("質量", &fdesc->mass);
			ImGui::InputFloat("効力", &fdesc->resistivity);
			if (ImGui::TreeNode("エフェクト"))
			{
				UINT idx = 0;
				for (auto& effect : fdesc->effects)
				{
					if (ImGui::TreeNode(std::string("[" + std::to_string(idx) + "]").c_str()))
					{
						ImGui::InputFloat("開始時間(s)", &effect.start_time);
						ImGui::SameLine(); HelpMarker("開始時間が経過後出現します。");

						ImGui::InputFloat("表示時間(s)", &effect.time);
						ImGui::SameLine(); HelpMarker("出現してから消滅するまでの時間");

						ImGui::InputFloat("開始時加速倍数", &effect.start_accel);
						ImGui::SameLine(); HelpMarker(
							"出現したときに親の速度にこの値をかける。\n"
							"(速度を変えない場合は[1.0f])"
						);
						ImGui::InputFloat("消滅時加速倍数", &effect.end_accel);
						ImGui::SameLine(); HelpMarker(
							"消滅したときに親の速度にこの値をかける。\n"
							"(速度を変えない場合は[1.0f])"
						);

						ImGui::InputFloat2("パーティクルの表示時間", (float*)&effect.alive_time);
						ImGui::SameLine(); HelpMarker(MINMAX_TEXT);

						ImGui::InputFloat2("生成レート(s)", (float*)&effect.late);
						ImGui::SameLine(); HelpMarker("一秒間にレート個のパーティクルが発生します\n" MINMAX_TEXT);

						ImGui::InputFloat("生成レート更新頻度(s)", (float*)&effect.late_update_time);

						ImGui::InputFloat2("パーティクル射出速度(m/s)", (float*)&effect.speed);
						ImGui::SameLine(); HelpMarker("パーティクル射出時の速度\n" MINMAX_TEXT);
						ImGui::InputFloat2("射出元速度の影響度", (float*)&effect.base_speed);
						ImGui::SameLine(); HelpMarker("パーティクル射出時の射出元速度の影響度\n" MINMAX_TEXT);

						ImGui::InputFloat2("パーティクル射出サイズ(m)", (float*)&effect.scale);
						ImGui::SameLine(); HelpMarker("パーティクル射出時の大きさ\n" MINMAX_TEXT);

						ImGui::InputFloat("移動方向へのサイズ(前)", &effect.scale_front);
						ImGui::SameLine(); HelpMarker("移動方向前方へ速度に影響を受け変動するサイズ\n" MINMAX_TEXT);
						ImGui::InputFloat("移動方向へのサイズ(後)", &effect.scale_back);
						ImGui::SameLine(); HelpMarker("移動方向後方へ速度に影響を受け変動するサイズ\n" MINMAX_TEXT);

						ImGui::InputFloat2("射出角度(rad)", (float*)&effect.angle);
						ImGui::SameLine(); HelpMarker(
							"min(x)からmax(y)の間の角度でランダムに射出する\n"
							"射出方向へ飛ばす場合は 0\n"
							"射出方向反対へ飛ばす場合は deg(180)"
						);

						ImGui::InputFloat2("射出方向スペース", (float*)&effect.spawn_space);
						ImGui::SameLine(); HelpMarker("パーティクル射出方向へspawn_space分位置をずらします。\n" MINMAX_TEXT);

						ImGui::ColorEdit4("生成時カラー", (float*)&effect.begin_color);
						ImGui::SameLine(); HelpMarker(
							"パーティクル生成時のカラーです\n"
							"消滅時カラーに向かって変化していきます"
						);
						ImGui::ColorEdit4("消滅時カラー", (float*)&effect.end_color);
						ImGui::SameLine(); HelpMarker("パーティクル消滅時のカラーです");

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