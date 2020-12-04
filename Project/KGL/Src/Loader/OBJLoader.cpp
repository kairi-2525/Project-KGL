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
				"[ " + n_path.string() + " ] の読み込みに失敗\n"
				+ "[ファイルが開けません]"
			);
		}

		// 拡張子の存在確認
		CheckExtensiton(".OBJ");

		// テキストモードでファイルを開いたときに発生するずれを
		// バッファリングを無効にすることで解決する
		ifs.rdbuf()->pubsetbuf(nullptr, 0);

		// ファイルの相対パスを保存
		auto r_path = n_path;
		r_path.remove_filename();

		std::string buff;
		buff.reserve(256);

		// ファイルの末端までループ
		while (!ifs.eof())
		{
			buff.clear();

			auto old_pos = ifs.tellg();

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
			else if (buff == "O" || buff == "V" || buff == "VT" || buff == "VN")
			{
				std::string obj_name;
				if (buff == "O")
				{
					// 最初の空白を飛ばして、一行コピー
					if (SCAST<char>(ifs.get()) != ' ')
					{
						throw std::runtime_error(
							"[ " + n_path.string() + " ] の読み込みに失敗\n"
							+ "[フォーマットエラー]"
						);
					}
					// オブジェクトの名前を取得
					std::getline(ifs, obj_name);
				}
				else
				{
					// それ以外が来た場合は””に読み込む
					ifs.seekg(old_pos);
				}

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
						"[ " + n_path.string() + " ] の読み込みに失敗\n"
						+ "[フォーマットエラー]"
					);
				}
				// 一行コピー
				std::string tex_path;
				std::getline(ifs, tex_path);

				auto materials = desc->materials[tex_path];
				
				// まだ作られていない場合作成
				if (!materials)
				{
					materials = std::make_shared<OBJ::Material>();
					desc->materials[tex_path] = materials;
				}

				// マテリアルデータを読み込む
				smooth = LoadMaterials(ifs, r_path, desc, materials, smooth);
			}
		}
		// コンテナのキャパシティを切り詰める
		desc->object_data.positions.shrink_to_fit();
		desc->object_data.uvs.shrink_to_fit();
		desc->object_data.normals.shrink_to_fit();

		// const のメンバに譲渡
		m_desc = desc;

		// StaticModel用のマテリアルに変換
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
	// テクスチャパラメータ初期化
	InitTexture(out_texture);

	std::string buff;
	buff.reserve(256);

	while (!ifs.eof())
	{
		auto old_pos = ifs.tellg();

		// 最初の空白を飛ばす
		if (SCAST<char>(ifs.get()) != ' ')
		{
			return false;
		}

		// "-"でなければテクスチャパス
		if (SCAST<char>(ifs.get()) != '-')
		{
			ifs.seekg(old_pos);

			// 最初の空白を飛ばして、一行コピー
			if (SCAST<char>(ifs.get()) != ' ')
			{
				return false;
			}
			// 一行コピー
			std::string tex_path;
			std::getline(ifs, tex_path);
			out_texture->path = tex_path;

			// パスの正規化
			out_texture->path = out_texture->path.lexically_normal();
			// 相対パスの場合
			if (out_texture->path.is_relative())
			{
				out_texture->path = r_path / out_texture->path;
			}

			return true;
		}

		buff.clear();

		// 大文字で統一
		buff = CONVERT::StrToUpper(buff);

		// 水平テクスチャ混合を設定
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
		// 垂直テクスチャ混合を設定 
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

		// mip-mapのシャープさを押し上げ
		else if (buff == "-BOOST")
		{
			ifs >> out_texture->mipmap_boost;
		}
		// テクスチャマップの値を変更 (標準は 0 1)
		else if (buff == "-MM")
		{
			ifs >> out_texture->texture_map.gain;
			ifs >> out_texture->texture_map.contrast;
		}

		// 原点オフセット
		else if (buff == "-O")
		{
			ifs >> out_texture->offset_pos.x;
			ifs >> out_texture->offset_pos.y;
			ifs >> out_texture->offset_pos.z;
		}
		// スケール
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
		// 作成するテクスチャ解像度
		else if (buff == "-TEXRES")
		{
			ifs >> out_texture->resolution.x;
			ifs >> out_texture->resolution.y;
		}
		// 0から1の範囲でクランプされたテクセルのみレンダリング (標準は off)
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
		// バンプ乗数
		else if (buff == "-BM")
		{
			ifs >> out_texture->bump_mult;
		}
		// スカラーまたはバンプテクスチャを作成するために
		else if (buff == "-IMFCHAN")
		{
			ifs >> out_texture->bump_file_ch;
		}

		// 不正値
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

	// 読み込まれなかったパラメーターが
	// 不定値にならないように初期化する
	InitMTL(out_material);

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

		// 透明度
		else if (buff == "D")
		{
			// 0 ~ 1
			ifs >> out_material->param.dissolve;
		}
		// Dの反転
		else if (buff == "TR")
		{
			// 0 ~ 1
			ifs >> out_material->param.dissolve;
			out_material->param.dissolve = 1.f - out_material->param.dissolve;
		}

		// 光学密度(屈折率)
		else if (buff == "NI")
		{
			// 0.001 ~ 10
			// 1.0の値は光がオブジェクトをパススルーし曲がらないことを意味する。
			ifs >> out_material->param.refraction;
		}
		// 照明モデル
		else if (buff == "ILLUM")
		{
			// 1で鏡面反射無効, 2で有効
			int flg_value;
			ifs >> flg_value;
			out_material->param.specular_flg = flg_value == 2;
		}

		// アンビエントテクスチャマップ
		else if (buff == "MAP_KA")
		{
			out_material->tex_ambient = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_ambient))
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
		}
		// ディフューズテクスチャマップ (多くの場合、アンビエントテクスチャマップと同じにされる)
		else if (buff == "MAP_KD")
		{
			out_material->tex_diffuse = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_diffuse))
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
		}
		// スペキュラカラーテクスチャマップ
		else if (buff == "MAP_KS")
		{
			out_material->tex_specular = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_specular))
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
		}
		// スペキュラハイライト成分
		else if (buff == "MAP_NS")
		{
			out_material->tex_specular_highlights = std::make_shared<OBJ::Material::Texture>();
			if(!LoadTexture(r_path, ifs, out_material->tex_specular_highlights))
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
		}
		// 透過度テクスチャマップ
		else if (buff == "MAP_D")
		{
			out_material->tex_dissolve = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_dissolve))
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
		}
		// バンプマップ(標準で画像の輝度値チャンネルを使用)
		else if (buff == "MAP_BUMP" || buff == "BUMP")
		{
			out_material->tex_bump = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_bump))
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
		}
		// ディスプレースメントマップ
		else if (buff == "MAP_DISP" || buff == "DISP")
		{
			out_material->tex_displacement = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_displacement))
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
		}
		// ステンシルデカールテクスチャ (標準で画像の'matte'チャンネルを使用)
		else if (buff == "MAP_DECAL" || buff == "DECAL")
		{
			out_material->tex_stencil_decal = std::make_shared<OBJ::Material::Texture>();
			if (!LoadTexture(r_path, ifs, out_material->tex_stencil_decal))
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
		}

		// 反射マップ
		else if (buff == "REFL")
		{
			std::string type_str;
			ifs >> type_str;
			type_str = CONVERT::StrToUpper(type_str);

			if (type_str != "-TYPE")
			{
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
			}
			type_str.clear();
			ifs >> type_str;
			type_str = CONVERT::StrToUpper(type_str);

			// 最初の空白を飛ばして、一行コピー
			if (SCAST<char>(ifs.get()) != ' ')
				throw std::runtime_error("[ " + path.string() + " ] の読み込みに失敗\n[フォーマットエラー]");
			// 一行コピー
			std::string reflection_str;
			std::getline(ifs, reflection_str);
			out_material->tex_reflections[type_str] = reflection_str;
		}

		// 物理ベースレンダリング用パラメーター
		else if (
			buff == "MAP_PR" || buff == "PR" ||	// ラフネス
			buff == "MAP_PM" || buff == "PM" ||	// メタリック
			buff == "MAP_PS" || buff == "PS" ||	// Sheen
			buff == "PC" ||						// クリアコートの厚さ
			buff == "PCR" ||					// クリアコートのラフネス
			buff == "MAP_KE" || buff == "KE" ||	// 放射
			buff == "ANISO" ||					// 異方性
			buff == "ANISOR" ||					// 異方性の回転
			buff == "NORM"						// 法線マップ、"bump" パラメーターと同じ形式
		){
			// 一行飛ばす
			std::getline(ifs, buff);
		}

		// その他はbreak
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
	// 拡張子の存在確認
	CheckExtensiton(out_desc->mtl_path, ".MTL");

	if (out_desc->mtl_path.is_relative())
	{	// 相対パス
		out_desc->mtl_path = r_path / out_desc->mtl_path;
	}

	// ファイルの相対パスを保存
	auto mtl_r_path = out_desc->mtl_path;
	mtl_r_path.remove_filename();

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

		ifs >> buff;

		// コメントアウトをスキップ
		if (buff[0] == '#')
		{
			std::getline(ifs, buff);
			continue;
		}

		// 大文字で統一
		buff = CONVERT::StrToUpper(buff);

		if (buff == "NEWMTL")
		{
			// 最初の空白を飛ばして、一行コピー
			if (SCAST<char>(ifs.get()) != ' ')
			{
				throw std::runtime_error(
					"[ " + out_desc->mtl_path.string() + " ] の読み込みに失敗\n"
					+ "[フォーマットエラー]"
				);
			}
			// 一行コピー
			std::string mtl_name;
			std::getline(ifs, mtl_name);

			auto materials = out_desc->materials[mtl_name];
			// まだ作られていない場合作成
			if (!materials)
			{
				materials = std::make_shared<OBJ::Material>();
				out_desc->materials[mtl_name] = materials;
			}

			LoadMTL(mtl_r_path, out_desc->mtl_path, ifs, materials);
		}

		// 空白が無視されてEOFにならない問題を回避
		else if(buff.empty())
		{
			if (ifs.get() == EOF)
				break;
		}
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
	const auto& n_path = GetPath();
	out_material->param.smooth = smooth_flg;

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

			out_material->param.smooth = smooth == "1";
		}

		// 頂点データ番号
		else if (buff == "F")
		{
			// 3頂点分
			for (UINT i = 0; i < 3; i++)
			{
				OBJ::Vertex vertex;
				ifs >> vertex.position;
				if (SCAST<char>(ifs.get()) != '/')
				{
					throw std::runtime_error(
						"[ " + n_path.string() + " ] の読み込みに失敗\n"
						+ "[フォーマットエラー]"
					);
				}
				ifs >> vertex.uv;
				if (SCAST<char>(ifs.get()) != '/')
				{
					throw std::runtime_error(
						"[ " + n_path.string() + " ] の読み込みに失敗\n"
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

	// マテリアルデータを変換
	for (const auto& obj_mt : m_desc->materials)
	{
		S_MODEL::Material mt;
		mt.param = obj_mt.second->param;

		// テクスチャデータを変換
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

		// 頂点データを変換
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

			// Tangentを計算
			XMVECTOR y_up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

			XMVECTOR tangent = XMVector3Normalize(XMVector3Cross(normal, y_up));
			float length;
			XMStoreFloat(&length, XMVector3Length(tangent));

			// 同じベクトルだったので外積をし直す
			if (length <= FLT_EPSILON)
			{
				XMVECTOR x_right = XMVectorSet(1.f, 0.f, 0.f, 0.f);
				tangent = XMVector3Normalize(XMVector3Cross(normal, x_right));
			}
			XMStoreFloat3(&mt_vertex.tangent, tangent);

			// bitangent も tangent と normal から計算可能
			XMStoreFloat3(&mt_vertex.bitangent, XMVector3Normalize(XMVector3Cross(tangent, normal)));
		}

		materials->insert(std::make_pair(obj_mt.first, mt));
	}
	SetMaterials(materials);
}