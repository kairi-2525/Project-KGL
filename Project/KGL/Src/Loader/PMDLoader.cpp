#include <Loader/PMDLoader.hpp>
#include <Helper/ThrowAssert.hpp>
#include <Helper/Cast.hpp>
#include <fstream>

using namespace KGL;

PMD_Loader::PMD_Loader(std::filesystem::path path) noexcept
{
	m_desc.path = path;

	UINT vert_num;	// 総頂点数
	UINT idx_num;	// 総インデックス数
	UINT material_num;

	std::ifstream ifs(m_desc.path, std::ios::in | std::ios::binary);
	try
	{
		if (!ifs.is_open()) throw std::runtime_error("[ " + m_desc.path.string() + " ] が見つかりませんでした。");
		std::string str;
		str.resize(3);
		ifs.read(str.data(), str.size());

		if (str != "Pmd")
		{
			throw std::runtime_error("[" + m_desc.path.string() + "] のファイル形式が不正です。");
		}

		ifs.read((char*)&m_desc.header, sizeof(PMD::Header));

		ifs.read((char*)&vert_num, sizeof(vert_num));

		if (vert_num < 3)
		{
			throw std::runtime_error("[" + m_desc.path.string() + "] のファイル形式が不正です。");
		}
	}
	catch (std::runtime_error& exception)
	{
		RuntimeErrorStop(exception);
	}

	m_desc.vertices.resize(vert_num * PMD::VERTEX_SIZE);
	ifs.read((char*)m_desc.vertices.data(), m_desc.vertices.size());

	ifs.read((char*)&idx_num, sizeof(idx_num));

	m_desc.indices.resize(idx_num);
	ifs.read((char*)m_desc.indices.data(), SCAST<ULONG>(m_desc.indices.size()) * sizeof(m_desc.indices[0]));

	ifs.read((char*)&material_num, sizeof(material_num));
	m_desc.materials.resize(material_num);

	ifs.read((char*)m_desc.materials.data(), SCAST<ULONG>(material_num * sizeof(PMD::Material)));

	USHORT bone_num = 0;
	ifs.read((char*)&bone_num, sizeof(bone_num));

	if (bone_num > 0)
	{
		m_desc.bones.resize(bone_num);
		ifs.read((char*)m_desc.bones.data(), SCAST<ULONG>(bone_num * sizeof(PMD::Bone)));
		
		// ボーンノードマップを作成
		std::vector<std::string> bone_names(bone_num);
		std::shared_ptr<PMD::BoneTable> bone_table = std::make_unique<PMD::BoneTable>();
		m_desc.bone_node_table = bone_table;
		for (size_t i = 0u; i < bone_num; i++)
		{
			const auto& it = m_desc.bones[i];
			auto& name = bone_names[i];
			name = it.bone_name;

			auto& node = bone_table->operator[](name);
			node.bone_idx = i;
			node.start_pos = it.pos;
		}

		// 親子関係を構築
		for (auto& it : m_desc.bones)
		{
			if (it.parent_no >= bone_num)
				continue;

			auto parent_name = bone_names[it.parent_no];
			bone_table->at(parent_name).children.emplace_back(
				&bone_table->at(it.bone_name)
			);
		}
	}
}