#include "../Hrd/ParticleManager.hpp"
#include <algorithm>
#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Dx12/Helper.hpp>

ParticleManager::ParticleManager(KGL::ComPtrC<ID3D12Device> device, UINT64 capacity) noexcept
{
	desc_mgr = std::make_shared<KGL::DescriptorManager>(device, 3u);
	parent_res = std::make_shared<KGL::Resource<ParticleParent>>(device, 1u);
	affect_obj_resource = std::make_shared<KGL::Resource<AffectObjects>>(device, 100u);

	{
		parent_begin_handle = desc_mgr->Alloc();
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = parent_res->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = SCAST<UINT>(parent_res->SizeInBytes());
		device->CreateConstantBufferView(&cbv_desc, parent_begin_handle.Cpu());
	}
	{
		affect_obj_begin_handle = desc_mgr->Alloc();
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = affect_obj_resource->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = SCAST<UINT>(affect_obj_resource->SizeInBytes());
		device->CreateConstantBufferView(&cbv_desc, affect_obj_begin_handle.Cpu());
	}

	D3D12_HEAP_PROPERTIES prop = {};
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.CreationNodeMask = 1;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	prop.Type = D3D12_HEAP_TYPE_CUSTOM;
	prop.VisibleNodeMask = 1;

	frame_particles.reserve(capacity);
	resource = std::make_shared<KGL::Resource<Particle>>(device, capacity, &prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	counter_res = std::make_shared<KGL::Resource<UINT32>>(device, 1u, &prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.Buffer.NumElements = KGL::SCAST<UINT>(resource->Size());
	uav_desc.Buffer.StructureByteStride = sizeof(Particle);
	uav_desc.Buffer.CounterOffsetInBytes = 0u;

	begin_handle = desc_mgr->Alloc();
	device->CreateUnorderedAccessView(resource->Data().Get(), counter_res->Data().Get(), &uav_desc, begin_handle.Cpu());
}

void ParticleManager::SetParent(const ParticleParent& particle_parent)
{
	auto* parent_data = parent_res->Map(0, &CD3DX12_RANGE(0, 0));
	*parent_data = particle_parent;
	parent_res->Unmap(0, &CD3DX12_RANGE(0, 0));
}

void ParticleManager::SetAffectObjects(const std::vector<AffectObjects>& affect_objects, const std::vector<Fireworks>& affect_fireworks)
{
	const UINT32 oj_size = affect_objects.size();
	const UINT32 fw_size = affect_fireworks.size();
	const UINT32 size = oj_size + fw_size;
	{
		auto* parent_data = parent_res->Map(0, &CD3DX12_RANGE(0, 0));
		parent_data->affect_obj_count = size;
		parent_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
	{
		auto* mp_affect_objects = affect_obj_resource->Map(0, &CD3DX12_RANGE(0, 0));

		UINT32 i = 0u;
		for (; i < oj_size; i++)
		{
			mp_affect_objects[i] = affect_objects[i];
		}
		for (; i < size; i++)
		{
			mp_affect_objects[i].pos = affect_fireworks[i - oj_size].pos;
			mp_affect_objects[i].mass = affect_fireworks[i - oj_size].mass;
		}
		affect_obj_resource->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
}

void ParticleManager::Dispatch(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list)
{
	cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(counter_res->Data().Get()));

	cmd_list->SetDescriptorHeaps(1, begin_handle.Heap().GetAddressOf());
	cmd_list->SetComputeRootDescriptorTable(2, begin_handle.Gpu());

	cmd_list->SetDescriptorHeaps(1, parent_begin_handle.Heap().GetAddressOf());
	cmd_list->SetComputeRootDescriptorTable(0, parent_begin_handle.Gpu());

	cmd_list->SetDescriptorHeaps(1, affect_obj_begin_handle.Heap().GetAddressOf());
	cmd_list->SetComputeRootDescriptorTable(1, affect_obj_begin_handle.Gpu());

	const UINT ptcl_size = std::min<UINT>(SCAST<UINT>(particle_total_num), SCAST<UINT>(resource->Size()));
	DirectX::XMUINT3 patch = {};
	constexpr UINT patch_max = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	patch.x = (ptcl_size / 64) + ((ptcl_size % 64) > 0 ? 1 : 0);
	patch.x = std::min<uint32_t>(patch.x, patch_max);
	patch.y = 1;
	patch.z = 1;

	cmd_list->Dispatch(patch.x, patch.y, patch.z);
}

void ParticleManager::Update(
	const std::vector<AffectObjects>& affect_objects,
	const std::vector<Fireworks>& affect_fireworks
)
{
	using namespace DirectX;

	auto* p_counter = counter_res->Map(0, &CD3DX12_RANGE(0, 0));
	auto particles = resource->Map(0, &CD3DX12_RANGE(0, 0));
	const size_t i_max = std::min<size_t>(particle_total_num, resource->Size());
	const auto* cb = parent_res->Map(0, &CD3DX12_RANGE(0, 0));
	for (int i = 0; i < i_max; i++)
	{
		if (!particles[i].Alive()) continue;
		particles[i].Update(cb->elapsed_time, cb, affect_objects, affect_fireworks);
		(*p_counter)++;
	}
	parent_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	resource->Unmap(0, &CD3DX12_RANGE(0, 0));
	counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
}

void ParticleManager::Sort()
{
	if (particle_total_num > 0)
	{
		next_particle_offset = std::min<size_t>(particle_total_num, resource->Size()) * sizeof(Particle);
		auto particles = resource->Map(0u, &CD3DX12_RANGE(0, next_particle_offset));
		const auto size = next_particle_offset / sizeof(Particle);
		UINT64 alive_count = 0;
		constexpr Particle clear_ptc_value = {};
		for (auto idx = 0; idx < size; idx++)
		{
			if (particles[idx].Alive())
			{
				if (idx > alive_count)
				{
					particles[alive_count] = particles[idx];
					particles[idx] = clear_ptc_value;
				}
				alive_count++;
			}
		}
		resource->Unmap(0u, &CD3DX12_RANGE(0, next_particle_offset));
		next_particle_offset = sizeof(Particle) * alive_count;
	}
}

void ParticleManager::AddToFrameParticle()
{
	if (!frame_particles.empty())
	{
		size_t frame_add_ptc_num = 0u;

		D3D12_RANGE range;
		range.Begin = next_particle_offset;
		range.End = range.Begin + sizeof(Particle) * (SCAST<SIZE_T>(frame_particles.size()));
		const size_t offset_max = sizeof(Particle) * (resource->Size());
		const size_t bi = range.Begin / sizeof(Particle);
		if (range.End > offset_max)
			range.End = offset_max;
		const auto check_count_max = (range.End - range.Begin) / sizeof(Particle);
		auto particles = resource->Map(0u, &range);
		UINT check_count = 0u;
		for (; check_count < check_count_max;)
		{
			size_t idx = bi + check_count;
			check_count++;
			if (particles[idx].Alive()) continue;
			particles[idx] = frame_particles.back();
			frame_particles.pop_back();
			frame_add_ptc_num++;
			if (frame_particles.empty()) break;
		}
		resource->Unmap(0u, &range);
		next_particle_offset = range.Begin + sizeof(Particle) * check_count++;

		auto* p_counter = counter_res->Map(0, &CD3DX12_RANGE(0, 0));
		*p_counter += SCAST<UINT32>(frame_add_ptc_num);
		particle_total_num = *p_counter;
		counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
}

void ParticleManager::Clear()
{
	auto* p_particles = resource->Map(0, &CD3DX12_RANGE(0, 0));
	Particle particle_base = {};
	std::fill(&p_particles[0], &p_particles[resource->Size()], particle_base);
	resource->Unmap(0, &CD3DX12_RANGE(0, 0));

	ResetCounter();

	next_particle_offset = 0u;

	frame_particles.clear();
}

UINT32 ParticleManager::ResetCounter()
{
	UINT32 result;
	auto* p_counter = counter_res->Map(0, &CD3DX12_RANGE(0, 0));
	result = *p_counter;
	*p_counter = 0u;
	counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	return result;
}

UINT32 ParticleManager::Size()
{
	UINT32 result;
	auto* p_counter = counter_res->Map(0, &CD3DX12_RANGE(0, 0));
	result = *p_counter;
	counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	return result;
}

ParticleTextureManager::ParticleTextureManager(
	KGL::ComPtrC<ID3D12Device> device,
	const std::filesystem::path& dir,
	std::shared_ptr<KGL::DX12::TextureManager> texture_mgr
)
{
	KGL::Directory directory(dir);
	files = directory.GetFiles(".DDS");
	tex_mgr = texture_mgr;
	if (!tex_mgr)
	{
		tex_mgr = std::make_shared<KGL::TextureManager>();
	}

	D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
	cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmd_queue_desc.NodeMask = 0;
	cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmd_queue = std::make_unique<KGL::CommandQueue>(device, cmd_queue_desc);

	HRESULT hr;
	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗");
	cmd_list->Close();

	textures.reserve(files.size() + 1u);

	// 0番目は白テクスチャ
	textures.emplace_back(std::make_shared<KGL::Texture>(device));

	const std::string directory_pass = directory.GetPath().string() + "\\";
	for (const auto& file : files)
	{		
		cmd_list->Reset(cmd_allocator.Get(), nullptr);
		textures.emplace_back(std::make_shared<KGL::Texture>(
			device, cmd_list, &upload_heaps,
			directory_pass + file.string(), 1u, tex_mgr.get())
		);
		cmd_list->Close();
		//コマンドの実行
		ID3D12CommandList* cmd_lists[] = {
		   cmd_list.Get(),
		};
		cmd_queue->Data()->ExecuteCommandLists(1, cmd_lists);
	}
	cmd_queue->Signal();

	// SRV作成
	srv_descriptor = std::make_shared<KGL::DescriptorManager>(device, textures.size());
	CreateSRV(device, &handles, srv_descriptor);
}

// SRV作成
void ParticleTextureManager::CreateSRV(
	KGL::ComPtrC<ID3D12Device> device,
	std::vector<KGL::DescriptorHandle>* out_handles,
	std::shared_ptr<KGL::DescriptorManager> srv_descriptor
)
{
	if (!out_handles) return;
	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	size_t i = out_handles->size();
	out_handles->resize(i + textures.size());
	for (const auto& tex : textures)
	{
		auto& handle = (*out_handles)[i];
		handle = srv_descriptor->Alloc();

		auto tex_desc = tex->Data()->GetDesc();
		srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
		srv_desc.Format = tex_desc.Format;

		device->CreateShaderResourceView(
			tex->Data().Get(),
			&srv_desc,
			handle.Cpu()
		);

		i++;
	}
}

void ParticleTextureManager::LoadWait()
{
	if (cmd_queue)
	{
		cmd_queue->Wait();
		cmd_queue = nullptr;
		cmd_allocator->Reset();
		cmd_allocator = nullptr;
		cmd_list = nullptr;
		upload_heaps.clear();
	}
}