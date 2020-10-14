#include "../Hrd/FireworkCerialManager.hpp"
#include "../Hrd/Fireworks.hpp"
#include <algorithm>
#include <Cereal/archives/binary.hpp>
#include <Cereal/types/vector.hpp>
#include <Cereal/types/memory.hpp>
#include <imgui.h>
#include <fstream>
#include "../Hrd/Particle.hpp"
#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>

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
	demo_frame_number = 0;
	select_demo_number = 0;
	demo_play = false;
	demo_play_frame = 0.f;
	//demo_data.reserve(100u);
	demo_mg_stop = false;

	Load(directory);
}
FCManager::~FCManager()
{
	if (demo_mg_th)
	{
		{
			std::lock_guard<std::mutex> stop_check_lock(demo_mg_stop_mutex);
			demo_mg_stop = true;
		}
		demo_mg_th->join();
	}
}

HRESULT FCManager::Load(const std::filesystem::path& directory) noexcept
{
	this->directory = std::make_shared<KGL::Directory>(directory);
	HRESULT hr = ReloadDesc();
#if 0
	desc_list["Pop Muluti Fireworks"] = std::make_shared<FireworksDesc>(FIREWORK_EFFECTS::Get(0));
	desc_list["Shower Fireworks"] = std::make_shared<FireworksDesc>(FIREWORK_EFFECTS::Get(1));
	desc_list["Big Firework"] = std::make_shared<FireworksDesc>(FIREWORK_EFFECTS::Get(2));
	desc_list["Small Firework"] = std::make_shared<FireworksDesc>(FIREWORK_EFFECTS::Get(3));
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

bool FCManager::ChangeName(std::string before, std::string after) noexcept
{
	if (desc_list.count(before) == 0u || desc_list.count(after) == 1u)
		return false;
	desc_list[after] = desc_list[before];
	desc_list.erase(before);
	return true;
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

void FCManager::PlayDemo(UINT frame_num) noexcept
{
	demo_play = true;
	demo_play_frame = SCAST<float>(frame_num);
}

void FCManager::StopDemo() noexcept
{
	demo_play = false;
	demo_play_frame = 0;

	for (auto& data : demo_data)
	{
		data.SetResource(demo_frame_number);
	}
}

void FCManager::UpdateDemo(float update_time) noexcept
{
	if (demo_play)
	{
		demo_play_frame += (1.f / DemoData::FRAME_SECOND) * update_time;
		size_t max_frame_count = 0u;
		for (const auto& data : demo_data)
		{
			max_frame_count = std::max(max_frame_count, data.ptcs.size());
		}

		if (max_frame_count != 0)
		{
			if (demo_play_frame >= SCAST<float>(max_frame_count))
			{
				demo_play_frame -= SCAST<float>(SCAST<size_t>(demo_play_frame / max_frame_count) * max_frame_count);
			}

			for (auto& data : demo_data)
			{
				data.SetResource(SCAST<UINT>(demo_play_frame));
			}
		}
		else
		{
			demo_play_frame = 0.f;
		}
	}
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


FCManager::DemoData::DemoData(KGL::ComPtrC<ID3D12Device> device,
	std::shared_ptr<FireworksDesc> desc, UINT64 capacity)
{
	{
		std::lock_guard<std::mutex> lock(build_mutex);
		build_flg = true;
		exist = true;
		fw_desc = desc;
	}

	world_resource = std::make_shared<KGL::Resource<World>>(device, 1u);
	world_dm = std::make_shared<KGL::DescriptorManager>(device, 1u);
	world_handle = world_dm->Alloc();
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = world_resource->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = SCAST<UINT>(world_resource->SizeInBytes());
		device->CreateConstantBufferView(&cbv_desc, world_handle.Cpu());
	}

	resource = std::make_shared<KGL::Resource<Particle>>(device, capacity);

	vbv.BufferLocation = resource->Data()->GetGPUVirtualAddress();
	vbv.SizeInBytes = SCAST<UINT>(resource->SizeInBytes());
	vbv.StrideInBytes = sizeof(Particle);

	draw_flg = true;
}

FCManager::DemoData::DemoData(const DemoData& data)
{
	*this = data;
}
FCManager::DemoData& FCManager::DemoData::operator=(const DemoData& data)
{
	fw_desc = data.fw_desc;
	ptcs = data.ptcs;
	resource = data.resource;
	world_resource = data.world_resource;
	world_dm = data.world_dm;
	world_handle = data.world_handle;
	vbv = data.vbv;
	draw_flg = data.draw_flg;

	build_flg = data.build_flg;
	exist = data.exist;

	return *this;
}

void FCManager::DemoData::SetResource(UINT num)
{
	if (ptcs.size() > num)
	{
		auto* particles = resource->Map(0, &CD3DX12_RANGE(0, 0));
		ZeroMemory(particles, resource->SizeInBytes());
		std::copy(ptcs[num].begin(), ptcs[num].end(), particles);
		resource->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
}

void FCManager::DemoData::Build(const ParticleParent* p_parent, UINT set_frame_num)
{
	if (fw_desc)
	{
		ptcs.clear();
		const float max_time_count = GetMaxTime(*fw_desc);
		std::vector<Fireworks> fws;
		fws.reserve(1000u);
		auto desc = *fw_desc;
		desc.pos = { 0.f, 0.f, 0.f };
		desc.velocity = { 0.f, desc.speed, 0.f };
		fws.emplace_back(desc);
		const UINT FRAME_COUNT = SCAST<UINT>(max_time_count / FRAME_SECOND);
		ptcs.resize(FRAME_COUNT);
		auto parent = *p_parent;
		parent.elapsed_time = FRAME_SECOND;
		for (UINT i = 0u; i < FRAME_COUNT; i++)
		{
			if (i >= 1u)
			{
				// Particle Update
				ptcs[i] = ptcs[i - 1u];
				for (auto& ptc : ptcs[i])
				{
					if (ptc.Alive())
					{
						ptc.Update(parent.elapsed_time, &parent);
					}
				}

				// Particle Sort
				for (auto pdx = 0; pdx < ptcs[i].size(); pdx++)
				{
					if (!ptcs[i][pdx].Alive())
					{
						ptcs[i][pdx] = ptcs[i].back();
						ptcs[i].pop_back();
						pdx--;
					}
				}
			}
			ptcs[i].reserve(resource->Size());

			for (UINT idx = 0; idx < fws.size(); idx++)
			{
				// Fireworks Sort
				if (!fws[idx].Update(parent.elapsed_time, &ptcs[i], &parent, &fws))
				{
					fws[idx] = fws.back();
					fws.pop_back();
					idx--;
				}
			}
			ptcs[i].shrink_to_fit();
		}
		SetResource(set_frame_num);
	}
	std::lock_guard<std::mutex> lock(build_mutex);
	build_flg = false;
}

void FCManager::CreateDemo(KGL::ComPtrC<ID3D12Device> device, const ParticleParent* p_parent) noexcept
{
	std::lock_guard<std::mutex> lock(demo_mg_mutex);
	for (auto it = add_demo_data.begin(); it != add_demo_data.end();)
	{
		auto& data = demo_data.emplace_back(device, *it, 100000u);
		data.Build(p_parent, demo_frame_number);
		it = add_demo_data.erase(it);

		std::lock_guard<std::mutex> stop_check_lock(demo_mg_stop_mutex);
		if (demo_mg_stop) break;
	}
}

HRESULT FCManager::Update(float update_time) noexcept
{
	if (demo_play)
	{
		UpdateDemo(update_time);
	}

	return S_OK;
}

HRESULT FCManager::ImGuiUpdate(KGL::ComPtrC<ID3D12Device> device, const ParticleParent* p_parent) noexcept
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
				if (select_desc)
				{
					if (ImGui::Button(u8"書き出し"))
					{
						if (select_desc)
						{
							Export(directory->GetPath(), { select_name, select_desc });
						}
					}
					ImGui::SameLine();
					ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, (u8"[" + select_name + u8"]").c_str());
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if (ImGui::TreeNode(u8"一覧"))
		{
			bool loop = true;
			while (loop)
			{
				for (auto& desc : desc_list)
				{
					loop = DescImGuiUpdate(device, &desc, p_parent);
					if (loop) break;
				}
			}

			ImGui::TreePop();
		}
	}
	ImGui::End();

	if (!demo_data.empty())
	{
		if (ImGui::Begin("Fireworks Editor [demo]"))
		{
			{
				if (ImGui::Checkbox(u8"デモ再生", &demo_play))
				{
					if (demo_play) PlayDemo(demo_frame_number);
					else StopDemo();
				}
				ImGui::SameLine(); HelpMarker(u8"保存したデモデータを再生します。");

				size_t max_frame_count = 0u;
				for (auto& data : demo_data)
				{
					std::lock_guard<std::mutex> lock(data.build_mutex);
					if (!data.build_flg && data.exist)
					{
						max_frame_count = std::max(max_frame_count, data.ptcs.size());
					}
				}
				int gui_use_inum;
				if (demo_play)
					gui_use_inum = SCAST<int>(demo_play_frame);
				else
					gui_use_inum = SCAST<int>(demo_frame_number);
				gui_use_inum = std::min(gui_use_inum, SCAST<int>(max_frame_count));
				if (ImGui::SliderInt(u8"デモサンプル番号", &gui_use_inum, 0, SCAST<int>(max_frame_count)))
				{
					demo_frame_number = SCAST<UINT>(gui_use_inum);
					if (demo_play)
					{
						PlayDemo(demo_frame_number);
					}
					for (auto& data : demo_data)
					{
						std::lock_guard<std::mutex> lock(data.build_mutex);
						if (!data.build_flg && data.exist)
						{
							data.SetResource(demo_frame_number);
						}
					}
				}
				ImGui::SameLine(); HelpMarker(u8"保存したデモデータを確認できます。");
			}
			UINT idx = 0;
			for (auto it = demo_data.begin(); it != demo_data.end();)
			{
				{
					it->build_mutex.lock();
					if (it->build_flg || !it->exist)
					{
						if (!it->build_flg && !it->exist)
						{
							it->build_mutex.unlock();
							it = demo_data.erase(it);
							continue;
						}

						it->build_mutex.unlock();
						it++;
						continue;
					}
					it->build_mutex.unlock();
				}
				if (select_demo_number == idx)
				{
					ImGui::TextColored({ 0.f, 1.f, 0.5f, 1.f }, ("[No." + std::to_string(idx + 1) + "]").c_str());
				}
				else if (ImGui::Button(("[No." + std::to_string(idx + 1) + "]").c_str()))
				{
					select_demo_number = idx;
				}
				ImGui::SameLine();
				if (ImGui::Button(((it->draw_flg ? u8"非表示##" : u8"表示##") + std::to_string(idx)).c_str()))
				{
					it->draw_flg = !it->draw_flg;
				}
				ImGui::SameLine();
				if (ImGui::Button((u8"削除##" + std::to_string(idx)).c_str()))
				{
					it->exist = false;

					// 前回描画に使用された可能性があるのでこのフレームでは消さない
					it++;
					idx++;
					continue;
				}

				if (select_demo_number == idx)
				{
					auto* world = it->world_resource->Map(0, &CD3DX12_RANGE(0, 0));
					ImGui::InputFloat3((u8"座標##" + std::to_string(idx)).c_str(), (float*)&world->position);
					it->world_resource->Unmap(0, &CD3DX12_RANGE(0, 0));
				}
				it++;
				idx++;
			}
		}
		ImGui::End();
	}
	return S_OK;
}

#define MINMAX_TEXT u8"ランダムで変動する(x = min, y = max)"

bool FCManager::DescImGuiUpdate(KGL::ComPtrC<ID3D12Device> device, Desc* desc, const ParticleParent* p_parent)
{
	bool result = false;
	if (desc && desc->second)
	{
		if (ImGui::TreeNode(desc->first.c_str()))
		{
			static std::string s_after_name("", 64u);
			ImGui::InputText(("##" + std::to_string(RCAST<intptr_t>(desc))).c_str(), s_after_name.data(), s_after_name.size());
			ImGui::SameLine();
			if (ImGui::Button(u8"名前変更"))
			{
				ChangeName(desc->first, s_after_name);
				ImGui::TreePop();
				return true;
			}

			if (desc->second == select_desc)
			{
				ImGui::Text(u8"選択中");
			}
			else if (ImGui::Button(u8"選択"))
			{
				select_desc = desc->second;
				select_name = desc->first;
			}

			UINT build_count = 0u;
			for (const auto& data : add_demo_data)
			{
				if (data == desc->second)
				{
					build_count++;
				}
			}
			if (ImGui::Button(u8"デモ作成"))
			{
				add_demo_data.push_back(desc->second);
				const bool try_lock = demo_mg_mutex.try_lock();
				if (try_lock) demo_mg_mutex.unlock();

				if (!demo_mg_th || try_lock)
				{
					if (demo_mg_th && try_lock)
						demo_mg_th->join();
					demo_mg_th = std::make_unique<std::thread>(&FCManager::CreateDemo, this, device, p_parent);
				}
			}
			if (build_count > 0u)
			{
				ImGui::SameLine();
				ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, (u8"[ " + std::to_string(build_count) + u8" 個作成中 ]").c_str());
			}

			FWDescImGuiUpdate(desc->second.get());
			ImGui::TreePop();
		}
	}
	return result;
}

void FCManager::FWDescImGuiUpdate(FireworksDesc* desc)
{
	if (desc)
	{
		if (ImGui::TreeNode(u8"基本パラメーター"))
		{
			ImGui::InputFloat(u8"質量", &desc->mass);
			ImGui::InputFloat(u8"効力", &desc->resistivity);
			ImGui::InputFloat(u8"スピード", &desc->speed);
			ImGui::SameLine(); HelpMarker(u8"プレイヤーから射出される場合の速度(初速)。");

			ImGui::TreePop();
		}
		if (ImGui::TreeNode(u8"エフェクト"))
		{
			if (ImGui::TreeNode(u8"エフェクト作成/破棄"))
			{
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
				ImGui::TreePop();
			}

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
						ImGui::InputFloat(u8"効力S", &effect.scale_resistivity);
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

void FCManager::DemoData::Render(KGL::ComPtr<ID3D12GraphicsCommandList> cmd_list, UINT num) const noexcept
{
	if (ptcs.size() > num && draw_flg)
	{
		const auto ptcs_size = ptcs[num].size();
		if (ptcs_size > 0)
		{
			cmd_list->SetDescriptorHeaps(1, world_handle.Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(1, world_handle.Gpu());

			cmd_list->IASetVertexBuffers(0, 1, &vbv);
			cmd_list->DrawInstanced(SCAST<UINT>(ptcs_size), 1, 0, 0);
		}
	}
}

size_t FCManager::DemoData::Size(UINT num) const
{
	if (ptcs.size() > num && draw_flg)
	{
		return ptcs[num].size();
	}
	return 0u;
}

void FCManager::Render(KGL::ComPtr<ID3D12GraphicsCommandList> cmd_list) noexcept
{
	const UINT frame_num = demo_play ? SCAST<UINT>(demo_play_frame) : demo_frame_number;
	for (auto& data : demo_data)
	{
		std::lock_guard<std::mutex> lock(data.build_mutex);
		if (!data.build_flg && data.exist)
		{
			data.Render(cmd_list, frame_num);
		}
	}
}

size_t FCManager::Size() const
{
	const UINT frame_num = demo_play ? SCAST<UINT>(demo_play_frame) : demo_frame_number;
	size_t total_size = 0u;

	for (const auto& data : demo_data)
	{
		if (!data.build_flg && data.exist)
		{
			total_size += data.Size(frame_num);
		}
	}
	return total_size;
}