#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <filesystem>
#include <DirectXMath.h>

namespace KGL
{
	inline namespace BASE
	{
		namespace PMD
		{
			struct Header
			{
				FLOAT version;
				CHAR model_name[20];
				CHAR comment[256];
			};
			struct Vertex
			{
				DirectX::XMFLOAT3 pos;
				DirectX::XMFLOAT3 normal;
				DirectX::XMFLOAT2 uv;
				USHORT bone_no[2];		// �{�[���ԍ�
				UCHAR bone_weight;		// �{�[���e���x
				UCHAR edge_flg;			// �֊s���t���O
			};

#pragma pack(1)		// ��������P�o�C�g�A���C�����g�ɂȂ�
			struct Material
			{
				DirectX::XMFLOAT3 diffuse;	// �f�B�t���[�Y�F
				FLOAT alpha;				// �f�B�t���[�Ya
				FLOAT specularity;			// �X�y�L�����̋���
				DirectX::XMFLOAT3 specular;	// �X�y�L�����F
				DirectX::XMFLOAT3 ambient;	// �A���r�G���g�F
				UCHAR toon_idx;				// �g�D�[���ԍ�
				UCHAR edge_flg;				// �}�e���A�����Ƃ̗֊s���t���O
				// �����ɂQ�o�C�g�̃p�f�B���O #pragma pack(1)�����邱�ƂŔ������Ȃ�
				UINT indices_num;			// �}�e���A�������蓖�Ă���C���f�b�N�X��
				CHAR tex_file_Path[20];		// �e�N�X�`���t�@�C���p�X+��
			};
#pragma pack()

			struct Desc
			{
				std::filesystem::path path;
				Header header;
				std::vector<UCHAR> vertices;
				std::vector<USHORT> indices;
				std::vector<Material> materials;
			};

			static constexpr size_t MT_SIZE = sizeof(Material);
			static constexpr size_t VERTEX_SIZE = 38u;
		}
	}
}