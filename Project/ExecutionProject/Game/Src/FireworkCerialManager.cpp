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

#define EditCheck(func, result, func_result) func_result = func; result = result || func_result;

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
	if (demo_select_th)
	{
		{
			std::lock_guard<std::mutex> stop_check_lock(demo_select_stop_mutex);
			demo_select_stop = true;
		}
		demo_select_th->join();
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
			//auto& desc = desc_list[file_name.string()];
			std::shared_ptr<FireworksDesc> desc;
			cereal::BinaryInputArchive i_archive(ifs);
			i_archive(KGL_NVP(file_name.string(), desc));
			Create(file_name.string(), desc.get());
		}
		//std::cout << ss.str() << std::endl;
	}

	return S_OK;
}

HRESULT FCManager::Export(
	const std::filesystem::path& path,
	std::string file_name,
	std::shared_ptr<FireworksDesc> desc
) noexcept
{
	std::string bin = ".bin";
	auto pos = file_name.find('\0');
	if (std::string::npos != pos)
		file_name.erase(pos, file_name.size());
	
	std::string file_name_e = file_name + bin;

	std::ofstream ofs(path.string() + file_name_e, std::ios::binary);
	if (ofs.is_open())
	{
		cereal::BinaryOutputArchive o_archive(ofs);
		o_archive(KGL_NVP(file_name, desc));
	}
	return S_OK;
}

bool FCManager::ChangeName(std::string before, std::string after) noexcept
{
	if (desc_list.count(before) == 0u || desc_list.count(after) == 1u)
		return false;
	desc_list[after] = desc_list[before];
	if (desc_list[after] == select_desc)
	{
		select_name = after;
	}
	desc_list.erase(before);
	return true;
}

void FCManager::Create(const std::string& name, const FireworksDesc* desc) noexcept
{
	if (desc)
	{
		if (desc_list.count(name) == 0u)
		{
			select_desc = desc_list[name] = std::make_shared<FireworksDesc>(*desc);
			select_name = name;
			select_desc->original_name = name;
			return;
		}
		std::string set_name;
		UINT num = 0u;
		while (1)
		{
			num++;
			set_name = name + "_" + std::to_string(num);
			if (desc_list.count(set_name) == 0u)
				break;
		}
		select_desc = desc_list[set_name] = std::make_shared<FireworksDesc>(*desc);
		select_name = set_name;
	}
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
		for (auto& data : demo_data)
		{
			std::lock_guard<std::mutex> lock(data.build_mutex);
			if (!data.build_flg && data.exist)
				max_frame_count = std::max(max_frame_count, data.ptcs.size());
		}
		for (auto& data : demo_select_data)
		{
			std::lock_guard<std::mutex> lock(data.build_mutex);
			if (!data.build_flg && data.exist)
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
				std::lock_guard<std::mutex> lock(data.build_mutex);
				if (!data.build_flg && data.exist)
					data.SetResource(SCAST<UINT>(demo_play_frame));
			}
			for (auto& data : demo_select_data)
			{
				std::lock_guard<std::mutex> lock(data.build_mutex);
				if (!data.build_flg && data.exist)
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
		const UINT FRAME_COUNT = SCAST<UINT>(max_time_count / FRAME_SECOND) + 1u;
		ptcs.resize(FRAME_COUNT);
		auto parent = *p_parent;
		parent.elapsed_time = FRAME_SECOND;
		for (UINT i = 1u; i < FRAME_COUNT; i++)
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

void FCManager::CreateDemo(
	KGL::ComPtrC<ID3D12Device> device, const ParticleParent* p_parent,
	std::mutex* mt_lock, std::mutex* mt_stop_lock,
	std::list<std::shared_ptr<FireworksDesc>>* p_add_demo_data,
	std::list<DemoData>* p_demo_data,
	const UINT* frame_number,
	const bool* stop_flg,
	std::mutex* mt_clear
) noexcept
{
	std::lock_guard<std::mutex> lock(*mt_lock);
	for (auto it = p_add_demo_data->begin(); it != p_add_demo_data->end();)
	{
		auto& data = p_demo_data->emplace_back(device, *it, 100000u);
		data.Build(p_parent, *frame_number);
		it = p_add_demo_data->erase(it);

		if (mt_clear)
		{
			std::lock_guard<std::mutex> lock(*mt_clear);
			if (!p_add_demo_data->empty())
			{
				auto add_data = *p_add_demo_data->rbegin();
				p_add_demo_data->clear();
				p_add_demo_data->push_back(add_data);
			}
		}

		std::lock_guard<std::mutex> stop_check_lock(*mt_stop_lock);
		if (*stop_flg) break;
	}
}

HRESULT FCManager::Update(float update_time) noexcept
{
	if (demo_play)
	{
		UpdateDemo(update_time);
	}

	demo_select_itr = demo_select_data.end();
	for (auto it = demo_select_data.begin(); it != demo_select_data.end();)
	{
		it->build_mutex.lock();
		bool build_flg = it->build_flg;
		if (build_flg || !it->exist)
		{
			if (!it->build_flg && !it->exist)
			{
				it->build_mutex.unlock();
				it = demo_select_data.erase(it);
				continue;
			}

			it->build_mutex.unlock();
			it++;
			continue;
		}
		it->build_mutex.unlock();

		if (!build_flg) demo_select_itr = it;
		it++;
	}
	for (auto it = demo_select_data.begin(); it != demo_select_data.end();)
	{
		it->build_mutex.lock();
		bool build_flg = it->build_flg;
		if (build_flg || !it->exist)
		{
			it->build_mutex.unlock();
			it++;
			continue;
		}
		it->build_mutex.unlock();

		if (!build_flg && it != demo_select_itr)
		{
			// �O��`��Ɏg�p���ꂽ�\��������̂ł��̃t���[���ł͏����Ȃ�
			it->exist = false;
		}
		it++;
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
				if (ImGui::Button(u8"�쐬"))
				{
					Create();
				}
				if (ImGui::Button(u8"�ǂݍ���"))
				{
					ReloadDesc();
				}
				if (select_desc)
				{
					if (ImGui::Button(u8"�����o��"))
					{
						if (select_desc)
						{
							Export(directory->GetPath(), select_name, select_desc);
						}
					}
					ImGui::SameLine();
					ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, (u8"[" + select_name + u8"]").c_str());
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if (ImGui::TreeNode(u8"�ꗗ"))
		{
			FWDESC_STATE state = LOOP;
			bool create_select_demo = false;
			while (state == LOOP)
			{
				if (desc_list.empty())
				{
					ImGui::Text(u8"���݂��܂���B");
					break;
				}
				for (auto& desc : desc_list)
				{
					bool edited = false;
					// Desc��Gui���X�V
					state = DescImGuiUpdate(device, &desc, p_parent, &edited);
					if (edited && desc.second == select_desc)
					{
						create_select_demo = true;
					}
					if (state == LOOP) break;
					else if (state == ERASE)
					{
						desc_list.erase(desc.first);
						state = LOOP;
						break;
					}
					else if (cpy_fw_desc)
					{
						Create(cpy_fw_desc->first, &cpy_fw_desc->second);
						cpy_fw_desc = nullptr;
						state = LOOP;
						break;
					}
				}
			}

			// �I�𒆂�Desc���X�V���ꂽ�̂�Demo���쐬����
			if (create_select_demo)
			{
				demo_select_clear_mutex.lock();
				add_demo_select_data.push_back(select_desc);
				demo_select_clear_mutex.unlock();
				const bool try_lock = demo_select_mutex.try_lock();
				if (try_lock) demo_select_mutex.unlock();

				if (!demo_select_th || try_lock)
				{
					if (demo_select_th && try_lock)
						demo_select_th->join();
					demo_select_th = std::make_unique<std::thread>(
						FCManager::CreateDemo, device, p_parent,
						&demo_select_mutex, &demo_select_stop_mutex,
						&add_demo_select_data, &demo_select_data,
						&demo_frame_number, &demo_select_stop,
						&demo_select_clear_mutex
						);
				}
			}

			ImGui::TreePop();
		}
	}
	ImGui::End();

	if (!demo_data.empty() || !demo_select_data.empty())
	{
		if (ImGui::Begin("Fireworks Editor [demo]"))
		{
			{
				if (ImGui::Checkbox(u8"�f���Đ�", &demo_play))
				{
					if (demo_play) PlayDemo(demo_frame_number);
					else StopDemo();
				}
				ImGui::SameLine(); HelpMarker(u8"�ۑ������f���f�[�^���Đ����܂��B");

				size_t max_frame_count = 0u;
				for (auto& data : demo_data)
				{
					std::lock_guard<std::mutex> lock(data.build_mutex);
					if (!data.build_flg && data.exist)
					{
						max_frame_count = std::max(max_frame_count, data.ptcs.size());
					}
				}
				for (auto& data : demo_select_data)
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
				if (ImGui::SliderInt(u8"�f���T���v���ԍ�", &gui_use_inum, 0, SCAST<int>(max_frame_count)))
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
					for (auto& data : demo_select_data)
					{
						std::lock_guard<std::mutex> lock(data.build_mutex);
						if (!data.build_flg && data.exist)
						{
							data.SetResource(demo_frame_number);
						}
					}
				}
				ImGui::SameLine(); HelpMarker(u8"�ۑ������f���f�[�^���m�F�ł��܂��B");
			}

			if (demo_select_itr != demo_select_data.end())
			{
				ImGui::TextColored({ 0.f, 1.f, 0.5f, 1.f }, u8"[�ҏW��] ##9999");
				ImGui::SameLine();
				if (ImGui::Button(demo_select_itr->draw_flg ? u8"��\��##9999" : u8"�\��##9999"))
				{
					demo_select_itr->draw_flg = !demo_select_itr->draw_flg;
				}
			}

			UINT idx = 0u;
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
				if (ImGui::Button(((it->draw_flg ? u8"��\��##" : u8"�\��##") + std::to_string(idx)).c_str()))
				{
					it->draw_flg = !it->draw_flg;
				}
				ImGui::SameLine();
				if (ImGui::Button((u8"�폜##" + std::to_string(idx)).c_str()))
				{
					it->exist = false;

					// �O��`��Ɏg�p���ꂽ�\��������̂ł��̃t���[���ł͏����Ȃ�
					it++;
					idx++;
					continue;
				}

				if (select_demo_number == idx)
				{
					auto* world = it->world_resource->Map(0, &CD3DX12_RANGE(0, 0));
					ImGui::InputFloat3((u8"���W##" + std::to_string(idx)).c_str(), (float*)&world->position);
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

#define MINMAX_TEXT u8"�����_���ŕϓ�����(x = min, y = max)"

FCManager::FWDESC_STATE FCManager::DescImGuiUpdate(KGL::ComPtrC<ID3D12Device> device, Desc* desc, const ParticleParent* p_parent, bool* edited)
{
	FWDESC_STATE result = FWDESC_STATE::NONE;
	if (desc && desc->second)
	{
		if (ImGui::TreeNode(desc->first.c_str()))
		{
			if (desc->second->set_name.size() < 64)
				desc->second->set_name.resize(64u);
			ImGui::InputText(("##" + std::to_string(RCAST<intptr_t>(desc))).c_str(), desc->second->set_name.data(), desc->second->set_name.size());
			ImGui::SameLine();
			if (ImGui::Button(u8"���O�ύX"))
			{
				ChangeName(desc->first, desc->second->set_name);
				ImGui::TreePop();
				return FWDESC_STATE::LOOP;
			}
			if (desc->second == select_desc)
			{
				ImGui::Text(u8"�I��");
			}
			else if (ImGui::Button(u8"�I��"))
			{
				select_desc = desc->second;
				select_name = desc->first;
				*edited = true;
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"�R�s�["))
			{
				std::string use_name = desc->second->original_name;
				if (use_name.empty()) use_name = desc->first;
				cpy_fw_desc = std::make_unique<std::pair<const std::string, FireworksDesc>>(use_name, *desc->second);
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"�폜"))
			{
				ImGui::TreePop();
				return FWDESC_STATE::ERASE;
			}
			ImGui::SameLine();

			UINT build_count = 0u;
			for (const auto& data : add_demo_data)
			{
				if (data == desc->second)
				{
					build_count++;
				}
			}
			if (ImGui::Button(u8"�f���쐬"))
			{
				add_demo_data.push_back(desc->second);
				const bool try_lock = demo_mg_mutex.try_lock();
				if (try_lock) demo_mg_mutex.unlock();

				if (!demo_mg_th || try_lock)
				{
					if (demo_mg_th && try_lock)
						demo_mg_th->join();
					demo_mg_th = std::make_unique<std::thread>(
						FCManager::CreateDemo, device, p_parent,
						&demo_mg_mutex, &demo_mg_stop_mutex,
						&add_demo_data, &demo_data,
						&demo_frame_number, &demo_mg_stop,
						nullptr
						);
				}
			}
			if (build_count > 0u)
			{
				ImGui::SameLine();
				ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.f }, (u8"[ " + std::to_string(build_count) + u8" �쐬�� ]").c_str());
			}

			
			bool edited_update = FWDescImGuiUpdate(desc->second.get());
			if (edited) *edited = edited_update || *edited;
			ImGui::TreePop();
		}
	}
	return result;
}

bool FCManager::FWDescImGuiUpdate(FireworksDesc* desc)
{
	bool edited = false;
	bool fresult;
	if (desc)
	{
		if (ImGui::TreeNode(u8"��{�p�����[�^�["))
		{
			EditCheck(ImGui::InputFloat(u8"����", &desc->mass), edited, fresult);
			EditCheck(ImGui::InputFloat(u8"����", &desc->resistivity), edited, fresult);
			EditCheck(ImGui::InputFloat(u8"�X�s�[�h", &desc->speed), edited, fresult);
			ImGui::SameLine(); HelpMarker(u8"�v���C���[����ˏo�����ꍇ�̑��x(����)�B");

			ImGui::TreePop();
		}
		if (ImGui::TreeNode(u8"�G�t�F�N�g"))
		{
			if (ImGui::TreeNode(u8"�G�t�F�N�g�쐬/�j��"))
			{
				if (ImGui::Button(u8"�G�t�F�N�g�ǉ�"))
				{
					edited = true;
					desc->effects.emplace_back(*FIREWORK_EFFECTS::FW_DEFAULT.effects.begin());
				}
				if (cpy_ef_desc)
				{
					if (ImGui::Button(u8"�\��t��"))
					{
						edited = true;
						desc->effects.emplace_back(*cpy_ef_desc);
					}
				}
				if (ImGui::TreeNode(u8"�G�t�F�N�g�폜"))
				{
					if (ImGui::Button(u8"�폜����"))
					{
						edited = true;
						desc->effects.pop_back();
					}
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}

			UINT idx = 0;
			for (auto& effect : desc->effects)
			{
				if (ImGui::TreeNode(std::string("[" + std::to_string(idx) + 
				(effect.name.empty() ? "]" : "] - " + effect.name)).c_str()))
				{
					if (effect.set_name.size() < 64)
						effect.set_name.resize(64u);
					ImGui::InputText(("##" + std::to_string(RCAST<intptr_t>(desc))).c_str(),
						effect.set_name.data(), effect.set_name.size());
					ImGui::SameLine();
					if (ImGui::Button(u8"���O�ύX"))
					{
						effect.name = effect.set_name;
						ImGui::TreePop();
					}
					if (ImGui::Button(u8"�R�s�["))
					{
						cpy_ef_desc = std::make_unique<EffectDesc>(effect);
					}
					if (ImGui::TreeNode(u8"�p�����[�^�["))
					{
						EditCheck(ImGui::InputFloat(u8"�J�n����(s)", &effect.start_time), edited, fresult);
						ImGui::SameLine(); HelpMarker(u8"�J�n���Ԃ��o�ߌ�o�����܂��B");

						EditCheck(ImGui::InputFloat(u8"�\������(s)", &effect.time);
						ImGui::SameLine(); HelpMarker(u8"�o�����Ă�����ł���܂ł̎���"), edited, fresult);

						EditCheck(ImGui::InputFloat(u8"�J�n�������{��", &effect.start_accel), edited, fresult);
						ImGui::SameLine(); HelpMarker(
							u8"�o�������Ƃ��ɐe�̑��x�ɂ��̒l��������B\n"
							u8"(���x��ς��Ȃ��ꍇ��[1.0f])"
						);
						EditCheck(ImGui::InputFloat(u8"���Ŏ������{��", &effect.end_accel), edited, fresult);
						ImGui::SameLine(); HelpMarker(
							u8"���ł����Ƃ��ɐe�̑��x�ɂ��̒l��������B\n"
							u8"(���x��ς��Ȃ��ꍇ��[1.0f])"
						);

						EditCheck(ImGui::InputFloat2(u8"�p�[�e�B�N���̕\������", (float*)&effect.alive_time), edited, fresult);
						effect.alive_time.y = (std::max)(effect.alive_time.x, effect.alive_time.y);
						ImGui::SameLine(); HelpMarker(MINMAX_TEXT);

						EditCheck(ImGui::InputFloat2(u8"�������[�g(s)", (float*)&effect.late), edited, fresult);
						effect.late.y = (std::max)(effect.late.x, effect.late.y);
						ImGui::SameLine(); HelpMarker(u8"��b�ԂɃ��[�g�̃p�[�e�B�N�����������܂�\n" MINMAX_TEXT);

						EditCheck(ImGui::InputFloat(u8"�������[�g�X�V�p�x(s)", (float*)&effect.late_update_time), edited, fresult);

						EditCheck(ImGui::InputFloat2(u8"�p�[�e�B�N���ˏo���x(m/s)", (float*)&effect.speed), edited, fresult);
						effect.speed.y = (std::max)(effect.speed.x, effect.speed.y);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���ˏo���̑��x\n" MINMAX_TEXT);
						EditCheck(ImGui::InputFloat2(u8"�ˏo�����x�̉e���x", (float*)&effect.base_speed), edited, fresult);
						effect.base_speed.y = (std::max)(effect.base_speed.x, effect.base_speed.y);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���ˏo���̎ˏo�����x�̉e���x\n" MINMAX_TEXT);

						EditCheck(ImGui::InputFloat2(u8"�p�[�e�B�N���ˏo�T�C�Y(m)", (float*)&effect.scale), edited, fresult);
						effect.scale.y = (std::max)(effect.scale.x, effect.scale.y);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���ˏo���̑傫��\n" MINMAX_TEXT);

						EditCheck(ImGui::InputFloat(u8"�ړ������ւ̃T�C�Y(�O)", &effect.scale_front), edited, fresult);
						ImGui::SameLine(); HelpMarker(u8"�ړ������O���֑��x�ɉe�����󂯕ϓ�����T�C�Y");
						EditCheck(ImGui::InputFloat(u8"�ړ������ւ̃T�C�Y(��)", &effect.scale_back), edited, fresult);
						ImGui::SameLine(); HelpMarker(u8"�ړ���������֑��x�ɉe�����󂯕ϓ�����T�C�Y");

						DirectX::XMFLOAT2 def_angle = { DirectX::XMConvertToDegrees(effect.angle.x), DirectX::XMConvertToDegrees(effect.angle.y) };
						if (ImGui::InputFloat2(u8"�ˏo�p�x(def)", (float*)&def_angle))
						{
							edited = true;
							effect.angle = { DirectX::XMConvertToRadians(def_angle.x), DirectX::XMConvertToRadians(def_angle.y) };
						}
						ImGui::SameLine(); HelpMarker(
							u8"min(x)����max(y)�̊Ԃ̊p�x�Ń����_���Ɏˏo����\n"
							u8"�ˏo�����֔�΂��ꍇ�� 0\n"
							u8"�ˏo�������΂֔�΂��ꍇ�� deg(180)"
						);

						EditCheck(ImGui::InputFloat2(u8"�ˏo�����X�y�[�X", (float*)&effect.spawn_space), edited, fresult);
						effect.spawn_space.y = (std::max)(effect.spawn_space.x, effect.spawn_space.y);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N���ˏo������spawn_space���ʒu�����炵�܂��B\n" MINMAX_TEXT);

						EditCheck(ImGui::ColorEdit4(u8"�G�t�F�N�g�������J���[", (float*)&effect.begin_color), edited, fresult);
						ImGui::SameLine(); HelpMarker(
							u8"�G�t�F�N�g�������̃p�[�e�B�N���������̃J���[�ł��B\n"
							u8"���Ŏ��J���[�Ɍ������ĕω����Ă����܂�"
						);
						EditCheck(ImGui::ColorEdit4(u8"�G�t�F�N�g���Ŏ��J���[", (float*)&effect.end_color), edited, fresult);
						ImGui::SameLine(); HelpMarker(u8"�G�t�F�N�g���Ŏ��̃p�[�e�B�N���������̃J���[�ł��B");

						EditCheck(ImGui::ColorEdit4(u8"�p�[�e�B�N�����Ŏ��J���[", (float*)&effect.erase_color), edited, fresult);
						ImGui::SameLine(); HelpMarker(u8"�p�[�e�B�N�����Ŏ��̃J���[�ł��B");

						EditCheck(ImGui::InputFloat(u8"����", &effect.resistivity), edited, fresult);
						ImGui::SameLine(); HelpMarker(u8"�s�̉e���x");
						EditCheck(ImGui::InputFloat(u8"����S", &effect.scale_resistivity), edited, fresult);
						ImGui::SameLine(); HelpMarker(u8"�s�̉e���x�i�X�P�[������e�����󂯂�j");

						EditCheck(ImGui::Checkbox(u8"�u���[��", &effect.bloom), edited, fresult);

						ImGui::TreePop();
					}

					EditCheck(ImGui::Checkbox(u8"�q��", &effect.has_child), edited, fresult);
					ImGui::SameLine(); HelpMarker(
						u8"�q���̃G�t�F�N�g�������ǂ���\n"
						u8"(���̃t���O�����ꍇ�p�[�e�B�N���ł͂Ȃ��G�t�F�N�g���ˏo���܂�)"
					);
					if (effect.has_child)
					{
						EditCheck(FWDescImGuiUpdate(&effect.child), edited, fresult);
					}

					ImGui::TreePop();
				}
				idx++;
			}

			ImGui::TreePop();
		}
	}
	return edited;
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
	for (auto& data : demo_select_data)
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
	for (const auto& data : demo_select_data)
	{
		if (!data.build_flg && data.exist)
		{
			total_size += data.Size(frame_num);
		}
	}
	return total_size;
}