#include "../Hrd/GUIManager.hpp"
#include "../Hrd/Fireworks.hpp"
#include "../Hrd/FireworkCerialManager.hpp"
#include "../Hrd/FireworksSpawnerManager.hpp"
#include "../Hrd/ParticleManager.hpp"
#include "../Hrd/SkyMap.hpp"
#include "../Hrd/MSAA.hpp"
#include "../Hrd/FXAA.hpp"
#include "../Hrd/Debug.hpp"
#include "../Hrd/RenderTarget.hpp"
#include "../Hrd/PlayerShotParameter.hpp"

#include <Helper/Cast.hpp>
#include <Base/Directory.hpp>

#include "../Hrd/Scenes/TestScene04.hpp"

GUIManager::GUIManager(ComPtrC<ID3D12Device> device, std::shared_ptr<KGL::DescriptorManager> imgui_descriptor)
{
	imgui_srv_desc = imgui_descriptor;

	KGL::Directory dir("./Assets/Textures/UI");

	const auto path = dir.GetPath().string() + "\\";
	const KGL::Files& files = dir.GetFiles(".png");

	textures.reserve(files.size());
	for (const auto& file_name : files)
	{
		auto tex = std::make_shared<KGL::Texture>(device, path + file_name.string());
		textures.push_back(tex);
		auto handle = std::make_shared<KGL::DescriptorHandle>(imgui_srv_desc->Alloc());
		imgui_srv_handles[file_name.stem().string()] = handle;

		// SRV���쐬
		tex->CreateSRVHandle(handle);
	}

	debug_msg_mgr = std::make_shared<DebugMsgMgr>();
}

GUIManager::~GUIManager()
{
	if (imgui_srv_desc)
	{
		for (auto& handle : imgui_srv_handles)
		{
			imgui_srv_desc->Free(*handle.second);
		}
	}
}

void GUIManager::Init()
{
	time_scale = 1.f;
	time_stop = false;
	spawn_fireworks = true;
	use_gpu = true;
	ptc_dof = false;
	dof_flg = true;
	ptc_wire = false;
	sky_draw = true;

	main_window_flag = ImGuiWindowFlags_::ImGuiWindowFlags_None;

	// �������Ȃ�
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoMove;
	// �T�C�Y��ύX�����Ȃ�
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoResize;
	// �^�C�g���o�[��\��
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar;
	// �_�u���N���b�N�ɂ��ŏ����̖�����
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse;
	// �X�N���[���o�[��\��
	main_window_flag |= ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar;

	for (auto& sub_window : sub_windows)
	{
		sub_window = SUB_WINDOW_TYPE::NONE;
	}

	CounterReset();

	debug_msg_mgr->AddMessage("���������܂����B", DebugMsgMgr::CL_SUCCESS);
}

#define FULL_UV { 0.f, 0.f }, { 1.f, 1.f }

void GUIManager::Update(
	const DirectX::XMUINT2& rt_resolution,
	const ParticleParent* p_parent,
	const std::vector<KGL::DescriptorHandle>& srv_gui_handles
)
{
	static bool open_flg = true;
	if (ImGui::Begin("GUIManager", &open_flg, main_window_flag))
	{
		UINT idx = 0u;

		// �E�B���h�E�̈ʒu�ƃT�C�Y���Œ�
		ImGui::SetWindowPos(ImVec2(0.f, (SCAST<float>(rt_resolution.y) / 5) * 4), ImGuiCond_::ImGuiCond_Once);
		ImGui::SetWindowSize(ImVec2(SCAST<float>(rt_resolution.x), SCAST<float>(rt_resolution.y) / 5), ImGuiCond_::ImGuiCond_Once);
		
		ImGui::BeginGroup();
		const ImVec2 window_size = ImGui::GetWindowSize();
		const ImVec2 play_b_size = { window_size.x / 50, window_size.y / 10 };
		const ImVec4 pause_b_tint_col = { 0.4f, 1.0f, 0.4f, 1.f };
		const ImVec4 play_b_tint_col = { 1.f, 1.f, 1.f, 1.f };
		const ImVec4 play_b_bg_col = { 0.f, 0.f, 0.f, 0.f };

		// Left Button
		if (imgui_srv_handles.count(NAME_LEFT_BUTTON) == 1u)
		{
			auto tex_id = (ImTextureID)imgui_srv_handles.at(NAME_LEFT_BUTTON)->Gpu().ptr;
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, play_b_tint_col))
			{
				time_scale = (std::max)(0.f, time_scale - TIME_ADD_SCALE);
			}
		}
		ImGui::SameLine();
		// Pause Button
		if (time_stop && imgui_srv_handles.count(NAME_PAUSE_BUTTON) == 1u)
		{
			auto tex_id = (ImTextureID)imgui_srv_handles.at(NAME_PAUSE_BUTTON)->Gpu().ptr;
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, play_b_tint_col))
			{
				time_stop = false;
			}
		}
		// Play Button
		else if (imgui_srv_handles.count(NAME_PLAY_BUTTON) == 1u)
		{
			auto tex_id = (ImTextureID)imgui_srv_handles.at(NAME_PLAY_BUTTON)->Gpu().ptr;
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, pause_b_tint_col))
			{
				time_stop = true;
			}
		}
		ImGui::SameLine();
		// Right Button
		if (imgui_srv_handles.count(NAME_RIGHT_BUTTON) == 1u)
		{
			auto tex_id = (ImTextureID)imgui_srv_handles.at(NAME_RIGHT_BUTTON)->Gpu().ptr;
			if (ImGui::ImageButton(tex_id, play_b_size, FULL_UV, -1, play_b_bg_col, play_b_tint_col))
			{
				time_scale += TIME_ADD_SCALE;
			}
		}
		ImGui::SameLine();
		// Time Scale
		ImGui::Text("Time "); ImGui::SameLine();
		ImGui::PushItemWidth(play_b_size.x * 2.f);
		ImGui::InputFloat(("##" + std::to_string(idx++)).c_str(), &time_scale, 0.f, 1.f);
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Spawn Fireworks
		ImGui::Text("Spawn"); ImGui::SameLine();
		ImGui::Checkbox(("##" + std::to_string(idx++)).c_str(), &spawn_fireworks);
		ImGui::SameLine();

		ImGui::Dummy(ImVec2(play_b_size.x / 2, play_b_size.y));
		ImGui::SameLine();

		if (ImGui::Button("Clear"))
		{
			desc.main_ptc_mgr->Clear();
			desc.player_ptc_mgr->Clear();
			desc.fireworks->clear();
			desc.player_fireworks->clear();
		}
		
		const ImVec2 info_window_size = { window_size.x / 4, -1.f };
		if (ImGui::BeginChild("Info Window", info_window_size, true, ImGuiWindowFlags_NoTitleBar))
		{
			ImGui::Text("FPS : %d", SCAST<int>(ImGui::GetIO().Framerate));
			HelperTimer("Update Count Total ", tm_update);
			HelperTimer("Render Count Total ", tm_render);
			HelperTimer("Particle Update Gpu", tm_ptc_update_gpu);
			HelperTimer("Particle Update Cpu", tm_ptc_update_cpu);
			HelperTimer("Particle Update Sort", tm_ptc_sort);

			HelperCounter("Particle Count Total", ct_ptc_total, &ct_ptc_total_max);
			HelperCounter("Firework Count Total", ct_fw, &ct_fw_max);
			HelperCounter("Particle Count Frame", ct_ptc_frame, &ct_ptc_frame_max);
		}
		ImGui::EndChild();
		ImGui::EndGroup();

		ImGui::SameLine();
		ImGui::NextColumn();

		// �T�u�E�B���h�E�ꗗ
		const ImVec2 param_window_size = { window_size.x / 10, -1.f };
		if (ImGui::BeginChild("Param Window", param_window_size, true, ImGuiWindowFlags_NoTitleBar))
		{
			if (ImGui::Button("Sky"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::SKY))
					EraseSubWindow(SUB_WINDOW_TYPE::SKY);
				else
				{
					SetSubWindow(SUB_WINDOW_TYPE::SKY, 0u);
					SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
				}
			}
			if (ImGui::Button("Particle"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::FW_EDITOR))
				{
					EraseSubWindow(SUB_WINDOW_TYPE::FW_EDITOR);
					EraseSubWindow(SUB_WINDOW_TYPE::FW_PARAM);
				}
				else
				{
					SetSubWindow(SUB_WINDOW_TYPE::FW_EDITOR, 0u);
					SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
				}
			}
			if (ImGui::Button("Spawner"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::FW_SPAWNER))
				{
					EraseSubWindow(SUB_WINDOW_TYPE::FW_SPAWNER);
				}
				else
				{
					SetSubWindow(SUB_WINDOW_TYPE::FW_SPAWNER, 0u);
					SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
				}
			}
			if (ImGui::Button("Shot"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::SHOT))
				{
					EraseSubWindow(SUB_WINDOW_TYPE::SHOT);
				}
				else
				{
					SetSubWindow(SUB_WINDOW_TYPE::SHOT, 0u);
					SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
				}
			}
			if (ImGui::Button("Render Targets"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::RT))
				{
					EraseSubWindow(SUB_WINDOW_TYPE::RT);
				}
				else
				{
					SetSubWindow(SUB_WINDOW_TYPE::RT, 0u);
					SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
				}
			}
			if (ImGui::Button("Options"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::OPTION))
				{
					EraseSubWindow(SUB_WINDOW_TYPE::OPTION);
				}
				else
				{
					SetSubWindow(SUB_WINDOW_TYPE::OPTION, 0u);
					SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
				}
			}
			if (ImGui::Button("Debug"))
			{
				if (HasSubWindow(SUB_WINDOW_TYPE::DEBUG))
				{
					EraseSubWindow(SUB_WINDOW_TYPE::DEBUG);
				}
				else
				{
					SetSubWindow(SUB_WINDOW_TYPE::DEBUG, 0u);
					SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
				}
			}
		}
		ImGui::EndChild();

		ImGui::SameLine();
		ImGui::NextColumn();

		const ImVec2 ptc_demo_window_size = { window_size.x / 3, -1.f };
		if (ImGui::BeginChild("Ptc Demo Window", ptc_demo_window_size, true, ImGuiWindowFlags_NoTitleBar))
		{
			desc.fc_mgr->UpdateDemoGui();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		const ImVec2 debug_msg_window_size = { -1.f, -1.f };
		if (ImGui::BeginChild("DebugMsg Window", debug_msg_window_size, true, ImGuiWindowFlags_NoTitleBar))
		{
			debug_msg_mgr->UpdateGui();
		}
		ImGui::EndChild();
	}
	ImGui::End();

	// �T�u�E�B���h�E��O�ɋl�߂�
	PackSubWindow();
	// �T�u�E�B���h�E
	UINT sub_window_idx = 0u;

	ComPtr<ID3D12Device> device;
	imgui_srv_desc->Heap()->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));

	for (auto& sub_window : sub_windows)
	{
		switch (sub_window)
		{
			case SUB_WINDOW_TYPE::SKY:
			{
				if (BeginSubWindow(rt_resolution, sub_window_idx, main_window_flag))
				{
					desc.sky_mgr->UpdateGui();
				}
				ImGui::End(); break;
			}
			case SUB_WINDOW_TYPE::FW_EDITOR:
			{
				auto use_flg = main_window_flag;
				// ���j���[�o�[��\��
				use_flg |= ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar;
				if (BeginSubWindow(rt_resolution, sub_window_idx, use_flg))
				{
					desc.fc_mgr->UpdateGui(device, p_parent, srv_gui_handles, debug_msg_mgr);

					auto fc_select = desc.fc_mgr->GetSelectDesc();
					if (fc_select)
					{
						SetSubWindow(SUB_WINDOW_TYPE::FW_PARAM, 1u);
					}
					else
					{
						SetSubWindow(SUB_WINDOW_TYPE::NONE, 1u);
					}
				}
				ImGui::End(); break;
			}
			case SUB_WINDOW_TYPE::FW_PARAM:
			{
				auto use_flg = main_window_flag;
				// �^�C�g���o�[��߂�
				use_flg &= (~ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
				// ���X�N���[���o�[��ǉ�
				use_flg |= ImGuiWindowFlags_::ImGuiWindowFlags_HorizontalScrollbar;
				// �X�N���[���o�[��\����߂�
				use_flg &= (~ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollbar);
				auto fc_select = desc.fc_mgr->GetSelectDesc();
				if (fc_select)
				{
					auto title = desc.fc_mgr->GetName(fc_select);
					if (BeginSubWindow(rt_resolution, sub_window_idx, use_flg, title))
					{
						if (desc.fc_mgr->FWDescImGuiUpdate(fc_select.get(), srv_gui_handles))
						{
							desc.fc_mgr->CreateSelectDemo(device, p_parent);
						}
					}
					ImGui::End();
				}
				break;
			}
			case SUB_WINDOW_TYPE::FW_SPAWNER:
			{
				auto use_flg = main_window_flag;
				// ���j���[�o�[��\��
				use_flg |= ImGuiWindowFlags_::ImGuiWindowFlags_MenuBar;
				if (BeginSubWindow(rt_resolution, sub_window_idx, use_flg))
				{
					desc.fs_mgr->UpdateGui(desc.fc_mgr->GetDescList(), debug_msg_mgr);
				}
				ImGui::End(); break;
			}
			case SUB_WINDOW_TYPE::SHOT:
			{
				if (BeginSubWindow(rt_resolution, sub_window_idx, main_window_flag))
				{
					UpdatePlShot();
				}
				ImGui::End(); break;
			}
			case SUB_WINDOW_TYPE::RT:
			{
				if (BeginSubWindow(rt_resolution, sub_window_idx, main_window_flag))
				{
					UpdateRtOption();
				}
				ImGui::End(); break;
			}
			case SUB_WINDOW_TYPE::OPTION:
			{
				if (BeginSubWindow(rt_resolution, sub_window_idx, main_window_flag))
				{
					UpdatePtcOption(rt_resolution);
				}
				ImGui::End(); break;
			}
			case SUB_WINDOW_TYPE::DEBUG:
			{
				if (BeginSubWindow(rt_resolution, sub_window_idx, main_window_flag))
				{
					desc.debug_mgr->UpdateGui();
				}
				ImGui::End(); break;
			}
		}
		sub_window_idx++;
	}
}

void GUIManager::UpdatePtcOption(const DirectX::XMUINT2& rt_resolution)
{
	UINT idx = 0u;

	ImGui::Text(u8"[�p�[�e�B�N��]");
	ImGui::Indent(16.0f);
	ImGui::Checkbox(u8"GPU�X�V", &use_gpu);

	{
		const auto& PTC_VT_TABLE = TestScene04::PTC_VT_TABLE;
		std::string select_vt = PTC_VT_TABLE[ptc_vt_type];

		ImGui::Text(u8"���_�^�C�v");
		if (ImGui::BeginCombo(("##" + std::to_string(idx++)).c_str(), select_vt.c_str()))
		{
			for (int n = 0; n < PTC_VT_TABLE.size(); n++)
			{
				// �I�u�W�F�N�g�̊O���܂��͓����ɁA�I����e���D���Ȃ悤�ɕۑ��ł��܂�
				bool is_selected = (select_vt == PTC_VT_TABLE[n]);
				if (ImGui::Selectable(PTC_VT_TABLE[n].c_str(), is_selected))
					ptc_vt_type = SCAST<TestScene04::PTC_VT>(n);
				if (is_selected)
					// �R���{���J���Ƃ��ɏ����t�H�[�J�X��ݒ�ł��܂��i�L�[�{�[�h�i�r�Q�[�V�����T�|�[�g�̏ꍇ��+���X�N���[�����܂��j
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	ImGui::Checkbox(u8"���C���t���[��", &ptc_wire);
	ImGui::Checkbox(u8"�[�x�l��������", &ptc_dof);
	{
		ImGui::Text(u8"[�u���[��]");
		ImGui::Indent(16.0f);
		ImGui::Text(u8"�X�P�[��");
		int bloom_scale = KGL::SCAST<int>(desc.bloom_generator->GetKernel());
		if (ImGui::SliderInt(("##" + std::to_string(idx++)).c_str(), &bloom_scale, 0, KGL::SCAST<int>(BloomGenerator::RTV_MAX)))
		{
			desc.bloom_generator->SetKernel(KGL::SCAST<UINT8>(bloom_scale));
		}
		if (ImGui::TreeNode(u8"�e�X�P�[���̉e���x"))
		{
			auto weights = desc.bloom_generator->GetWeights();
			bool weights_changed = false;
			for (UINT i = 0u; i < BloomGenerator::RTV_MAX; i++)
			{
				bool changed = ImGui::SliderFloat(std::to_string(i).c_str(), &weights[i], 0.f, 1.f);
				weights_changed = weights_changed || changed;
			}
			if (weights_changed) desc.bloom_generator->SetWeights(weights);
			ImGui::TreePop();
		}
		ImGui::Unindent(16.0f);
	}
	ImGui::Unindent(16.0f);

	ImGui::Spacing();

	ImGui::Text(u8"[�ԉ�]");
	ImGui::Indent(16.0f);
	ImGui::Checkbox(u8"����", &spawn_fireworks);
	ImGui::Unindent(16.0f);

	ImGui::Spacing();

	ImGui::Text(u8"[��ʊE�[�x]");
	ImGui::Indent(16.0f);
	{
		ImGui::Checkbox(u8"�g�p����", &dof_flg);
		ImGui::Text(u8"�X�P�[��");
		int dof_scale = KGL::SCAST<int>(desc.dof_generator->GetRtvNum());
		if (ImGui::SliderInt(("##" + std::to_string(idx++)).c_str(), &dof_scale, 0, KGL::SCAST<int>(DOFGenerator::RTV_MAX)))
		{
			desc.dof_generator->SetRtvNum(KGL::SCAST<UINT8>(dof_scale));
		}
	}
	ImGui::Unindent(16.0f);

	ImGui::Spacing();

	ImGui::Text(u8"[��]");
	ImGui::Indent(16.0f);
	ImGui::Checkbox(u8"�`��", &sky_draw);
	ImGui::Unindent(16.0f);

	ImGui::Spacing();

	ImGui::Text(u8"[�A���`�G�C���A�X]");
	ImGui::Indent(16.0f);
	{
		bool use_fxaa = desc.fxaa_mgr->IsActive();
		bool msaa_off = MSAASelector::MSAA_OFF == desc.msaa_selector->GetScale();

		if (ImGui::RadioButton("OFF", !use_fxaa && msaa_off))
		{
			use_fxaa = false;
			desc.msaa_selector->SetScale(MSAASelector::MSAA_OFF);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("MSAA", !use_fxaa && !msaa_off))
		{
			use_fxaa = false;
			if (msaa_off)
			{
				desc.msaa_selector->SetScale(MSAASelector::MSAAx2);
			}
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("FXAA", use_fxaa))
		{
			use_fxaa = true;
		}
		desc.fxaa_mgr->SetActiveFlg(use_fxaa);

		if (use_fxaa)
		{
			ImGui::Text(u8"[FXAA]");
			ImGui::Indent(16.0f);
			{
				idx = desc.fxaa_mgr->UpdateGui(rt_resolution, idx);
			}
			ImGui::Unindent(16.0f);
		}
		else
		{
			ImGui::Text(u8"[MSAA]");
			ImGui::Indent(16.0f);
			std::string select_msaa = desc.msaa_combo_texts->at(SCAST<UINT>(desc.msaa_selector->GetScale()));

			// 2�Ԗڂ̃p�����[�^�[�́A�R���{���J���O�Ƀv���r���[����郉�x���ł��B
			if (ImGui::BeginCombo(u8"", select_msaa.c_str()))
			{
				for (int n = 0; n < desc.msaa_combo_texts->size(); n++)
				{
					// �I�u�W�F�N�g�̊O���܂��͓����ɁA�I����e���D���Ȃ悤�ɕۑ��ł��܂�
					bool is_selected = (select_msaa == desc.msaa_combo_texts->at(n));
					if (ImGui::Selectable(desc.msaa_combo_texts->at(n).c_str(), is_selected))
						desc.msaa_selector->SetScale(SCAST<MSAASelector::TYPE>(n));
					if (is_selected)
						// �R���{���J���Ƃ��ɏ����t�H�[�J�X��ݒ�ł��܂��i�L�[�{�[�h�i�r�Q�[�V�����T�|�[�g�̏ꍇ��+���X�N���[�����܂��j
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::Unindent(16.0f);
		}
	}
	ImGui::Unindent(16.0f);
}

//#define BORDER_COLOR(color) ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(1.f, 1.f, 1.f, 1.f), color
void GUIManager::UpdateRtOption()
{
	const auto bd_color = ImGui::GetStyle().Colors[ImGuiCol_Border];
	auto gui_size = ImGui::GetWindowSize();
	const float x_size = gui_size.x * 0.8f;
	ImVec2 image_size = { x_size, (x_size / 16) * 9.f };
	if (ImGui::TreeNode("DSV"))
	{
		//ImGui::GetWindowDrawList()->AddImage((ImTextureID)it.imgui_handle.Gpu().ptr,
		// ImVec2(0, 0), image_size, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 0, 0, 255));

		HelperZoomImage((ImTextureID)desc.rt_resources->at(MSAASelector::TYPE::MSAA_OFF).depth_gui_srv_handle.Gpu().ptr, image_size);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Particles"))
	{
		HelperZoomImage((ImTextureID)desc.rt_resources->at(MSAASelector::TYPE::MSAA_OFF).render_targets[TestScene04::PTC_NON_BLOOM].gui_srv_handle.Gpu().ptr, image_size);
		HelperZoomImage((ImTextureID)desc.rt_resources->at(MSAASelector::TYPE::MSAA_OFF).render_targets[TestScene04::PTC_BLOOM].gui_srv_handle.Gpu().ptr, image_size);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Bloom"))
	{
		ImVec2 halh_image_size = { image_size.x * 0.5f, image_size.y * 0.5f };
		UINT idx = 0u;
		ImGui::Text("Compression");
		for (const auto& handle : bl_c_imgui_handles)
		{
			HelperZoomImage((ImTextureID)handle.Gpu().ptr, halh_image_size);
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		idx = 0u;
		ImGui::Text("Width Blur");
		for (const auto& handle : bl_w_imgui_handles)
		{
			HelperZoomImage((ImTextureID)handle.Gpu().ptr, halh_image_size);
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		idx = 0u;
		ImGui::Text("Height Blur");
		for (const auto& handle : bl_h_imgui_handles)
		{
			HelperZoomImage((ImTextureID)handle.Gpu().ptr, halh_image_size);
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		ImGui::Text("Result");
		HelperZoomImage((ImTextureID)bl_bloom_imgui_handle.Gpu().ptr, halh_image_size);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("DOF"))
	{
		ImVec2 halh_image_size = { image_size.x * 0.5f, image_size.y * 0.5f };
		UINT idx = 0u;
		for (const auto& handle : dof_imgui_handles)
		{
			HelperZoomImage((ImTextureID)handle.Gpu().ptr, halh_image_size);
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		ImGui::TreePop();
	}
}
void GUIManager::UpdatePlShot()
{
	ImGui::Text(u8"[�V���b�g]");
	ImGui::Indent(16.0f);
	ImGui::Checkbox(u8"�����_���J���[", &desc.pl_shot_param->random_color);
	ImGui::Checkbox(u8"���ʂ�ݒ肷��", &desc.pl_shot_param->use_mass);

	if (desc.pl_shot_param->use_mass)
	{
		ImGui::InputFloat(u8"����", &desc.pl_shot_param->mass);
		if (ImGui::Button(u8"�u���b�N�z�[��"))
			desc.pl_shot_param->mass = PlayerShotParametor::BLACK_HOLL_MASS;
		ImGui::SameLine();
		if (ImGui::Button(u8"�z���C�g�z�[��"))
			desc.pl_shot_param->mass = PlayerShotParametor::WHITE_HOLL_MASS;
	}

	ImGui::Unindent(16.0f);
}

// �T�u�E�B���h�E�����W�Ȃǂ��Z�b�g������Ԃ�Begin����
bool GUIManager::BeginSubWindow(
	const DirectX::XMUINT2& rt_resolution,
	UINT num,
	ImGuiWindowFlags flags,
	std::string title)
{
	static bool open_flg = true;
	const ImVec2 base_size = { SCAST<float>(rt_resolution.x) / 5, SCAST<float>(rt_resolution.y) / 5.f };
	const ImVec2 window_size = ImVec2(base_size.x * 1, base_size.y * 4);
	UINT offset = 4 - num;

	ImGui::SetNextWindowContentSize(ImVec2(window_size.x * 2, 0.f));
	if (ImGui::Begin((title + "##" + std::to_string(num)).c_str(), &open_flg, flags))
	{
		ImGui::SetWindowPos(ImVec2(base_size.x * offset, 0.f), ImGuiCond_::ImGuiCond_Once);
		ImGui::SetWindowSize(window_size, ImGuiCond_::ImGuiCond_Once);
		return true;
	}
	return false;
}

// �󂢂Ă���ꏊ��T���ăT�u�E�B���h�E���Z�b�g����
// force �ōŌ�̃E�B���h�E�������I�ɒu��������
bool GUIManager::SetSubWindow(SUB_WINDOW_TYPE type, bool force)
{
	for (auto& sub_window : sub_windows)
	{
		if (sub_window == type)
		{
			return false;
		}
		if (sub_window == SUB_WINDOW_TYPE::NONE)
		{
			sub_window = type;
			return true;
		}
	}
	if (force)
	{
		sub_windows.back() = type;
		return true;
	}

	return false;
}

// num �Ԗڂ̃E�B���h�E�𖳗����u��������
void GUIManager::SetSubWindow(SUB_WINDOW_TYPE type, UINT num)
{
	if (sub_windows.size() > SCAST<size_t>(num))
	{
		sub_windows[num] = type;
	}
}

bool GUIManager::EraseSubWindow(SUB_WINDOW_TYPE type)
{
	for (auto& sub_window : sub_windows)
	{
		if (sub_window == type)
		{
			sub_window = SUB_WINDOW_TYPE::NONE;
			return true;
		}
	}
	return false;
}

// �w�肵���^�C�v�����݂��邩�m�F����
bool GUIManager::HasSubWindow(SUB_WINDOW_TYPE type)
{
	for (auto& sub_window : sub_windows)
	{
		if (sub_window == type)
		{
			return true;
		}
	}
	return false;
}

// NONE�������đO�ɋl�߂�
void GUIManager::PackSubWindow()
{
	SUB_WINDOW_TYPE* p_none_window = nullptr;
	for (auto& sub_window : sub_windows)
	{
		if (sub_window == SUB_WINDOW_TYPE::NONE)
		{
			p_none_window = &sub_window;
		}
		else if (p_none_window)
		{
			*p_none_window = sub_window;
			sub_window = SUB_WINDOW_TYPE::NONE;
			p_none_window = &sub_window;
		}
	}
}

void GUIManager::CounterReset()
{
	ct_fw = ct_fw_max = 0u;
	ct_ptc_total = ct_ptc_total_max = 0u;
	ct_ptc_frame = ct_ptc_frame_max = 0u;

	tm_update.Clear();
	tm_render.Clear();
	tm_ptc_update_gpu.Clear();
	tm_ptc_update_cpu.Clear();
	tm_ptc_sort.Clear();
}

// Imgui Timer�N���X�w���p�[
void GUIManager::HelperTimer(const std::string& title, const KGL::Timer& timer, KGL::Timer::SEC sec_type)
{
	const std::string title_text = (title + " [ %5d ][ %5d ][ %5d ][ %5d ]");
	switch (sec_type)
	{
		case KGL::Timer::SEC::MICRO:
			ImGui::Text(title_text.c_str(), timer.Last().micro, timer.Average().micro, timer.Min().micro, timer.Max().micro);
			break;
		case KGL::Timer::SEC::NANO:
			ImGui::Text(title_text.c_str(), timer.Last().nano, timer.Average().nano, timer.Min().nano, timer.Max().nano);
			break;
		default:
			ImGui::Text(title_text.c_str(), timer.Last().milli, timer.Average().milli, timer.Min().milli, timer.Max().milli);
			break;
	}
}
// Imgui Counter �w���p�[
void GUIManager::HelperCounter(const std::string& title, UINT64 count, UINT64* max_count)
{
	if (max_count)
	{
		const std::string title_text = (title + " [ %5d ] : [ %5d ]");
		*max_count = (std::max)(count, *max_count);
		ImGui::Text(title_text.c_str(), count, *max_count);
	}
}

DirectX::XMUINT2 GUIManager::GetNoWindowSpace(const DirectX::XMUINT2& rt_resolution) const
{
	const ImVec2 base_size = { SCAST<float>(rt_resolution.x) / 5, SCAST<float>(rt_resolution.y) / 5 };
	auto result = rt_resolution;
	UINT offset = 0u;
	UINT i = 0u;
	for (const auto& sub_window : sub_windows)
	{
		i++;
		if (sub_window != SUB_WINDOW_TYPE::NONE)
			offset = i;
	}

	result.x -= SCAST<UINT>(base_size.x * offset);
	result.y -= SCAST<UINT>(base_size.y);

	return result;
}

void GUIManager::HelperZoomImage(ImTextureID image, ImVec2 tex_size)
{
	auto& io = ImGui::GetIO();
	
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
	ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
	ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
	ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
	ImGui::Image(image, tex_size, uv_min, uv_max, tint_col, border_col);
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		float region_sz = 32.0f;
		float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
		float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
		float zoom = 4.0f;
		if (region_x < 0.0f) { region_x = 0.0f; }
		else if (region_x > tex_size.x - region_sz) { region_x = tex_size.x - region_sz; }
		if (region_y < 0.0f) { region_y = 0.0f; }
		else if (region_y > tex_size.y - region_sz) { region_y = tex_size.y - region_sz; }
		ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
		ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
		ImVec2 uv0 = ImVec2((region_x) / tex_size.x, (region_y) / tex_size.y);
		ImVec2 uv1 = ImVec2((region_x + region_sz) / tex_size.x, (region_y + region_sz) / tex_size.y);
		ImGui::Image(image, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, tint_col, border_col);
		ImGui::EndTooltip();
	}
}