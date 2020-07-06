#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <filesystem>
#include <map>
#include <DirectXMath.h>
#include <memory>

namespace KGL
{
	inline namespace BASE
	{
		namespace PMD
		{
			struct Header
			{
				FLOAT	version;
				CHAR	model_name[20];
				CHAR	comment[256];
			};
			struct Vertex
			{
				DirectX::XMFLOAT3	pos;
				DirectX::XMFLOAT3	normal;
				DirectX::XMFLOAT2	uv;
				USHORT				bone_no[2];		// �{�[���ԍ�
				UCHAR				bone_weight;	// �{�[���e���x
				UCHAR				edge_flg;		// �֊s���t���O
			};

#pragma pack(1)		// ��������P�o�C�g�A���C�����g�ɂȂ�
			struct Material
			{
				DirectX::XMFLOAT3	diffuse;			// �f�B�t���[�Y�F
				FLOAT				alpha;				// �f�B�t���[�Ya
				FLOAT				specularity;		// �X�y�L�����̋���
				DirectX::XMFLOAT3	specular;			// �X�y�L�����F
				DirectX::XMFLOAT3	ambient;			// �A���r�G���g�F
				UCHAR				toon_idx;			// �g�D�[���ԍ�
				UCHAR				edge_flg;			// �}�e���A�����Ƃ̗֊s���t���O
				// �����ɂQ�o�C�g�̃p�f�B���O #pragma pack(1)�����邱�ƂŔ������Ȃ�
				UINT				indices_num;		// �}�e���A�������蓖�Ă���C���f�b�N�X��
				CHAR				tex_file_Path[20];	// �e�N�X�`���t�@�C���p�X+��
			};
			struct Bone
			{
				CHAR				bone_name[20];	// �{�[����
				USHORT				parent_no;		// �e�{�[���ԍ�
				USHORT				next_no;		// ��[�̃{�[���ԍ�
				UCHAR				type;			// �{�[���̎��
				// �����ɃA���C�����g���������邽�� #pragma pack(1)
				USHORT				ik_bone_no;		// IK�{�[���ԍ�
				DirectX::XMFLOAT3	pos;			// �{�[���̊�_���W
			};
#pragma pack()
			struct BoneNode
			{
				UINT32					bone_idx;		// �{�[���C���f�b�N�X
				UINT32					bone_type;		// �{�[�����
				UINT32					ik_parent_bone;	// IK�e�{�[��
				DirectX::XMFLOAT3		start_pos;		// �{�[����_�i��]�̒��S�j
				DirectX::XMFLOAT3		end_pos;		// �{�[����[�_�i���ۂ̃X�L�j���O�ɂ͗��p���Ȃ��j
				std::vector<BoneNode*>	children;		// �q�m�[�h
			};

			using BoneTable = std::map<std::string, BoneNode>;

			struct IK
			{
				UINT16				bone_idx;		// IK�Ώۂ̃{�[��������
				UINT16				target_idx;		// �^�[�Q�b�g�ɋ߂Â��邽�߂̃{�[���C���f�b�N�X
				UINT16				iterations;		// ���s��
				FLOAT				limit;			// 1�񂠂���̉�]����
				std::vector<UINT16> node_idxes;		// �Ԃ̃m�[�h�ԍ�
			};

			struct Desc
			{
				std::filesystem::path				path;
				Header								header;
				std::vector<UCHAR>					vertices;
				std::vector<USHORT>					indices;
				std::vector<Material>				materials;
				std::vector<Bone>					bones;
				std::vector<IK>						ik_data;
				BoneTable							bone_node_table;
				std::vector<std::string>			bone_name_array;
				std::vector<BoneNode*>				bone_node_address_array;
				std::vector<UINT32>					knee_idxes;
			};

			static constexpr size_t MT_SIZE = sizeof(Material);
			static constexpr size_t VERTEX_SIZE = 38u;
		}
	}
}