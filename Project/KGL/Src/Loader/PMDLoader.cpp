#include <Loader/PMDLoader.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Cast.hpp>
#include <fstream>
#include <sstream>
#include <Helper/Debug.hpp>

using namespace KGL;

PMD_Loader::PMD_Loader(std::filesystem::path path) noexcept
{
	std::shared_ptr<PMD::Desc> desc = std::make_shared<PMD::Desc>();
	m_desc = desc;
	desc->path = path;

	UINT32 vert_num;	// 総頂点数
	UINT32 idx_num;	// 総インデックス数
	UINT32 material_num;

	std::ifstream ifs(desc->path, std::ios::in | std::ios::binary);
	try
	{
		if (!ifs.is_open()) throw std::runtime_error("[ " + desc->path.string() + " ] が見つかりませんでした。");
		std::string str;
		str.resize(3);
		ifs.read(str.data(), str.size());

		if (str != "Pmd")
		{
			throw std::runtime_error("[" + desc->path.string() + "] のファイル形式が不正です。");
		}

		ifs.read((char*)&desc->header, sizeof(PMD::Header));

		ifs.read((char*)&vert_num, sizeof(vert_num));

		if (vert_num < 3)
		{
			throw std::runtime_error("[" + desc->path.string() + "] のファイル形式が不正です。");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}

	desc->vertices.resize(vert_num);
	ifs.read((char*)desc->vertices.data(), vert_num * PMD::VERTEX_SIZE);

	//std::vector<PMD::Vertex> vertices(vert_num);
	//ifs.read((char*)vertices.data(), vert_num * PMD::VERTEX_SIZE);
	//std::memcpy(vertices.data(), desc->vertices.data(), PMD::VERTEX_SIZE * vert_num);
	//for (auto& vert : vertices)
	//{
	//	vert.normal = { -vert.normal.x, -vert.normal.y, -vert.normal.z };
	//}
	//std::memcpy(desc->vertices.data(), vertices.data(), PMD::VERTEX_SIZE * vert_num);

	ifs.read((char*)&idx_num, sizeof(idx_num));

	desc->indices.resize(idx_num);
	ifs.read((char*)desc->indices.data(), SCAST<ULONG>(desc->indices.size()) * sizeof(desc->indices[0]));

	ifs.read((char*)&material_num, sizeof(material_num));
	desc->materials.resize(material_num);

	ifs.read((char*)desc->materials.data(), SCAST<ULONG>(material_num * sizeof(PMD::Material)));

	UINT16 bone_num = 0;
	ifs.read((char*)&bone_num, sizeof(bone_num));

	if (bone_num > 0)
	{
		desc->bones.resize(bone_num);
		ifs.read((char*)desc->bones.data(), SCAST<ULONG>(bone_num * sizeof(PMD::Bone)));
		
		// ボーンノードマップを作成
		std::vector<std::string> bone_names(bone_num);
		desc->bone_name_array.resize(bone_num);
		desc->bone_node_address_array.resize(bone_num);

		for (size_t i = 0u; i < bone_num; i++)
		{
			const auto& it = desc->bones[i];
			auto& name = bone_names[i];
			name = it.bone_name;

			auto& node = desc->bone_node_table[name];
			node.bone_idx = SCAST<UINT32>(i);
			node.start_pos = it.pos;

			desc->bone_name_array[i] = name;
			desc->bone_node_address_array[i] = &node;

			if (std::string(it.bone_name).find("ひざ") != std::string::npos)
			{
				desc->knee_idxes.emplace_back(SCAST<UINT32>(i));
			}
		}

		// 親子関係を構築
		for (auto& it : desc->bones)
		{
			if (it.parent_no >= bone_num)
				continue;

			auto parent_name = bone_names[it.parent_no];
			desc->bone_node_table.at(parent_name).children.emplace_back(
				&desc->bone_node_table.at(it.bone_name)
			);
		}
	}

	UINT16 ik_num = 0;
	ifs.read((char*)&ik_num, sizeof(ik_num));
	if (ik_num > 0)
	{
		desc->ik_data.resize(ik_num);
		for (auto& ik : desc->ik_data)
		{
			ifs.read((char*)&ik.bone_idx, sizeof(ik.bone_idx));
			ifs.read((char*)&ik.target_idx, sizeof(ik.target_idx));
			UINT8 chain_len = 0u;	// 間のノード数
			ifs.read((char*)&chain_len, sizeof(chain_len));
			ik.node_idxes.resize(chain_len);
			ifs.read((char*)&ik.iterations, sizeof(ik.iterations));
			ifs.read((char*)&ik.limit, sizeof(ik.limit));
			if (chain_len == 0)
				continue;
			ifs.read((char*)ik.node_idxes.data(), chain_len * sizeof(ik.node_idxes[0]));

		}
	}

	UINT16 morph_num = 0;
	ifs.read((char*)&morph_num, sizeof(morph_num));
	desc->morphs.resize(morph_num);
	for (UINT16 i = 0u; i < morph_num; i++)
	{
		PMD::Morph morph;
		ifs.read((char*)&morph, sizeof(PMD::Morph));
		desc->morphs[i].name = morph.name;
		desc->morphs[i].type = morph.type;
		desc->morphs[i].vertices.resize(morph.vertex_count);

		ifs.read((char*)desc->morphs[i].vertices.data(), sizeof(PMD::MorphVertex) * morph.vertex_count);
	}

	UINT8 morph_label_num = 0;
	ifs.read((char*)&morph_label_num, sizeof(morph_label_num));
	desc->morph_label_indices.resize(morph_label_num);
	ifs.read((char*)desc->morph_label_indices.data(), sizeof(UINT16) * morph_label_num);

	UINT8 bone_label_num = 0;
	ifs.read((char*)&bone_label_num, sizeof(bone_label_num));
	std::vector<char[50]> bone_labels(bone_label_num);
	desc->bone_labels.resize(bone_label_num);
	ifs.read((char*)bone_labels.data(), sizeof(char[50]) * bone_label_num);
	std::copy(bone_labels.begin(), bone_labels.end(), desc->bone_labels.begin());

	UINT32 bone_label_index_num = 0;
	ifs.read((char*)&bone_label_index_num, sizeof(bone_label_index_num));
	desc->bone_label_indices.resize(bone_label_index_num);
	ifs.read((char*)desc->bone_label_indices.data(), sizeof(PMD::BoneLabelIndex) * bone_label_index_num);

	if (!ifs.eof())
	{
		ifs.read((char*)&desc->localize_header, sizeof(PMD::LocalizeHeader));

		if (desc->localize_header.flag == 0x1)
		{
			{
				std::vector<char[20]> en_bone_names(bone_num);
				desc->en.bone_names.resize(bone_num);
				ifs.read((char*)en_bone_names.data(), sizeof(char[20]) * bone_num);
				std::copy(en_bone_names.begin(), en_bone_names.end(), desc->en.bone_names.begin());
			}
			{
				std::vector<char[20]> en_morph_names(morph_num - 1);
				desc->en.morph_names.resize(morph_num - 1);
				ifs.read((char*)en_morph_names.data(), sizeof(char[20])* (morph_num - 1));
				std::copy(en_morph_names.begin(), en_morph_names.end(), desc->en.morph_names.begin());
			}
			{
				std::vector<char[50]> en_bone_labels(bone_label_num);
				desc->en.bone_labels.resize(bone_label_num);
				ifs.read((char*)en_bone_labels.data(), sizeof(char[50]) * bone_label_num);
				std::copy(en_bone_labels.begin(), en_bone_labels.end(), desc->en.bone_labels.begin());
			}
		}
	}
	if (!ifs.eof())
	{
		PMD::ToonTextureList toon_textures;
		ifs.read((char*)&toon_textures, sizeof(PMD::ToonTextureList));
		for (UCHAR i = 0u; i < 10u; i++)
		{
			desc->toon_tex_table[i] = toon_textures.file_name[i];
		}
	}
#ifdef _CONSOLEa
	auto GetNameFromIdx = [&](UINT16 idx)->std::string
	{
		auto itr = std::find_if(
			m_desc->bone_node_table.cbegin(),
			m_desc->bone_node_table.cend(),
			[idx](const std::pair<std::string, PMD::BoneNode>& obj)
			{
				return obj.second.bone_idx == idx;
			}
		);
		if (itr != m_desc->bone_node_table.cend())
		{
			return itr->first;
		}
		else
		{
			return "";
		}
	};
	for (auto& ik : desc->ik_data)
	{
		std::ostringstream oss;
		oss << "IKボーン番号 : " << ik.bone_idx << " ( " << GetNameFromIdx(ik.bone_idx) << " )" << std::endl;
		for (auto& node : ik.node_idxes)
		{
			oss << "\tノードボーン : " << node << " ( " << GetNameFromIdx(node) << " )" << std::endl;
		}
		KGLDebugOutPutString(oss.str());
	}
#endif
}