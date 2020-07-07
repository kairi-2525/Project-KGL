#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <map>
//#include <unordered_map>

namespace KGL
{
	inline namespace BASE
	{
		namespace VMD
		{
			/*struct Header
			{
				FLOAT	version;
				CHAR	model_name[20];
				CHAR	comment[256];
			};*/
#pragma pack(1)
			struct Motion
			{
				CHAR				bone_name[15];
				UINT				frame_no;
				DirectX::XMFLOAT3	location;
				DirectX::XMFLOAT4	quaternion;
				UCHAR				bezier[64];		// [4][4][4] �x�W�F�ۊǃp�����[�^�[
			};

			//�\��f�[�^(���_���[�t�f�[�^)
			struct Morph
			{
				char		name[15];
				UINT32		frame_no;
				float		weight;		// 0.0f ~ 1.0f
			};	// 23�o�C�g

			struct Camera
			{
				UINT32				frame_no;
				FLOAT				distance;
				DirectX::XMFLOAT3	pos;
				DirectX::XMFLOAT3	euler_angle;
				UINT8				interpolation[24];	// ���
				UINT32				fov;
				UINT8				pers_flg;			// ON / OFF
			};	// 61�o�C�g
			
			// �Z���t�e�f�[�^
			struct SelfShadow
			{
				UINT32	frame_no;
				UINT8	mode;		// 0:�e�����A1:���[�h�P�A2:���[�h2
				FLOAT	distance;
			};
#pragma pack()
			struct Light
			{
				UINT32 frame_no;		// �t���[���ԍ�
				DirectX::XMFLOAT3 rgb;
				DirectX::XMFLOAT3 vec;	// �����x�N�g��(���s����)
			};
			struct IKEnable
			{
				// �L�[�t���[��������t���[���ԍ�
				UINT32 frame_no;
				std::map<std::string, bool> ik_enable_table;
			};

			struct Key_Frame
			{
				UINT				frame_no;	// �A�j���[�V�����J�n����̃t���[����
				DirectX::XMVECTOR	quaternion;
				DirectX::XMFLOAT3	offset;		// IK�̏������W
				DirectX::XMFLOAT2	p1, p2;		// �x�W�F�Ȑ��̒��ԃR���g���[���|�C���g

				explicit Key_Frame(UINT fno, DirectX::CXMVECTOR q,
					const DirectX::XMFLOAT3& offset,
					const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2) noexcept
					: frame_no(fno), quaternion(q), offset(offset), p1(p1), p2(p2) {}
			};

			struct Desc
			{
				std::vector<Motion>								motions;
				std::map<std::string, std::vector<Key_Frame>>	motion_data;
				UINT											max_frame;
				std::vector<Morph>								morphs;
				std::vector<Camera>								cameras;
				std::vector<Light>								lights;
				std::vector<SelfShadow>							self_shadows;
				std::vector<IKEnable>							ik_enable_data;
			};
		}
	}
}