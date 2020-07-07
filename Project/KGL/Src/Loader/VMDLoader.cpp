#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <Loader/VMDLoader.hpp>
#include <Helper/ThrowAssert.hpp>
#include <fstream>

using namespace KGL;

namespace // �������O���
{
	enum class BoneType
	{
		Rotation,			// ��]
		RotAndMove,			// ��]���ړ�
		IK,					// IK
		Undefined,			// ����`
		IKChild,			// IK�e���{�[��
		RotationChild,		// ��]�e���{�[��
		IKDestination,		// IK�ڑ���
		Invisible			// �����Ȃ��{�[��
	};
}

VMD_Loader::VMD_Loader(std::filesystem::path path) noexcept
{
	using namespace DirectX;
	std::ifstream ifs(path, std::ios::in | std::ios::binary);
	try
	{
		if (!ifs.is_open()) throw std::runtime_error("[ " + path.string() + " ] ��������܂���ł����B");
		std::shared_ptr<VMD::Desc> desc = std::make_shared<VMD::Desc>();
		m_desc = desc;
		// �w�b�_�[���X�L�b�v
		ifs.seekg(50);

		UINT motion_data_num = 0u;
		ifs.read((char*)&motion_data_num, sizeof(motion_data_num));

		desc->motions.resize(motion_data_num);
		ifs.read((char*)desc->motions.data(),
			sizeof(VMD::Motion) * motion_data_num
		);

		UINT32 morph_count = 0u;
		ifs.read((char*)&morph_count, sizeof(morph_count));
		desc->morphs.resize(morph_count);
		ifs.read((char*)desc->morphs.data(), morph_count * sizeof(VMD::Morph));

		UINT32 camera_count = 0u;
		ifs.read((char*)&camera_count, sizeof(camera_count));
		desc->cameras.resize(camera_count);
		ifs.read((char*)desc->cameras.data(), camera_count * sizeof(VMD::Camera));

		UINT32 light_count = 0u;
		ifs.read((char*)&light_count, sizeof(light_count));
		desc->lights.resize(light_count);
		ifs.read((char*)desc->lights.data(), light_count * sizeof(VMD::Light));

		UINT32 self_shadow_count = 0u;
		ifs.read((char*)&self_shadow_count, sizeof(self_shadow_count));
		desc->self_shadows.resize(self_shadow_count);
		ifs.read((char*)desc->self_shadows.data(), self_shadow_count * sizeof(VMD::SelfShadow));

		UINT32 is_switch_count = 0u;
		ifs.read((char*)&is_switch_count, sizeof(is_switch_count));
		desc->ik_enable_data.resize(is_switch_count);
		for (auto& ik_enable : desc->ik_enable_data)
		{
			// �t���[���ԍ��ǂݍ���
			ifs.read((char*)&ik_enable.frame_no, sizeof(ik_enable.frame_no));
			// ���ɉ��z�t���O�����邪�A����͎g�p���Ȃ����߂P�o�C�g�V�[�N�ł��\��Ȃ�
			UINT8 visivle_flg = 0u;
			ifs.read((char*)&visivle_flg, sizeof(visivle_flg));

			// �Ώۃ{�[�����ǂݍ���
			UINT32 ik_bone_count = 0u;
			ifs.read((char*)&ik_bone_count, sizeof(ik_bone_count));
			// ���[�v�����O��ON / OFF�����擾
			for (UINT32 i = 0u; i < ik_bone_count; i++)
			{
				char ik_bone_name[20];
				ifs.read(ik_bone_name, std::size(ik_bone_name));

				UINT8 flg = 0u;
				ifs.read((char*)&flg, sizeof(flg));
				ik_enable.ik_enable_table[ik_bone_name] = flg != 0u;
			}
		}



		// VMD�̃��[�V�����f�[�^����A���ۂɎg�p���郂�[�V�����e�[�u���֕ϊ�
		desc->max_frame = 0u;
		for (auto& motion : desc->motions)
		{
			desc->max_frame = std::max(motion.frame_no, desc->max_frame);
			desc->motion_data[motion.bone_name].emplace_back(
				motion.frame_no,
				XMLoadFloat4(&motion.quaternion),
				motion.location,
				XMFLOAT2{ motion.bezier[3] / 127.f, motion.bezier[7] / 127.f },
				XMFLOAT2{ motion.bezier[11] / 127.f, motion.bezier[15] / 127.f }
			);
		}

		// �t���[���ԍ����Ƀ\�[�g
		for (auto& motion : desc->motion_data)
		{
			std::sort(
				motion.second.begin(),
				motion.second.end(),
				[](const VMD::Key_Frame lval, const VMD::Key_Frame& rval)
				{
					return lval.frame_no <= rval.frame_no;
				}
			);
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}