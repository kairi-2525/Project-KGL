#pragma once

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include "Model/StaticModel.hpp"

namespace KGL
{
	inline namespace BASE
	{
		namespace OBJ
		{
			struct ObjectData
			{
				std::vector<DirectX::XMFLOAT3>	positions;
				std::vector<DirectX::XMFLOAT2>	uvs;
				std::vector<DirectX::XMFLOAT3>	normals;
			};

			struct Object
			{
				struct Desc
				{
					size_t begin;
					size_t count;
				};

				Desc positions;
				Desc uvs;
				Desc normals;
			};

			struct Vertex
			{
				UINT position;
				UINT uv;
				UINT normal;
			};

			using Vertices = std::vector<Vertex>;
			
			struct Material
			{
				struct Texture
				{
					struct TextureMap
					{
						float gain, contrast;
					};
					std::filesystem::path	path;
					bool					blend_u;			// �����e�N�X�`��������ݒ� (�W���� on)
					bool					blend_v;			// �����e�N�X�`��������ݒ� (�W���� on)
					float					mipmap_boost;		// mip-map�̃V���[�v���������グ
					TextureMap				texture_map;		// �e�N�X�`���}�b�v�̒l��ύX (�W���� 0 1)
					DirectX::XMFLOAT3		offset_pos;			// ���_�I�t�Z�b�g (�W���� 0 0 0)
					DirectX::XMFLOAT3		offset_scale;		// �X�P�[���I�t�Z�b�g (�W���� 1 1 1)
					DirectX::XMFLOAT3		offset_turbulence;	// ���C��(?)�I�t�Z�b�g (�W���� 0 0 0)
					DirectX::XMUINT2		resolution;			// �쐬����e�N�X�`���𑜓x
					bool					clamp;				// 0����1�͈̔͂ŃN�����v���ꂽ�e�N�Z���̂݃����_�����O (�W���� off)
					float					bump_mult;			// �o���v�搔 (�o���v�}�b�v��p)
					std::string				bump_file_ch;		// �X�J���[�܂��̓o���v�e�N�X�`�����쐬���邽�߂�
																// �ǂ̃t�@�C���̃`�����l�����g�p���邩�w��B
				};
				using ReflectionsTexture = std::unordered_map<std::string, std::filesystem::path>;

				// �p�����[�^�[
				S_MODEL::Material::Parameter param;
				
				// �e�N�X�`��
				std::shared_ptr<Texture>	tex_ambient;
				std::shared_ptr<Texture>	tex_diffuse;
				std::shared_ptr<Texture>	tex_specular;
				std::shared_ptr<Texture>	tex_specular_highlights;
				std::shared_ptr<Texture>	tex_dissolve;
				std::shared_ptr<Texture>	tex_bump;
				std::shared_ptr<Texture>	tex_displacement;
				std::shared_ptr<Texture>	tex_stencil_decal;

				ReflectionsTexture			tex_reflections;

				// ���_
				Vertices					vertices;
			};

			struct Desc
			{
				std::filesystem::path										mtl_path;
				std::unordered_map<std::string, std::shared_ptr<Material>>	materials;
				std::unordered_map<std::string, std::shared_ptr<Object>>	objects;
				ObjectData													object_data;
			};
		}
	}
}