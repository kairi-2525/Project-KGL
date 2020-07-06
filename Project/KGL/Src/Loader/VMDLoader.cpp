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

		UINT motion_data_num = 0;
		ifs.read((char*)&motion_data_num, sizeof(motion_data_num));

		desc->motions.resize(motion_data_num);
		ifs.read((char*)desc->motions.data(),
			sizeof(VMD::Motion) * motion_data_num
		);

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