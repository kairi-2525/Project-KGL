#include <Dx12/DXR/StaticModel.hpp>

using namespace KGL;

void DXR::StaticModel::ComputeVertexBufferSize(
	ComPtrC<ID3D12Device5> device,
	std::shared_ptr<const StaticModelLoader> loader,
	UINT64* scratch_size_in_bytes,
	UINT64* result_size_in_bytes
) noexcept
{
	RCHECK(!scratch_size_in_bytes, "scratch_size_in_bytes �� nullptr");
	RCHECK(!result_size_in_bytes, "result_size_in_bytes �� nullptr");

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> vertex_descriptors;
	for (const auto& mtl : m_materials)
	{
		const auto& loader_mtl = loader->GetMaterials()->find(mtl.first);
		const auto& mtl_data = mtl.second;
		const auto& vertex_resource = mtl_data.rs_vertices;
		const auto& vertex_view = mtl_data.vertex_buffer_view;
		vertex_resource->SizeInBytes();
		// 3xf32�̒��_���W��32�r�b�g�̃C���f�b�N�X���g�p���āA
		// �s�����ȎO�p�`�ƌ��Ȃ������̓f�[�^��\��DX12�L�q�q���쐬���܂��B
		D3D12_RAYTRACING_GEOMETRY_DESC descriptor = {};
		descriptor.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		descriptor.Triangles.VertexBuffer.StartAddress =
			vertex_resource->Data()->GetGPUVirtualAddress();
		descriptor.Triangles.VertexBuffer.StrideInBytes = sizeof(S_MODEL::Vertex);
		descriptor.Triangles.VertexCount = SCAST<UINT>(vertex_resource->Size());
		descriptor.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		descriptor.Triangles.IndexBuffer = 0;
		descriptor.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
		descriptor.Triangles.IndexCount = 0;
		descriptor.Triangles.Transform3x4 = 0;

		descriptor.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		if (loader_mtl != loader->GetMaterials()->cend())
		{
			if (loader_mtl->second.param.dissolve < 1.f)
				descriptor.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
			else
				descriptor.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		}

		vertex_descriptors.push_back(descriptor);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS as_input_desc;
	as_input_desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	as_input_desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	as_input_desc.NumDescs = static_cast<UINT>(vertex_descriptors.size());
	as_input_desc.pGeometryDescs = vertex_descriptors.data();
	// �X�V�s�v�̃t���O (�X�V����ꍇ�� D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE )
	as_input_desc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO as_info_desc = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&as_input_desc, &as_info_desc);

	// 256 �A���C�����g
	*scratch_size_in_bytes = KGL::ResourcesBase::AlignmentSize(as_info_desc.ScratchDataSizeInBytes);
	*result_size_in_bytes = KGL::ResourcesBase::AlignmentSize(as_info_desc.ResultDataMaxSizeInBytes);
}

static void GenerateBLAS()
{

}

DXR::StaticModel::StaticModel(
	ComPtrC<ID3D12Device5> device,
	std::shared_ptr<const StaticModelLoader> loader,
	std::shared_ptr<TextureManager> tex_mgr,
	std::shared_ptr<DescriptorManager> descriptor_mgr
) noexcept : _3D::StaticModel(device, loader, tex_mgr, descriptor_mgr)
{
	{	// _3D::StaticModel�ŗ�O�������ꂽ�ꍇ�����ł����Ă���
		if (!loader) return;
		auto materials = loader->GetMaterials();
		if (!materials) return;
	}

	UINT64 scratch_size_in_bytes = {}, result_size_in_bytes = {};
	ComputeVertexBufferSize(device, loader, &scratch_size_in_bytes, &result_size_in_bytes);
	ASResources bottom_level_buffer;

	bottom_level_buffer.scratch = std::make_shared<KGL::Resource<CHAR>>(
		device, SCAST<size_t>(scratch_size_in_bytes),
		nullptr, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON
		);

	bottom_level_buffer.result = std::make_shared<KGL::Resource<CHAR>>(
		device, SCAST<size_t>(result_size_in_bytes),
		nullptr, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
		);
}

void DXR::StaticModel::Render(ComPtrC<ID3D12GraphicsCommandList> cmd_list) const noexcept
{
	for (const auto& it : m_materials)
	{
		const auto& mt = it.second;

		cmd_list->SetDescriptorHeaps(1, mt.param_cbv_handle->Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, mt.param_cbv_handle->Gpu());

		cmd_list->SetDescriptorHeaps(1, mt.ambient.handle->Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(1, mt.ambient.handle->Gpu());

		cmd_list->IASetVertexBuffers(0, 1, &mt.vertex_buffer_view);
		cmd_list->DrawInstanced(SCAST<UINT>(mt.rs_vertices->Size()), 1, 0, 0);
	}
}