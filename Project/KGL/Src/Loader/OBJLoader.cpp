#include <Loader/OBJLoader.hpp>
#include <Helper/Convert.hpp>
#include <Helper/Cast.hpp>

using namespace KGL;

OBJ_Loader::OBJ_Loader(const std::filesystem::path& path) noexcept :
	Loader(path)
{
	std::ifstream ifs(this->path);

	auto desc = std::make_shared<OBJ::Desc>();

	bool smooth = false;

	try
	{
		if (!ifs.is_open())
		{
			throw std::runtime_error(
				"[ " + this->path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
				+ "[�t�@�C�����J���܂���]"
			);
		}

		// �g���q�̑��݊m�F
		CheckExtensiton(".OBJ");

		// �e�L�X�g���[�h�Ńt�@�C�����J�����Ƃ��ɔ������邸���
		// �o�b�t�@�����O�𖳌��ɂ��邱�Ƃŉ�������
		ifs.rdbuf()->pubsetbuf(nullptr, 0);

		// �t�@�C���̑��΃p�X��ۑ�
		auto r_path = this->path;
		r_path.remove_filename();

		std::string buff;
		buff.reserve(256);

		// �t�@�C���̖��[�܂Ń��[�v
		while (!ifs.eof())
		{
			buff.clear();

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
			else if (buff == "O")
			{
				// �ŏ��̋󔒂��΂��āA��s�R�s�[
				if (SCAST<char>(ifs.get()) != ' ')
				{
					throw std::runtime_error(
						"[ " + this->path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
						+ "[�t�H�[�}�b�g�G���[]"
					);
				}

				std::string obj_name;

				// �I�u�W�F�N�g�̖��O���擾
				std::getline(ifs, obj_name);

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
						"[ " + this->path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
						+ "[�t�H�[�}�b�g�G���[]"
					);
				}
				// ��s�R�s�[
				std::string tex_path;
				std::getline(ifs, tex_path);

				if (CONVERT::StrToUpper(tex_path) != "NONE")
				{
					std::filesystem::path tpath = tex_path;

					// ���΃p�X�̏ꍇ�p�X����������
					if (tpath.is_relative())
					{
						tex_path = r_path.string() + tex_path;
					}
				}
				
				// NONE�̏ꍇ�@�������O�����邽�ߊm�F����
				if (desc->materials.count(tex_path) == 0u)
				{
					desc->materials[tex_path] = std::make_shared<OBJ::Material>();
				}

				// �}�e���A���f�[�^��ǂݍ���
				smooth = LoadMaterials(ifs, r_path, desc, desc->materials[tex_path], smooth);
			}
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}

	// �R���e�i�̃L���p�V�e�B��؂�l�߂�
	desc->object_data.positions.shrink_to_fit();
	desc->object_data.uvs.shrink_to_fit();
	desc->object_data.normals.shrink_to_fit();

	// const �̃����o�ɏ��n
	m_desc = desc;
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
		out_desc->mtl_path = r_path.string() + out_desc->mtl_path.string();
	}

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

		// �R�����g�A�E�g���X�L�b�v
		if (buff[0] == '#')
		{
			std::getline(ifs, buff);
			continue;
		}

		// �啶���œ���
		buff = CONVERT::StrToUpper(buff);

		ifs >> buff;
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
	out_material->smooth = smooth_flg;

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

			out_material->smooth = smooth == "1";
		}

		// ���_�f�[�^�ԍ�
		if (buff == "F")
		{
			// 3���_��
			for (UINT i = 0; i < 3; i++)
			{
				OBJ::Vertex vertex;
				ifs >> vertex.position;
				if (SCAST<char>(ifs.get()) != '/')
				{
					throw std::runtime_error(
						"[ " + this->path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
						+ "[�t�H�[�}�b�g�G���[]"
					);
				}
				ifs >> vertex.uv;
				if (SCAST<char>(ifs.get()) != '/')
				{
					throw std::runtime_error(
						"[ " + this->path.string() + " ] �̓ǂݍ��݂Ɏ��s\n"
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

	return out_material->smooth;
}