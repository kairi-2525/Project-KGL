#include <Loader/PMDLoader.hpp>
#include <Helper/ThrowAssert.hpp>
#include <fstream>

using namespace KGL;

PMDLoader::PMDLoader(std::filesystem::path path) noexcept
{
	m_desc.path = path;

	UINT vert_num;	// �����_��
	UINT idx_num;	// ���C���f�b�N�X��
	UINT material_num;

	std::ifstream ifs(m_desc.path, std::ios::in | std::ios::binary);
	try
	{
		if (!ifs.is_open()) throw std::runtime_error("[ " + m_desc.path.string() + " ] ��������܂���ł����B");
		std::string str;
		str.resize(3);
		ifs.read(str.data(), str.size());

		if (str != "Pmd")
		{
			throw std::runtime_error("[" + m_desc.path.string() + "] �̃t�@�C���`�����s���ł��B");
		}

		ifs.read((char*)&m_desc.header, sizeof(PMD::Header));

		ifs.read((char*)&vert_num, sizeof(vert_num));

		if (vert_num < 3)
		{
			throw std::runtime_error("[" + m_desc.path.string() + "] �̃t�@�C���`�����s���ł��B");
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
	ifs.read((char*)m_desc.indices.data(), static_cast<ULONG>(m_desc.indices.size()) * sizeof(m_desc.indices[0]));

	ifs.read((char*)&material_num, sizeof(material_num));
	m_desc.materials.resize(material_num);
	ifs.read((char*)m_desc.materials.data(), m_desc.materials.size() * sizeof(PMD::Material));
}