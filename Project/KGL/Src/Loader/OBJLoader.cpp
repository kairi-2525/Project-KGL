#include <Loader/OBJLoader.hpp>
#include <Helper/Convert.hpp>
#include <Helper/Cast.hpp>

using namespace KGL;

OBJ_Loader::OBJ_Loader(const std::filesystem::path& path) noexcept :
	StaticModelLoader(path)
{
	const auto& n_path = GetPath();
	std::ifstream ifs(n_path);

	auto desc = std::make_shared<OBJ::Desc>();

	bool smooth = false;

	try
	{
		if (!ifs.is_open())
		{
			throw std::runtime_error(
				"[ " + n_path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
				+ "[�t�@�C�����J���܂���]"
			);
		}

		// �g���q�̑��݊m�F
		CheckExtensiton(".OBJ");

		// �e�L�X�g���[�h�Ńt�@�C�����J�����Ƃ��ɔ������邸���
		// �o�b�t�@�����O�𖳌��ɂ��邱�Ƃŉ�������
		ifs.rdbuf()->pubsetbuf(nullptr, 0);

		// �t�@�C���̑��΃p�X��ۑ�
		auto r_path = n_path;
		r_path.remove_filename();

		std::string buff;
		buff.reserve(256);

		// �t�@�C���̖��[�܂Ń��[�v
		while (!ifs.eof())
		{
			buff.clear();

			auto old_pos = ifs.tellg();

			ifs >> buff;

			// �󔒂����������EOF�ɂȂ�Ȃ��������
			if (buff.empty())
			{
				if (ifs.get() == EOF)
					break;
			}

			// �R�����g�A�E�g���X�L�b�v
			if (buff[0] == '#')
			{
				std::getline(ifs, buff);
				continue;
			}

			// �啶���œ���
			buff = CONVERT::StrToUpper(buff);

			// MTL�t�@�C����ǂݍ���
			if (buff == "MTLLIB")
			{
				// �t�@�C���p�X��ǂݍ��݁A���K��
				ifs >> desc->mtl_path;
				LoadMTLFile(r_path, desc);
			}

			// �I�u�W�F�N�g�f�[�^
			else if (buff == "O" || buff == "V" || buff == "VT" || buff == "VN")
			{
				std::string obj_name;
				if (buff == "O")
				{
					// �ŏ��̋󔒂��΂��āA��s�R�s�[
					if (SCAST<char>(ifs.get()) != ' ')
					{
						throw std::runtime_error(
							"[ " + n_path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
							+ "[�t�H�[�}�b�g�G���[]"
						);
					}
					// �I�u�W�F�N�g�̖��O���擾
					std::getline(ifs, obj_name);
				}
				else
				{
					// ����ȊO�������ꍇ�́h�h�ɓǂݍ���
					ifs.seekg(old_pos);
				}

				// �I�u�W�F�N�g��ǂݍ���
				auto objects = desc->objects[obj_name] = std::make_shared<OBJ::Object>();
				objects->positions.begin = desc->object_data.positions.size();
				objects->uvs.begin = desc->object_data.uvs.size();
				objects->normals.begin = desc->object_data.normals.size();

				LoadObjects(ifs, desc, desc->objects[obj_name]);
			}

			// �}�e���A���f�[�^
			else if (buff == "USEMTL")
			{
				// �ŏ��̋󔒂��΂��āA��s�R�s�[
				if (SCAST<char>(ifs.get()) != ' ')
				{
					throw std::runtime_error(
						"[ " + n_path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
						+ "[�t�H�[�}�b�g�G���[]"
					);
				}
				// ��s�R�s�[
				std::string tex_path;
				std::getline(ifs, tex_path);

				auto materials = desc->materials[tex_path];
				
				// �܂�����Ă��Ȃ��ꍇ�쐬
				if (!materials)
				{
					materials = std::make_shared<OBJ::Material>();
					desc->materials[tex_path] = materials;
				}

				// �}�e���A���f�[�^��ǂݍ���
				smooth = LoadMaterials(ifs, r_path, desc, materials, smooth);
			}
		}
		// �R���e�i�̃L���p�V�e�B��؂�l�߂�
		desc->object_data.positions.shrink_to_fit();
		desc->object_data.uvs.shrink_to_fit();
		desc->object_data.normals.shrink_to_fit();

		// const �̃����o�ɏ��n
		m_desc = desc;

		// StaticModel�p�̃}�e���A���ɕϊ�
		ConvertMaterial();
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}
}

static void InitTexture(std::shared_ptr<OBJ::Material::Texture> out_texture)
noexcept
{
	out_texture->blend_u = true;
	out_texture->blend_v = true;

	out_texture->mipmap_boost = 0.f;
	out_texture->texture_map = { 0.f, 1.f };

	out_texture->offset_pos = { 0.f, 0.f, 0.f };
	out_texture->offset_scale = { 1.f, 1.f, 1.f };
	out_texture->offset_turbulence = { 0.f, 0.f, 0.f };

	out_texture->resolution = { 0u, 0u };
	out_texture->clamp = false;
	out_texture->bump_mult = 0.f;

	out_texture->bump_file_ch = "L";
}

static bool LoadTexture(
	const std::filesystem::path& r_path,
	std::ifstream& ifs,
	std::shared_ptr<OBJ::Material::Texture> out_texture
) noexcept
{
	// �e�N�X�`���p�����[�^������
	InitTexture(out_texture);

	std::string buff;
	buff.reserve(256);

	while (!ifs.eof())
	{
		auto old_pos = ifs.tellg();

		// �ŏ��̋󔒂��΂�
		if (SCAST<char>(ifs.get()) != ' ')
		{
			return false;
		}

		// "-"�łȂ���΃e�N�X�`���p�X
		if (SCAST<char>(ifs.get()) != '-')
		{
			ifs.seekg(old_pos);

			// �ŏ��̋󔒂��΂��āA��s�R�s�[
			if (SCAST<char>(ifs.get()) != ' ')
			{
				return false;
			}
			// ��s�R�s�[
			std::string tex_path;
			std::getline(ifs, tex_path);
			out_texture->path = tex_path;

			// �p�X�̐��K��
			out_texture->path = out_texture->path.lexically_normal();
			// ���΃p�X�̏ꍇ
			if (out_texture->path.is_relative())
			{
				out_texture->path = r_path / out_texture->path;
			}

			return true;
		}

		buff.clear();

		// �啶���œ���
		buff = CONVERT::StrToUpper(buff);

		// �����e�N�X�`��������ݒ�
		if (buff == "-BLENDU")
		{
			std::string flg_str;
			ifs >> flg_str;
			flg_str = CONVERT::StrToUpper(flg_str);

			if (flg_str == "OFF")
			{
				out_texture->blend_u = false;
			}
		}
		// �����e�N�X�`��������ݒ� 
		else if (buff == "-BLENDV")
		{
			std::string flg_str;
			ifs >> flg_str;
			flg_str = CONVERT::StrToUpper(flg_str);

			if (flg_str == "OFF")
			{
				out_texture->blend_v = false;
			}
		}

		// mip-map�̃V���[�v���������グ
		else if (buff == "-BOOST")
		{
			ifs >> out_texture->mipmap_boost;
		}
		// �e�N�X�`���}�b�v�̒l��ύX (�W���� 0 1)
		else if (buff == "-MM")
		{
			ifs >> out_texture->texture_map.gain;
			ifs >> out_texture->texture_map.contrast;
		}

		// ���_�I�t�Z�b�g
		else if (buff == "-O")
		{
			ifs >> out_texture->offset_pos.x;
			ifs >> out_texture->offset_pos.y;
			ifs >> out_texture->offset_pos.z;
		}
		// �X�P�[��
		else if (buff == "-S")
		{
			ifs >> out_texture->offset_scale.x;
			ifs >> out_texture->offset_scale.y;
			ifs >> out_texture->offset_scale.z;
		}
		// Turbulence
		else if (buff == "-T")
		{
			ifs >> out_texture->offset_turbulence.x;
			ifs >> out_texture->offset_turbulence.y;
			ifs >> out_texture->offset_turbulence.z;
		}
		// �쐬����e�N�X�`���𑜓x
		else if (buff == "-TEXRES")
		{
			ifs >> out_texture->resolution.x;
			ifs >> out_texture->resolution.y;
		}
		// 0����1�͈̔͂ŃN�����v���ꂽ�e�N�Z���̂݃����_�����O (�W���� off)
		else if (buff == "-CLAMP")
		{
			std::string flg_str;
			ifs >> flg_str;
			flg_str = CONVERT::StrToUpper(flg_str);

			if (flg_str == "ON")
			{
				out_texture->blend_v = true;
			}
		}
		// �o���v�搔
		else if (buff == "-BM")
		{
			ifs >> out_texture->bump_mult;
		}
		// �X�J���[�܂��̓o���v�e�N�X�`�����쐬���邽�߂�
		else if (buff == "-IMFCHAN")
		{
			ifs >> out_texture->bump_file_ch;
		}

		// �s���l
		else
		{
			ifs.seekg(old_pos);
			return false;
		}
	}

	return false;
}

static void InitMTL(std::shared_ptr<OBJ::Material> out_material) noexcept
{
	out_material->param.refraction = 1.0f;
	out_material->param.ambient_color = { 0.5f, 0.5f, 0.5f };
	out_material->param.specular_color = { 1.0f, 1.0f, 1.0f };
	out_material->param.specular_weight = 0.f;
	out_material->param.dissolve = 1.f;
	out_material->param.smooth = true;
}

static void LoadMTL(
	const std::filesystem::path& r_path,
	const std::filesystem::path& path,
	std::ifstream& ifs,
	std::shared_ptr<OBJ::Material> out_material
) noexcept(false)
{
	std::string buff;
	buff.reserve(256);

	// �ǂݍ��܂�Ȃ������p�����[�^�[��
	// �s��l�ɂȂ�Ȃ��悤�ɏ���������
	InitMTL(out_material);

	while (!ifs.eof())
	{
		buff.clear();

		// �߂����߂̍��W��ۊ�
		std::ifstream::pos_type old_pos = ifs.tellg();

		ifs >> buff;

		// �R�����g�A�E�g���X�L�b�v
		if (buff[0] == '#')
		{
			std::getline(ifs, buff);
			continue;
		}

		// �啶���œ���
		buff = CONVERT::StrToUpper(buff);

		if (buff == "KA")
		{
			// 0 ~ 1
			ifs >> out_material->param.ambient_color.x;
			ifs >> out_material->param.ambient_color.y;
			ifs >> out_material->param.ambient_color.z;
		}
		else if (buff == "KD")
		{
			// 0 ~ 1
			ifs >> out_material->param.diffuse_color.x;
			ifs >> out_material->param.diffuse_color.y;
			ifs >> out_material->param.diffuse_color.z;
		}
		else if (buff == "KS")
		{
			// 0 ~ 1
			ifs >> out_material->param.specular_color.x;
			ifs >> out_material->param.specular_color.y;
			ifs >> out_material->param.specular_color.z;
		}
		else if (buff == "NS")
		{
			// 0 ~ 1000
			ifs >> out_material->param.specular_weight;
			// 0 ~ 1
			out_material->param.specular_weight /= 1000.f;
		}

		// �����x
		else if (buff == "D")
		{
			// 0 ~ 1
			ifs >> out_material->param.dissolve;
		}
		// D�̔��]
		else if (buff == "TR")
		{
			// 0 ~ 1
			ifs >> out_material->param.dissolve;
			out_material->param.dissolve = 1.f - out_material->param.dissolve;
		}

		// ���w���x(���ܗ�)
		else if (buff == "NI")
		{
			// 0.001 ~ 10
			// 1.0�̒l�͌����I�u�W�F�N�g���p�X�X���[���Ȃ���Ȃ����Ƃ��Ӗ�����B
			ifs >> out_material->param.refraction;
		}
		// �Ɩ����f��
		else if (buff == "ILLUM")
		{
			// 1�ŋ��ʔ��˖���, 2�ŗL��
			int flg_value;
			ifs >> flg_value;
			out_material->param.specular_flg = flg_value == 2;
		}

		// �A���r�G���g�e�N�X�`���}�b�v
		else if (buff == "MAP_KA")
		{
			out_material->tex_ambient = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_ambient))
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
		}
		// �f�B�t���[�Y�e�N�X�`���}�b�v (�����̏ꍇ�A�A���r�G���g�e�N�X�`���}�b�v�Ɠ����ɂ����)
		else if (buff == "MAP_KD")
		{
			out_material->tex_diffuse = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_diffuse))
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
		}
		// �X�y�L�����J���[�e�N�X�`���}�b�v
		else if (buff == "MAP_KS")
		{
			out_material->tex_specular = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_specular))
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
		}
		// �X�y�L�����n�C���C�g����
		else if (buff == "MAP_NS")
		{
			out_material->tex_specular_highlights = std::make_shared<OBJ::Material::Texture>();
			if(!LoadTexture(r_path, ifs, out_material->tex_specular_highlights))
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
		}
		// ���ߓx�e�N�X�`���}�b�v
		else if (buff == "MAP_D")
		{
			out_material->tex_dissolve = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_dissolve))
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
		}
		// �o���v�}�b�v(�W���ŉ摜�̋P�x�l�`�����l�����g�p)
		else if (buff == "MAP_BUMP" || buff == "BUMP")
		{
			out_material->tex_bump = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_bump))
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
		}
		// �f�B�X�v���[�X�����g�}�b�v
		else if (buff == "MAP_DISP" || buff == "DISP")
		{
			out_material->tex_displacement = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_displacement))
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
		}
		// �X�e���V���f�J�[���e�N�X�`�� (�W���ŉ摜��'matte'�`�����l�����g�p)
		else if (buff == "MAP_DECAL" || buff == "DECAL")
		{
			out_material->tex_stencil_decal = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_stencil_decal))
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
		}

		// ���˃}�b�v
		else if (buff == "REFL")
		{
			std::string type_str;
			ifs >> type_str;
			type_str = CONVERT::StrToUpper(type_str);

			if (type_str != "-TYPE")
			{
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
			}
			type_str.clear();
			ifs >> type_str;
			type_str = CONVERT::StrToUpper(type_str);

			// �ŏ��̋󔒂��΂��āA��s�R�s�[
			if (SCAST<char>(ifs.get()) != ' ')
				throw std::runtime_error("[ " + path.string() + " ] �̓ǂݍ��݂Ɏ��s\n[�t�H�[�}�b�g�G���[]");
			// ��s�R�s�[
			std::string reflection_str;
			std::getline(ifs, reflection_str);
			out_material->tex_reflections[type_str] = reflection_str;
		}

		// �����x�[�X�����_�����O�p�p�����[�^�[
		else if (
			buff == "MAP_PR" || buff == "PR" ||	// ���t�l�X
			buff == "MAP_PM" || buff == "PM" ||	// ���^���b�N
			buff == "MAP_PS" || buff == "PS" ||	// Sheen
			buff == "PC" ||						// �N���A�R�[�g�̌���
			buff == "PCR" ||					// �N���A�R�[�g�̃��t�l�X
			buff == "MAP_KE" || buff == "KE" ||	// ����
			buff == "ANISO" ||					// �ٕ���
			buff == "ANISOR" ||					// �ٕ����̉�]
			buff == "NORM"						// �@���}�b�v�A"bump" �p�����[�^�[�Ɠ����`��
		){
			// ��s��΂�
			std::getline(ifs, buff);
		}

		// ���̑���break
		else
		{
			ifs.seekg(old_pos);
			break;
		}
	}
}

void OBJ_Loader::LoadMTLFile(
	const std::filesystem::path& r_path,
	std::shared_ptr<OBJ::Desc> out_desc
)	noexcept(false)
{
	out_desc->mtl_path = out_desc->mtl_path.lexically_normal();
	// �g���q�̑��݊m�F
	CheckExtensiton(out_desc->mtl_path, ".MTL");

	if (out_desc->mtl_path.is_relative())
	{	// ���΃p�X
		out_desc->mtl_path = r_path / out_desc->mtl_path;
	}

	// �t�@�C���̑��΃p�X��ۑ�
	auto mtl_r_path = out_desc->mtl_path;
	mtl_r_path.remove_filename();

	std::ifstream ifs(out_desc->mtl_path);

	if (!ifs.is_open())
	{
		throw std::runtime_error("[ " + out_desc->mtl_path.string() + " ] �̓ǂݍ��݂Ɏ��s");
	}

	// �e�L�X�g���[�h�Ńt�@�C�����J�����Ƃ��ɔ������邸���
	// �o�b�t�@�����O�𖳌��ɂ��邱�Ƃŉ�������
	ifs.rdbuf()->pubsetbuf(nullptr, 0);

	std::string buff;
	buff.reserve(256);

	while (!ifs.eof())
	{
		buff.clear();

		ifs >> buff;

		// �R�����g�A�E�g���X�L�b�v
		if (buff[0] == '#')
		{
			std::getline(ifs, buff);
			continue;
		}

		// �啶���œ���
		buff = CONVERT::StrToUpper(buff);

		if (buff == "NEWMTL")
		{
			// �ŏ��̋󔒂��΂��āA��s�R�s�[
			if (SCAST<char>(ifs.get()) != ' ')
			{
				throw std::runtime_error(
					"[ " + out_desc->mtl_path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
					+ "[�t�H�[�}�b�g�G���[]"
				);
			}
			// ��s�R�s�[
			std::string mtl_name;
			std::getline(ifs, mtl_name);

			auto materials = out_desc->materials[mtl_name];
			// �܂�����Ă��Ȃ��ꍇ�쐬
			if (!materials)
			{
				materials = std::make_shared<OBJ::Material>();
				out_desc->materials[mtl_name] = materials;
			}

			LoadMTL(mtl_r_path, out_desc->mtl_path, ifs, materials);
		}

		// �󔒂����������EOF�ɂȂ�Ȃ��������
		else if(buff.empty())
		{
			if (ifs.get() == EOF)
				break;
		}
	}
}

// �I�u�W�F�N�g����ǂݍ���
void OBJ_Loader::LoadObjects(
	std::ifstream& ifs,
	std::shared_ptr<OBJ::Desc> out_desc,
	std::shared_ptr<OBJ::Object> out_objects
) noexcept
{
	std::string buff;
	buff.reserve(256);

	out_objects->positions.count = 0u;
	out_objects->uvs.count = 0u;
	out_objects->normals.count = 0u;

	auto& positions = out_desc->object_data.positions;
	auto& uvs = out_desc->object_data.uvs;
	auto& normals = out_desc->object_data.normals;

	positions.reserve(positions.size() + 1024u);
	uvs.reserve(uvs.size() + 1024u);
	normals.reserve(normals.size() + 1024u);

	while (!ifs.eof())
	{
		buff.clear();

		// �߂����߂̍��W��ۊ�
		std::ifstream::pos_type old_pos = ifs.tellg();

		ifs >> buff;

		// �R�����g�A�E�g���X�L�b�v
		if (buff[0] == '#')
		{
			std::getline(ifs, buff);
			continue;
		}

		// �啶���œ���
		buff = CONVERT::StrToUpper(buff);

		// ���W
		if (buff == "V")
		{
			auto& pos = positions.emplace_back();
			ifs >> pos.x; ifs >> pos.y; ifs >> pos.z;
			out_objects->positions.count++;
		}

		// �e�N�X�`��UV
		else if (buff == "VT")
		{
			auto& uv = uvs.emplace_back();
			ifs >> uv.x; ifs >> uv.y;
			out_objects->uvs.count++;
		}

		// �@��
		else if (buff == "VN")
		{
			auto& normal = normals.emplace_back();
			ifs >> normal.x; ifs >> normal.y; ifs >> normal.z;
			out_objects->normals.count++;
		}

		else
		{
			ifs.seekg(old_pos);
			break;
		}
	}
}

bool OBJ_Loader::LoadMaterials(
	std::ifstream& ifs,
	const std::filesystem::path& r_path,
	std::shared_ptr<OBJ::Desc> out_desc,
	std::shared_ptr<OBJ::Material> out_material,
	bool smooth_flg
) noexcept(false)
{
	const auto& n_path = GetPath();
	out_material->param.smooth = smooth_flg;

	std::string buff;
	buff.reserve(256);
	out_material->vertices.reserve(out_material->vertices.size() + 1024u);

	while (!ifs.eof())
	{
		buff.clear();

		// �߂����߂̍��W��ۊ�
		auto old_pos = ifs.tellg();

		ifs >> buff;

		// �R�����g�A�E�g���X�L�b�v
		if (buff[0] == '#')
		{
			std::getline(ifs, buff);
			continue;
		}

		// �啶���œ���
		buff = CONVERT::StrToUpper(buff);

		// Smooth flg
		if (buff == "S")
		{
			std::string smooth;
			ifs >> smooth;

			out_material->param.smooth = smooth == "1";
		}

		// ���_�f�[�^�ԍ�
		else if (buff == "F")
		{
			// 3���_��
			for (UINT i = 0; i < 3; i++)
			{
				OBJ::Vertex vertex;
				ifs >> vertex.position;
				if (SCAST<char>(ifs.get()) != '/')
				{
					throw std::runtime_error(
						"[ " + n_path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
						+ "[�t�H�[�}�b�g�G���[]"
					);
				}
				ifs >> vertex.uv;
				if (SCAST<char>(ifs.get()) != '/')
				{
					throw std::runtime_error(
						"[ " + n_path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
						+ "[�t�H�[�}�b�g�G���[]"
					);
				}
				ifs >> vertex.normal;

				out_material->vertices.push_back(vertex);
			}
		}
		
		// ����ȊO�������̂�Break
		else
		{
			ifs.seekg(old_pos);
			break;
		}
	}

	// �R���e�i�̃L���p�V�e�B��؂�l�߂�
	out_material->vertices.shrink_to_fit();

	return out_material->param.smooth;
}

void OBJ_Loader::ConvertMaterial()
{
	auto materials = std::make_shared<S_MODEL::Materials>();

	const auto& obj_positions = m_desc->object_data.positions;
	const auto& obj_uvs = m_desc->object_data.uvs;
	const auto& obj_normals = m_desc->object_data.normals;

	constexpr auto ConvertTexture = [](
		std::filesystem::path* out_tex_path,
		std::shared_ptr<const OBJ::Material::Texture> tex
		){
			if (tex)
			{
				*out_tex_path = tex->path;
			}
		};

	// �}�e���A���f�[�^��ϊ�
	for (const auto& obj_mt : m_desc->materials)
	{
		S_MODEL::Material mt;
		mt.param = obj_mt.second->param;

		// �e�N�X�`���f�[�^��ϊ�
		ConvertTexture(&mt.tex.ambient, obj_mt.second->tex_ambient);
		ConvertTexture(&mt.tex.diffuse, obj_mt.second->tex_diffuse);
		ConvertTexture(&mt.tex.specular, obj_mt.second->tex_specular);
		ConvertTexture(&mt.tex.specular_highlights, obj_mt.second->tex_specular_highlights);
		ConvertTexture(&mt.tex.dissolve, obj_mt.second->tex_dissolve);
		ConvertTexture(&mt.tex.bump, obj_mt.second->tex_bump);
		ConvertTexture(&mt.tex.displacement, obj_mt.second->tex_displacement);
		ConvertTexture(&mt.tex.stencil_decal, obj_mt.second->tex_stencil_decal);
		mt.tex.reflections = obj_mt.second->tex_reflections;

		const size_t v_size = obj_mt.second->vertices.size();
		mt.vertices.resize(v_size);

		// ���_�f�[�^��ϊ�
		for (size_t i = 0u; i < v_size; i++)
		{
			auto& mt_vertex = mt.vertices[i];
			const auto& obj_vertex = obj_mt.second->vertices[i];

			mt_vertex.position = obj_positions[obj_vertex.position - 1];
			mt_vertex.uv = obj_uvs[obj_vertex.uv - 1];
			mt_vertex.uv.y = 1.f - mt_vertex.uv.y;
			mt_vertex.normal = obj_normals[obj_vertex.normal - 1];
			using namespace DirectX;
			XMVECTOR normal = XMVector3Normalize(XMLoadFloat3(&mt_vertex.normal));
			XMStoreFloat3(&mt_vertex.normal, normal);

			// Tangent���v�Z
			XMVECTOR y_up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

			XMVECTOR tangent = XMVector3Normalize(XMVector3Cross(normal, y_up));
			float length;
			XMStoreFloat(&length, XMVector3Length(tangent));

			// �����x�N�g���������̂ŊO�ς�������
			if (length <= FLT_EPSILON)
			{
				XMVECTOR x_right = XMVectorSet(1.f, 0.f, 0.f, 0.f);
				tangent = XMVector3Normalize(XMVector3Cross(normal, x_right));
			}
			XMStoreFloat3(&mt_vertex.tangent, tangent);

			// bitangent �� tangent �� normal ����v�Z�\
			XMStoreFloat3(&mt_vertex.bitangent, XMVector3Normalize(XMVector3Cross(tangent, normal)));
		}

		materials->insert(std::make_pair(obj_mt.first, mt));
	}
	SetMaterials(materials);
}