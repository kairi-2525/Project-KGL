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
				"[ " + this->path.string() + " ] の読み込みに失敗\n"
				+ "[ファイルが開けません]"
			);
		}

		// 拡張子の存在確認
		CheckExtensiton(".OBJ");

		// テキストモードでファイルを開いたときに発生するずれを
		// バッファリングを無効にすることで解決する
		ifs.rdbuf()->pubsetbuf(nullptr, 0);

		// ファイルの相対パスを保存
		auto r_path = this->path;
		r_path.remove_filename();

		std::string buff;
		buff.reserve(256);

		// ファイルの末端までループ
		while (!ifs.eof())
		{
			buff.clear();

			ifs >> buff;

			// 空白が無視されてEOFにならない問題を回避
			if (buff.empty())
			{
				if (ifs.get() == EOF)
					break;
			}

			// コメントアウトをスキップ
			if (buff[0] == '#')
			{
				std::getline(ifs, buff);
				continue;
			}

			// 大文字で統一
			buff = CONVERT::StrToUpper(buff);

			// MTLファイルを読み込む
			if (buff == "MTLLIB")
			{
				// ファイルパスを読み込み、正規化
				ifs >> desc->mtl_path;
				LoadMTLFile(r_path, desc);
			}

			// オブジェクトデータ
			else if (buff == "O")
			{
				// 最初の空白を飛ばして、一行コピー
				if (SCAST<char>(ifs.get()) != ' ')
				{
					throw std::runtime_error(
						"[ " + this->path.string() + " ] の読み込みに失敗\n"
						+ "[フォーマットエラー]"
					);
				}

				std::string obj_name;

				// オブジェクトの名前を取得
				std::getline(ifs, obj_name);

				// オブジェクトを読み込む
				auto objects = desc->objects[obj_name] = std::make_shared<OBJ::Object>();
				objects->positions.begin = desc->object_data.positions.size();
				objects->uvs.begin = desc->object_data.uvs.size();
				objects->normals.begin = desc->object_data.normals.size();

				LoadObjects(ifs, desc, desc->objects[obj_name]);
			}

			// マテリアルデータ
			else if (buff == "USEMTL")
			{
				// 最初の空白を飛ばして、一行コピー
				if (SCAST<char>(ifs.get()) != ' ')
				{
					throw std::runtime_error(
						"[ " + this->path.string() + " ] の読み込みに失敗\n"
						+ "[フォーマットエラー]"
					);
				}
				// 一行コピー
				std::string tex_path;
				std::getline(ifs, tex_path);

				if (CONVERT::StrToUpper(tex_path) != "NONE")
				{
					std::filesystem::path tpath = tex_path;

					// 相対パスの場合パスを結合する
					if (tpath.is_relative())
					{
						tex_path = r_path.string() + tex_path;
					}
				}
				
				// NONEの場合　同じ名前が来るため確認する
				if (desc->materials.count(tex_path) == 0u)
				{
					desc->materials[tex_path] = std::make_shared<OBJ::Material>();
				}

				// マテリアルデータを読み込む
				smooth = LoadMaterials(ifs, r_path, desc, desc->materials[tex_path], smooth);
			}
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}

	// コンテナのキャパシティを切り詰める
	desc->object_data.positions.shrink_to_fit();
	desc->object_data.uvs.shrink_to_fit();
	desc->object_data.normals.shrink_to_fit();

	// const のメンバに譲渡
	m_desc = desc;
}

void OBJ_Loader::LoadMTLFile(
	const std::filesystem::path& r_path,
	std::shared_ptr<OBJ::Desc> out_desc
)	noexcept(false)
{
	out_desc->mtl_path = out_desc->mtl_path.lexically_normal();
	// 拡張子の存在確認
	CheckExtensiton(out_desc->mtl_path, ".MTL");

	if (out_desc->mtl_path.is_relative())
	{	// 相対パス
		out_desc->mtl_path = r_path.string() + out_desc->mtl_path.string();
	}

	std::ifstream ifs(out_desc->mtl_path);

	if (!ifs.is_open())
	{
		throw std::runtime_error("[ " + out_desc->mtl_path.string() + " ] の読み込みに失敗");
	}

	// テキストモードでファイルを開いたときに発生するずれを
		// バッファリングを無効にすることで解決する
	ifs.rdbuf()->pubsetbuf(nullptr, 0);

	std::string buff;
	buff.reserve(256);

	while (!ifs.eof())
	{
		buff.clear();

		// コメントアウトをスキップ
		if (buff[0] == '#')
		{
			std::getline(ifs, buff);
			continue;
		}

		// 大文字で統一
		buff = CONVERT::StrToUpper(buff);

		ifs >> buff;
	}
}

// オブジェクト情報を読み込む
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

		// 戻すための座標を保管
		std::ifstream::pos_type old_pos = ifs.tellg();

		ifs >> buff;

		// コメントアウトをスキップ
		if (buff[0] == '#')
		{
			std::getline(ifs, buff);
			continue;
		}

		// 大文字で統一
		buff = CONVERT::StrToUpper(buff);

		// 座標
		if (buff == "V")
		{
			auto& pos = positions.emplace_back();
			ifs >> pos.x; ifs >> pos.y; ifs >> pos.z;
			out_objects->positions.count++;
		}

		// テクスチャUV
		else if (buff == "VT")
		{
			auto& uv = uvs.emplace_back();
			ifs >> uv.x; ifs >> uv.y;
			out_objects->uvs.count++;
		}

		// 法線
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

		// 戻すための座標を保管
		auto old_pos = ifs.tellg();

		ifs >> buff;

		// コメントアウトをスキップ
		if (buff[0] == '#')
		{
			std::getline(ifs, buff);
			continue;
		}

		// 大文字で統一
		buff = CONVERT::StrToUpper(buff);

		// Smooth flg
		if (buff == "S")
		{
			std::string smooth;
			ifs >> smooth;

			out_material->smooth = smooth == "1";
		}

		// 頂点データ番号
		if (buff == "F")
		{
			// 3頂点分
			for (UINT i = 0; i < 3; i++)
			{
				OBJ::Vertex vertex;
				ifs >> vertex.position;
				if (SCAST<char>(ifs.get()) != '/')
				{
					throw std::runtime_error(
						"[ " + this->path.string() + " ] の読み込みに失敗\n"
						+ "[フォーマットエラー]"
					);
				}
				ifs >> vertex.uv;
				if (SCAST<char>(ifs.get()) != '/')
				{
					throw std::runtime_error(
						"[ " + this->path.string() + " ] の読み込みに失敗\n"
						+ "[フォーマットエラー]"
					);
				}
				ifs >> vertex.normal;

				out_material->vertices.push_back(vertex);
			}
		}
		
		// それ以外が来たのでBreak
		else
		{
			ifs.seekg(old_pos);
			break;
		}
	}

	// コンテナのキャパシティを切り詰める
	out_material->vertices.shrink_to_fit();

	return out_material->smooth;
}