#include <Dx12/DXR/AccelerationStructure.hpp>

using namespace KGL;

void DXR::ComputeASBufferSizes(
	ComPtrC<ID3D12Device5> device,
	bool allow_update,
	const std::vector<BLASInstance>& instances,
	UINT64* scratch_size_in_bytes,
	UINT64* result_size_in_bytes,
	UINT64* descriptors_size_in_bytes
)
{
	RCHECK(!scratch_size_in_bytes, "scratch_size_in_bytes が nullptr");
	RCHECK(!result_size_in_bytes, "result_size_in_bytes が nullptr");
	RCHECK(!descriptors_size_in_bytes, "descriptors_size_in_bytes が nullptr");

	const auto flags = allow_update	? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
									: D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuild_desc = {};
	prebuild_desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	prebuild_desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuild_desc.NumDescs = static_cast<UINT>(instances.size());
	prebuild_desc.Flags = flags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&prebuild_desc, &info);

	// 256 アライメント
	*result_size_in_bytes = ResourcesBase::AlignmentSize(info.ResultDataMaxSizeInBytes);
	*scratch_size_in_bytes = ResourcesBase::AlignmentSize(info.ScratchDataSizeInBytes);
	*descriptors_size_in_bytes = ResourcesBase::AlignmentSize(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * static_cast<UINT64>(instances.size()));
}

static void GenerateTLAS(
	ComPtrC<ID3D12Device4> device,
	ComPtrC<ID3D12GraphicsCommandList4> cmd_list,
	const std::vector<DXR::BLASInstance>& instances,
	const DXR::ASResources& as_resources,
	UINT64 instance_descs_size_in_bytes,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags
)
{
	auto& result_buffer = as_resources.result->Data();
	auto& scratch_buffer = as_resources.scratch->Data();
	auto& descriptors_buffer = as_resources.instance_desc->Data();
	const uint32_t instance_count = static_cast<UINT>(instances.size());

	// ターゲットディスクリプタバッファ内のディスクリプタをコピーします。
	{
		D3D12_RAYTRACING_INSTANCE_DESC* instance_descs;
		descriptors_buffer->Map(0, nullptr, reinterpret_cast<void**>(&instance_descs));
		RCHECK(!instance_descs, "descriptorsBufferのMapに失敗");

		// 初回のみメモリをゼロクリアする
		ZeroMemory(instance_descs, instance_descs_size_in_bytes);

		// 各インスタンスを生成
		for (uint32_t i = 0; i < instance_count; i++)
		{
			instance_descs[i].InstanceID = instances[i].instance_id;
			instance_descs[i].InstanceContributionToHitGroupIndex = instances[i].hit_group_index;
			instance_descs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			DirectX::XMMATRIX m = DirectX::XMMatrixTranspose(instances[i].transform);
			memcpy(instance_descs[i].Transform, &m, sizeof(instance_descs[i].Transform));
			instance_descs[i].AccelerationStructure = instances[i].blas->Data()->GetGPUVirtualAddress();
			instance_descs[i].InstanceMask = 0xFF;
		}
		descriptors_buffer->Unmap(0, nullptr);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc = {};
	build_desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	build_desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	build_desc.Inputs.InstanceDescs = descriptors_buffer->GetGPUVirtualAddress();
	build_desc.Inputs.NumDescs = instance_count;
	build_desc.DestAccelerationStructureData = { result_buffer->GetGPUVirtualAddress() };
	build_desc.ScratchAccelerationStructureData = { scratch_buffer->GetGPUVirtualAddress() };
	build_desc.SourceAccelerationStructureData = 0;
	build_desc.Inputs.Flags = flags;

	// TLAS を生成
	cmd_list->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);

	// バリアを設定して生成を待機する
	D3D12_RESOURCE_BARRIER uav_barrier;
	uav_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uav_barrier.UAV.pResource = result_buffer.Get();
	uav_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	cmd_list->ResourceBarrier(1, &uav_barrier);
}

DXR::ASResources DXR::CreateTLAS(
	ComPtrC<ID3D12Device5> device,
	ComPtrC<ID3D12GraphicsCommandList4> cmd_list,
	const std::vector<BLAS>& instances,
	UINT hit_group_index
)
{
	const size_t size = instances.size();
	std::vector<BLASInstance> blas_instances(size);

	// すべてのインスタンスを収集
	for (size_t i = 0u; i < size; i++)
	{
		const auto& instance = instances[i];
		auto& blas_instance = blas_instances[i];

		blas_instance.blas = instance.first;
		blas_instance.transform = instance.second;
		blas_instance.instance_id = i;
		blas_instance.hit_group_index = hit_group_index;
	}

	// データを格納するためのサイズを算出する
	UINT64 scratch_size = {}, result_size = {}, instance_descs_size = {};
	ComputeASBufferSizes(device, true, blas_instances, &scratch_size, &result_size, &instance_descs_size);

	const D3D12_HEAP_PROPERTIES default_heap_prop = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
	const D3D12_HEAP_PROPERTIES upload_heap_prop = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };

	ASResources as_resources;

	as_resources.scratch = std::make_shared<KGL::Resource<CHAR>>(
		device, SCAST<size_t>(scratch_size),
		&default_heap_prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);

	as_resources.result = std::make_shared<KGL::Resource<CHAR>>(
		device, SCAST<size_t>(result_size),
		&default_heap_prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
		);

	as_resources.instance_desc = std::make_shared<KGL::Resource<CHAR>>(
		device, SCAST<size_t>(instance_descs_size),
		&upload_heap_prop, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ
		);

	GenerateTLAS(
		device, cmd_list,
		blas_instances,
		as_resources,
		instance_descs_size,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
	);

	return as_resources;
}