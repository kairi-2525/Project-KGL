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

	UINT vert_num;	// 総頂点数
	UINT idx_num;	// 総インデックス数
	UINT material_num;

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

	desc->vertices.resize(vert_num * PMD::VERTEX_SIZE);
	ifs.read((char*)desc->vertices.data(), desc->vertices.size());

	ifs.read((char*)&idx_num, sizeof(idx_num));

	desc->indices.resize(idx_num);
	ifs.read((char*)desc->indices.data(), SCAST<ULONG>(desc->indices.size()) * sizeof(desc->indices[0]));

	ifs.read((char*)&material_num, sizeof(material_num));
	desc->materials.resize(material_num);

	ifs.read((char*)desc->materials.data(), SCAST<ULONG>(material_num * sizeof(PMD::Material)));

	USHORT bone_num = 0;
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
			node.bone_idx = i;
			node.start_pos = it.pos;

			desc->bone_name_array[i] = name;
			desc->bone_node_address_array[i] = &node;
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

#ifdef _CONSOLE
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