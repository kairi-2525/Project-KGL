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

#define MINMAX_TEXT u8"ランダムで変動する(x = min, y = max)"

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
	RCHECK(!desc, "FS_Obj::SetRandomColor で desc が nullptr");
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

// ここでFireworksを生成する
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
		// この関数内で使用する更新時間(余剰分は再帰処理で処理する)
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
			// 一回分の生成時間を減らす
			counter -= spawn_late;
			
			desc.pos = { 0.f, 0.f, 0.f };

			rmd_float rmd_spawn_power(obj_desc.spawn_power.x, obj_desc.spawn_power.y);
			desc.velocity = { 0.f, desc.speed * rmd_spawn_power(mt), 0.f };

			rmd_float rmdpos(-obj_desc.spawn_radius, +obj_desc.spawn_radius);
			desc.pos.x += rmdpos(mt);
			desc.pos.z += rmdpos(mt);

			// 射出角度のランダム値を決定
			XMFLOAT2 nm_angle;
			nm_angle.x = XMConvertToRadians(obj_desc.spawn_angle.x) / XM_PI;
			nm_angle.y = XMConvertToRadians(obj_desc.spawn_angle.y) / XM_PI;
			rmd_float rmdangle(nm_angle.x, nm_angle.y);
			static rmd_float rmdangle360(0.f, XM_2PI);
			constexpr float radian90f = XMConvertToRadians(90.f);

			// ランダムな射出ベクトルを計算
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

			// ランダムな色をセット(HSV空間でHのみ変更)
			SetRandomColor(&desc);

			pout_fireworks->emplace_back(desc);
		}

		// 再帰処理
		return Update(update_time, pout_fireworks);
	}
}

bool FS_Obj::GUIUpdate(const std::map<const std::string, std::shared_ptr<FireworksDesc>>& desc_list)
{
	bool result = true;
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
		if (ImGui::Button(u8"削除"))
		{
			result = false;
		}

		// エフェクト変更
		{
			// 2番目のパラメーターは、コンボを開く前にプレビューされるラベルです。
			if (ImGui::BeginCombo((u8"エフェクト変更##" + std::to_string(RCAST<intptr_t>(this))).c_str(), obj_desc.fw_name.c_str()))
			{
				for (const auto& desc : desc_list)
				{
					// オブジェクトの外側または内側に、選択内容を好きなように保存できます
					bool is_selected = (obj_desc.fw_name == desc.first);
					if (ImGui::Selectable(desc.first.c_str(), is_selected))
					{
						obj_desc.fw_name = desc.first;
						fw_desc = desc.second;
					}
					if (is_selected)
						// コンボを開くときに初期フォーカスを設定できます（キーボードナビゲーションサポートの場合は+をスクロールします）
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}

		// 各種パラメーター変更
		ImGui::InputFloat2(u8"生成開始時間", (float*)&obj_desc.start_time);
		obj_desc.start_time.y = (std::max)(obj_desc.start_time.x, obj_desc.start_time.y);
		ImGui::SameLine(); HelpMarker(u8"この時間が経過後パーティクルの生成を開始します。\n" MINMAX_TEXT);

		if (obj_desc.infinity)
		{
			ImGui::Checkbox(u8"無限", &obj_desc.infinity);
			ImGui::SameLine(); HelpMarker(u8"永遠にパーティクルを生成し続けます。");
		}
		else
		{
			ImGui::InputFloat2(u8"生成時間", (float*)&obj_desc.time);
			obj_desc.time.y = (std::max)(obj_desc.time.x, obj_desc.time.y);
			if (obj_desc.time.y < 1.f / obj_desc.spawn_late.y)
			{
				obj_desc.spawn_late.y = 1.f / obj_desc.time.y;
			}
			ImGui::SameLine(); ImGui::Checkbox(u8"無限", &obj_desc.infinity);
			ImGui::SameLine(); HelpMarker(u8"この時間だけパーティクルを生成します。\n" MINMAX_TEXT);
		}

		ImGui::InputFloat2(u8"待機時間", (float*)&obj_desc.wait_time);
		obj_desc.wait_time.y = (std::max)(obj_desc.wait_time.x, obj_desc.wait_time.y);
		ImGui::SameLine(); HelpMarker(u8"パーティクルの生成を再開するまでの時間。\n" MINMAX_TEXT);

		ImGui::InputFloat(u8"生成範囲", (float*)&obj_desc.spawn_radius);
		obj_desc.spawn_radius = (std::max)(0.f, obj_desc.spawn_radius);
		ImGui::SameLine(); HelpMarker(u8"パーティクルを生成する半径");

		ImGui::InputFloat2(u8"射出角度", (float*)&obj_desc.spawn_angle);
		obj_desc.spawn_angle.y = (std::max)(obj_desc.spawn_angle.x, obj_desc.spawn_angle.y);
		ImGui::SameLine(); HelpMarker(u8"真上からずらす角度(deg)\n" MINMAX_TEXT);

		ImGui::InputFloat2(u8"生成レート", (float*)&obj_desc.spawn_late);
		obj_desc.spawn_late.y = (std::max)(obj_desc.spawn_late.x, obj_desc.spawn_late.y);
		obj_desc.time.y = (std::max)(obj_desc.time.y, 1.f / obj_desc.spawn_late.y);
		obj_desc.time.x = (std::min)(obj_desc.time.x, obj_desc.time.y);
		ImGui::SameLine(); HelpMarker(u8"1秒間に何個生成するか。\n(待機時間を挟んで更新)\n" MINMAX_TEXT);

		ImGui::InputFloat2(u8"射出速度スケール", (float*)&obj_desc.spawn_power);
		obj_desc.spawn_power.y = (std::max)(obj_desc.spawn_power.x, obj_desc.spawn_power.y);
		ImGui::SameLine(); HelpMarker(u8"元の射出速度をスケール倍します\n(元の速度はFireworksが持っている)\n" MINMAX_TEXT);

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
		// 名前変更
		if (set_name.size() < 64u)
			set_name.resize(64u);
		ImGui::InputText(("##" + ptr_str0).c_str(), set_name.data(), set_name.size());
		ImGui::SameLine();
		if (ImGui::Button(u8"名前変更") && !set_name.empty())
		{
			name = set_name;
		}

		if (ImGui::Button(u8"削除"))
		{
			result = false;
		}

		// 新規作成
		{
			// 2番目のパラメーターは、コンボを開く前にプレビューされるラベルです。
			if (ImGui::BeginCombo(("##" + ptr_str1).c_str(), create_name.c_str()))
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

	// spawnerを読み込む
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

//	選択中のFSを書き出す
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
				if (select_fs)
				{
					if (ImGui::BeginMenu(u8"書き出し"))
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
			if (ImGui::TreeNode(u8"一覧"))
			{
				UINT i = 0u;
				for (auto itr = fs_list.begin(); itr != fs_list.end();)
				{
					if ((*itr) == select_fs)
					{
						if (ImGui::Button((u8"解除##" + std::to_string(i)).c_str()))
						{
							select_fs = nullptr;
						}
					}
					else
					{
						if (ImGui::Button((u8"選択##" + std::to_string(i)).c_str()))
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