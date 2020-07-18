#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <filesystem>
#include <map>
#include <DirectXMath.h>
#include <memory>
#include <unordered_map>

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

#pragma pack(1)		// ��������P�o�C�g�A���C�����g�ɂȂ�
			struct Vertex
			{
				DirectX::XMFLOAT3	pos;
				DirectX::XMFLOAT3	normal;
				DirectX::XMFLOAT2	uv;
				USHORT				bone_no[2];		// �{�[���ԍ�
				UCHAR				bone_weight;	// �{�[���e���x
				UCHAR				edge_flg;		// �֊s���t���O
			};
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
				UINT16					bone_idx;		// IK�Ώۂ̃{�[��������
				UINT16					target_idx;		// �^�[�Q�b�g�ɋ߂Â��邽�߂̃{�[���C���f�b�N�X
				UINT16					iterations;		// ���s��
				FLOAT					limit;			// 1�񂠂���̉�]����
				std::vector<UINT16>		node_idxes;		// �Ԃ̃m�[�h�ԍ�
			};

#pragma pack(1)
			struct Morph			// ���[�t
			{
				CHAR						name[20];
				UINT32						vertex_count;
				UINT8						type;		
			};
#pragma pack()
			struct MorphVertex
			{
				UINT32						index;			// �\��p�̒��_�ԍ�
				DirectX::XMFLOAT3			position;
			};
			struct MorpthData
			{
				std::string					name;
				UINT8						type;
				std::vector<MorphVertex>	vertices;
			};
			enum MORPH_TYPE
			{
				PMD_MORPH_TYPE_BASE,
				PMD_MORPH_TYPE_EYEBROW,
				PMD_MORPH_TYPE_EYE,
				PMD_MORPH_TYPE_LIP,
				PMD_MORPH_TYPE_OTHER
			};
#pragma pack(1)
			struct BoneLabelIndex
			{
				UINT16						bone_index;
				UINT8						frame_index;
			};
			struct LocalizeHeader
			{
				UINT8						flag;		// 1 : �p��Ή�����
				CHAR						model_name[20];		// �p�ꖼ���f��
				CHAR						comment[256];
			};
			struct ToonTextureList
			{
				CHAR						file_name[10][100];
			};
#pragma pack()
			struct EnglishDesc
			{
				std::vector<std::string>			bone_names;
				std::vector<std::string>			morph_names;
				std::vector<std::string>			bone_labels;
			};
			using ToonTextureTable = std::unordered_map<UCHAR, std::filesystem::path>;
			struct Desc
			{
				std::filesystem::path				path;
				Header								header;
				std::vector<Vertex>					vertices;
				std::vector<USHORT>					indices;
				std::vector<Material>				materials;
				std::vector<Bone>					bones;
				std::vector<IK>						ik_data;
				BoneTable							bone_node_table;
				std::vector<std::string>			bone_name_array;
				std::vector<BoneNode*>				bone_node_address_array;
				std::vector<UINT32>					knee_idxes;
				std::vector<MorpthData>				morphs;
				std::vector<UINT16>					morph_label_indices;
				std::vector<std::string>			bone_labels;
				std::vector<BoneLabelIndex>			bone_label_indices;
				LocalizeHeader						localize_header;
				EnglishDesc							en;
				ToonTextureTable					toon_tex_table;
			};
			/*class AlignedVertex
			{
			public:
				Vertex vertex;
			public:
				const AlignedVertex& operator=(const Vertex& vert) noexcept
				{
					vertex.pos = vert.pos;
					vertex.uv = vert.uv;
					vertex.normal = vert.normal;
					vertex.edge_flg = vert.edge_flg;
					vertex.bone_no[0] = vert.bone_no[0];
					vertex.bone_no[1] = vert.bone_no[1];
					vertex.bone_weight = vert.bone_weight;
					return *this;
				}
				void* operator new(size_t size)
				{
					return _aligned_malloc(size, 1u);
				}
				void operator delete(void* p)
				{
					return _aligned_free(p);
				}
			};*/
			static constexpr size_t MT_SIZE = sizeof(Material);
			static constexpr size_t VERTEX_SIZE = sizeof(Vertex);
		}
	}
}