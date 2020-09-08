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
				if (ImGui::Button(u8"作成"))
				{
					Create();
				}
				if (ImGui::Button(u8"読み込み"))
				{
					ReloadDesc();
				}
				if (ImGui::Button(u8"書き出し"))
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

#define MINMAX_TEXT u8"ランダムで変動する(x = min, y = max)"

void FCManager::DescImGuiUpdate(Desc* desc)
{
	if (desc && desc->second)
	{
		if (ImGui::TreeNode(desc->first.c_str()))
		{
			if (desc->second == select_desc)
			{
				ImGui::Text(u8"選択中");
			}
			else if (ImGui::Button(u8"選択"))
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
		ImGui::InputFloat(u8"質量", &desc->mass);
		ImGui::InputFloat(u8"効力", &desc->resistivity);
		ImGui::InputFloat(u8"スピード", &desc->speed);
		ImGui::SameLine(); HelpMarker(u8"プレイヤーから射出される場合の速度(初速)。");

		if (ImGui::Button(u8"エフェクト追加"))
		{
			desc->effects.emplace_back();
		}
		if (ImGui::TreeNode(u8"エフェクト削除"))
		{
			if (ImGui::Button(u8"削除する"))
			{
				desc->effects.pop_back();
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNode(u8"エフェクト"))
		{
			UINT idx = 0;
			for (auto& effect : desc->effects)
			{
				if (ImGui::TreeNode(std::string("[" + std::to_string(idx) + "]").c_str()))
				{
					if (ImGui::TreeNode(u8"パラメーター"))
					{
						ImGui::InputFloat(u8"開始時間(s)", &effect.start_time);
							ImGui::SameLine(); HelpMarker(u8"開始時間が経過後出現します。");

							ImGui::InputFloat(u8"表示時間(s)", &effect.time);
							ImGui::SameLine(); HelpMarker(u8"出現してから消滅するまでの時間");

							ImGui::InputFloat(u8"開始時加速倍数", &effect.start_accel);
							ImGui::SameLine(); HelpMarker(
								u8"出現したときに親の速度にこの値をかける。\n"
								u8"(速度を変えない場合は[1.0f])"
							);
						ImGui::InputFloat(u8"消滅時加速倍数", &effect.end_accel);
						ImGui::SameLine(); HelpMarker(
							u8"消滅したときに親の速度にこの値をかける。\n"
							u8"(速度を変えない場合は[1.0f])"
						);

						ImGui::InputFloat2(u8"パーティクルの表示時間", (float*)&effect.alive_time);
						ImGui::SameLine(); HelpMarker(MINMAX_TEXT);

						ImGui::InputFloat2(u8"生成レート(s)", (float*)&effect.late);
						ImGui::SameLine(); HelpMarker(u8"一秒間にレート個のパーティクルが発生します\n" MINMAX_TEXT);

						ImGui::InputFloat(u8"生成レート更新頻度(s)", (float*)&effect.late_update_time);

						ImGui::InputFloat2(u8"パーティクル射出速度(m/s)", (float*)&effect.speed);
						ImGui::SameLine(); HelpMarker(u8"パーティクル射出時の速度\n" MINMAX_TEXT);
						ImGui::InputFloat2(u8"射出元速度の影響度", (float*)&effect.base_speed);
						ImGui::SameLine(); HelpMarker(u8"パーティクル射出時の射出元速度の影響度\n" MINMAX_TEXT);

						ImGui::InputFloat2(u8"パーティクル射出サイズ(m)", (float*)&effect.scale);
						ImGui::SameLine(); HelpMarker(u8"パーティクル射出時の大きさ\n" MINMAX_TEXT);

						ImGui::InputFloat(u8"移動方向へのサイズ(前)", &effect.scale_front);
						ImGui::SameLine(); HelpMarker(u8"移動方向前方へ速度に影響を受け変動するサイズ\n" MINMAX_TEXT);
						ImGui::InputFloat(u8"移動方向へのサイズ(後)", &effect.scale_back);
						ImGui::SameLine(); HelpMarker(u8"移動方向後方へ速度に影響を受け変動するサイズ\n" MINMAX_TEXT);

						DirectX::XMFLOAT2 def_angle = { DirectX::XMConvertToDegrees(effect.angle.x), DirectX::XMConvertToDegrees(effect.angle.y) };
						if (ImGui::InputFloat2(u8"射出角度(def)", (float*)&def_angle))
							effect.angle = { DirectX::XMConvertToRadians(def_angle.x), DirectX::XMConvertToRadians(def_angle.y) };
						ImGui::SameLine(); HelpMarker(
							u8"min(x)からmax(y)の間の角度でランダムに射出する\n"
							u8"射出方向へ飛ばす場合は 0\n"
							u8"射出方向反対へ飛ばす場合は deg(180)"
						);

						ImGui::InputFloat2(u8"射出方向スペース", (float*)&effect.spawn_space);
						ImGui::SameLine(); HelpMarker(u8"パーティクル射出方向へspawn_space分位置をずらします。\n" MINMAX_TEXT);

						ImGui::ColorEdit4(u8"エフェクト生成時カラー", (float*)&effect.begin_color);
						ImGui::SameLine(); HelpMarker(
							u8"エフェクト生成時のパーティクル生成時のカラーです。\n"
							u8"消滅時カラーに向かって変化していきます"
						);
						ImGui::ColorEdit4(u8"エフェクト消滅時カラー", (float*)&effect.end_color);
						ImGui::SameLine(); HelpMarker(u8"エフェクト消滅時のパーティクル生成時のカラーです。");

						ImGui::ColorEdit4(u8"パーティクル消滅時カラー", (float*)&effect.erase_color);
						ImGui::SameLine(); HelpMarker(u8"パーティクル消滅時のカラーです。");

						ImGui::InputFloat(u8"効力", &effect.resistivity);
						ImGui::SameLine(); HelpMarker(u8"不可の影響度");
						ImGui::InputFloat(u8"効力S", &effect.resistivity);
						ImGui::SameLine(); HelpMarker(u8"不可の影響度（スケールから影響を受ける）");

						ImGui::Checkbox(u8"ブルーム", &effect.bloom);

						ImGui::TreePop();
					}

					ImGui::Checkbox(u8"子供", &effect.has_child);
					ImGui::SameLine(); HelpMarker(
						u8"子供のエフェクトを持つかどうか\n"
						u8"(このフラグを持つ場合パーティクルではなくエフェクトを射出します)"
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