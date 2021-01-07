#include <Dx12/DXR/StaticModel.hpp>

using namespace KGL;

void DXR::StaticModel::ComputeVertexBufferSize(
	ComPtrC<ID3D12Device5> device,
	std::shared_ptr<const StaticModelLoader> loader,
	UINT64* scratch_size_in_bytes,
	UINT64* result_size_in_bytes
) noexcept
{
	RCHECK(!scratch_size_in_bytes, "scratch_size_in_bytes が nullptr");
	RCHECK(!result_size_in_bytes, "result_size_in_bytes が nullptr");

	for (const auto& mtl : m_materials)
	{
		const auto& loader_mtl = loader->GetMaterials()->find(mtl.first);
		const auto& mtl_data = mtl.second;
		const auto& vertex_resource = mtl_data.rs_vertices;
		const auto& vertex_view = mtl_data.vertex_buffer_view;
		vertex_resource->SizeInBytes();
		// 3xf32の頂点座標と32ビットのインデックスを使用して、
		// 不透明な三角形と見なされる入力データを表すDX12記述子を作成します。
		D3D12_RAYTRACING_GEOMETRY_DESC geometry_desc = {};
		geometry_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometry_desc.Triangles.VertexBuffer.StartAddress =
			vertex_resource->Data()->GetGPUVirtualAddress();
		geometry_desc.Triangles.VertexBuffer.StrideInBytes = sizeof(S_MODEL::Vertex);
		geometry_desc.Triangles.VertexCount = SCAST<UINT>(vertex_resource->Size());
		geometry_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometry_desc.Triangles.IndexBuffer = 0;
		geometry_desc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
		geometry_desc.Triangles.IndexCount = 0;
		geometry_desc.Triangles.Transform3x4 = 0;

		geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		if (loader_mtl != loader->GetMaterials()->cend())
		{
			if (loader_mtl->second.param.dissolve < 1.f)
				geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
			else
				geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		}

		m_vertex_geometry_descs.push_back(geometry_desc);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS as_input_desc;
	as_input_desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	as_input_desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	as_input_desc.NumDescs = static_cast<UINT>(m_vertex_geometry_descs.size());
	as_input_desc.pGeometryDescs = m_vertex_geometry_descs.data();
	// 更新不要のフラグ (更新する場合は D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE )
	as_input_desc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO as_info_desc = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&as_input_desc, &as_info_desc);

	// 256 アライメント
	*scratch_size_in_bytes = KGL::ResourcesBase::AlignmentSize(as_info_desc.ScratchDataSizeInBytes);
	*result_size_in_bytes = KGL::ResourcesBase::AlignmentSize(as_info_desc.ResultDataMaxSizeInBytes);
}

static void GenerateBLAS(
	ComPtrC<ID3D12Device4> device,
	ComPtrC<ID3D12GraphicsCommandList4> cmd_list,
	std::shared_ptr<KGL::Resource<CHAR>> scratch_buffer,
	std::shared_ptr<KGL::Resource<CHAR>> result_buffer,
	const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& vertex_descriptors
)
{
	auto flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	
	// Create a descriptor of the requested builder work, to generate a
	// bottom-level AS from the input parameters
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc;
	build_desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	build_desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	build_desc.Inputs.NumDescs = static_cast<UINT>(vertex_descriptors.size());
	build_desc.Inputs.pGeometryDescs = vertex_descriptors.data();
	build_desc.DestAccelerationStructureData = { result_buffer->Data()->GetGPUVirtualAddress() };
	build_desc.ScratchAccelerationStructureData = { scratch_buffer->Data()->GetGPUVirtualAddress() };
	build_desc.SourceAccelerationStructureData = 0;
	build_desc.Inputs.Flags = flags;

	// Build the AS
	cmd_list->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

	// Wait for the builder to complete by setting a barrier on the resulting
	// buffer. This is particularly important as the construction of the top-level
	// hierarchy may be called right afterwards, before executing the command
	// list.
	D3D12_RESOURCE_BARRIER uav_barrier;
	uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uav_barrier.UAV.pResource = result_buffer->Data().Get();
	uav_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	cmd_list->ResourceBarrier(1, &uav_barrier);
}

DXR::StaticModel::StaticModel(
	ComPtrC<ID3D12Device5> device,
	ComPtrC<ID3D12GraphicsCommandList4> cmd_list,
	std::shared_ptr<const StaticModelLoader> loader,
	std::shared_ptr<TextureManager> tex_mgr,
	std::shared_ptr<DescriptorManager> descriptor_mgr
) noexcept : _3D::StaticModel(device, loader, tex_mgr, descriptor_mgr)
{
	{	// _3D::StaticModelで例外処理された場合ここでもしておく
		if (!loader) return;
		auto materials = loader->GetMaterials();
		if (!materials) return;
	}

	UINT64 scratch_size_in_bytes = {}, result_size_in_bytes = {};
	ComputeVertexBufferSize(device, loader, &scratch_size_in_bytes, &result_size_in_bytes);

	const D3D12_HEAP_PROPERTIES default_heap_prop = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

	m_as_resources.scratch = std::make_shared<KGL::Resource<CHAR>>(
		device, SCAST<size_t>(scratch_size_in_bytes),
		&default_heap_prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON
		);

	m_as_resources.result = std::make_shared<KGL::Resource<CHAR>>(
		device, SCAST<size_t>(result_size_in_bytes),
		&default_heap_prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
		);

	GenerateBLAS(
		device, cmd_list,
		m_as_resources.scratch, m_as_resources.result,
		m_vertex_geometry_descs
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