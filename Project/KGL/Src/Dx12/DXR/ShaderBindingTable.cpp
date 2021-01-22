#include <Dx12/DXR/ShaderBindingTable.hpp>
#include <Helper/Convert.hpp>
#include <Helper/Cast.hpp>
#include <Helper/ThrowAssert.hpp>

using namespace KGL;

void DXR::ShaderBindingTable::ConvertDesc(const Desc& desc) noexcept
{
	auto ConvertTable = [](
		std::vector<PrivateEntryData>* dst,
		const std::vector<EntryData>& src
		)->UINT64
	{
		const size_t src_size = src.size();
		dst->resize(src_size);

		// 1�̃G���g���Ŏg�p�����p�����[�^�̍ő吔
		size_t max_args = 0;

		for (size_t i = 0u; i < src_size; i++)
		{
			auto& dst_it = (*dst)[i];
			const auto& src_it = src[i];

			// �v�f���R�s�[
			dst_it.entry_point = CONVERT::MultiToWide(src_it.entry_point);
			dst_it.input_data = src_it.input_data;

			// �p�����[�^�̍ő吔���L�^
			max_args = (std::max)(max_args, src_it.input_data.size());
		}

		// �v���O�������ʎq�̃T�C�Y;
		const UINT64 id_size = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
		const UINT64 entry_size = id_size + sizeof(UINT64) * SCAST<UINT64>(max_args);

		// id_size �A���C�����g
		return CONVERT::RoundUp(entry_size, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	};

	// �etable���R���o�[�g�Ɠ����ɃG���g���[�T�C�Y���L�^
	m_desc.raygen_entry_size = ConvertTable(&m_desc.raygen_table, desc.raygen_table);
	m_desc.miss_entry_size = ConvertTable(&m_desc.miss_table, desc.miss_table);
	m_desc.hit_group_entry_size = ConvertTable(&m_desc.hit_group_table, desc.hit_group_table);
}

DXR::ShaderBindingTable::ShaderBindingTable(
	ComPtrC<ID3D12Device> device,
	const Desc& desc
) noexcept
{
	ConvertDesc(desc);

	const D3D12_HEAP_PROPERTIES upload_heap_props = {
		D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 
	};

	m_storage_resource = std::make_shared<KGL::ResourcesBase>(
		device, StorageSize(), &upload_heap_props,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ
		);
}

void DXR::ShaderBindingTable::GenerateRayDesc(
	D3D12_DISPATCH_RAYS_DESC* out_desc,
	D3D12_GPU_VIRTUAL_ADDRESS start_addres
) const noexcept
{
	RCHECK(!out_desc, "out_desc ��nullptr");

	// ���������V�F�[�_�[�͏��SBT�̐擪�ɂ���܂��B
	auto& raygen_desc = out_desc->RayGenerationShaderRecord;
	raygen_desc.StartAddress = start_addres;
	raygen_desc.SizeInBytes = m_desc.raygen_entry_size * SCAST<UINT64>(m_desc.raygen_table.size());

	// �~�X�V�F�[�_�[�́A���������V�F�[�_�[�̒����2�Ԗڂ�SBT�Z�N�V�����ɂ���܂��B 
	auto& miss_desc = out_desc->MissShaderTable;
	miss_desc.StartAddress = raygen_desc.StartAddress + raygen_desc.SizeInBytes;
	miss_desc.SizeInBytes = m_desc.miss_entry_size * SCAST<UINT64>(m_desc.miss_table.size());
	miss_desc.StrideInBytes = m_desc.miss_entry_size;

	// �q�b�g�O���[�v�Z�N�V�����́A�~�X�V�F�[�_�[�̌�Ɏn�܂�܂��B
	auto& hit_group_desc = out_desc->HitGroupTable;
	hit_group_desc.StartAddress = miss_desc.StartAddress + miss_desc.SizeInBytes;
	hit_group_desc.SizeInBytes = m_desc.hit_group_entry_size * SCAST<UINT64>(m_desc.hit_group_table.size());
	hit_group_desc.StrideInBytes = m_desc.hit_group_entry_size;
}

UINT64 DXR::ShaderBindingTable::StorageSize() const noexcept
{
	return CONVERT::RoundUp(
		m_desc.raygen_entry_size * m_desc.raygen_table.size() +
		m_desc.miss_entry_size * m_desc.miss_table.size() +
		m_desc.hit_group_entry_size * m_desc.hit_group_table.size(),
		256 // 256 byte �A���C�����g
	);
}