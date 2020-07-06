#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <map>

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
#pragma pack()
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
			};
		}
	}
}