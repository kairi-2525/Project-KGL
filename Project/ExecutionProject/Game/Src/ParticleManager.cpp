#include "../Hrd/ParticleManager.hpp"
#include <algorithm>
#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Dx12/Helper.hpp>

ParticleManager::ParticleManager(KGL::ComPtrC<ID3D12Device> device, UINT32 capacity) noexcept
{
	cmd_count = 0u;

	// 2の何条かを求めて正規化する(何条かはGPUで使用するためリソースに記録)
	lgn_resource = std::make_shared<KGL::Resource<UINT32>>(device, 1u);
	{
		UINT32* mapped_lgn = lgn_resource->Map();
		*mapped_lgn = SCAST<UINT32>(std::ceilf(std::log10f(capacity) / std::log10f(2.f)));
		this->capacity = 1 << *mapped_lgn;

		step_size = 0u;
		for (UINT32 i = 1u; i <= *mapped_lgn; i++)
		{
			step_size += i;
		}

		lgn_resource->Unmap();
	}

	HRESULT hr = S_OK;
	sort_cmds.resize(SCAST<size_t>(step_size));
	for (auto& sort_cmd : sort_cmds)
	{
		hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(
				device, &sort_cmd.allocator, &sort_cmd.list,
				D3D12_COMMAND_LIST_TYPE_COMPUTE
			);
		RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗");
	}

	desc_mgr = std::make_shared<KGL::DescriptorManager>(device, 3u + 1u + step_size);

	// CBV を作成
	{
		// 2の何条か（lgn）のリソース
		lgn_cbv_handle = std::make_shared<KGL::DescriptorHandle>(desc_mgr->Alloc());
		lgn_resource->CreateCBV(lgn_cbv_handle);

		// Block Step のリソース(複数)
		step_resource = std::make_shared<KGL::MultiResource<StepBuffer>>(device, step_size);
		step_cbv_handles.resize(step_size);
		for (UINT32 i = 0u; i < step_size; i++)
		{
			step_cbv_handles[i] = std::make_shared<KGL::DescriptorHandle>(desc_mgr->Alloc());
			step_resource->CreateCBV(step_cbv_handles[i], i);
		}

		// Parentのリソース
		parent_res = std::make_shared<KGL::Resource<ParticleParent>>(device, 1u);
		parent_begin_handle = std::make_shared<KGL::DescriptorHandle>(desc_mgr->Alloc());
		parent_res->CreateCBV(parent_begin_handle);

		// 引力に影響するAffectのリソース
		affect_obj_resource = std::make_shared<KGL::Resource<AffectObjects>>(device, 100u);
		affect_obj_begin_handle = std::make_shared<KGL::DescriptorHandle>(desc_mgr->Alloc());
		affect_obj_resource->CreateCBV(affect_obj_begin_handle);
	}

	// UAV を作成
	{
		D3D12_HEAP_PROPERTIES prop = {};
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
		prop.CreationNodeMask = 1;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
		prop.Type = D3D12_HEAP_TYPE_CUSTOM;
		prop.VisibleNodeMask = 1;

		frame_particles.reserve(this->capacity);
		resource = std::make_shared<KGL::Resource<Particle>>(device, this->capacity, &prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
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

	// step リソースを設定
	{
		StepBuffer* p = step_resource->Map();
		std::vector<StepBuffer> sb;
		for (UINT32 block = 2; block <= this->capacity; block *= 2)
		{
			for (UINT32 step = block / 2; step >= 1; step /= 2)
			{
				p->block_step = block;
				p->sub_block_step = step;
				p = step_resource->IncrementPtr(p);
			}
		}
		step_resource->Unmap();
	}

	Clear();
}

void ParticleManager::SetParent(const ParticleParent& particle_parent)
{
	auto* parent_data = parent_res->Map();
	*parent_data = particle_parent;
	parent_res->Unmap();
}

void ParticleManager::SetAffectObjects(const std::vector<AffectObjects>& affect_objects, const std::vector<Fireworks>& affect_fireworks)
{
	const UINT32 max_size = SCAST<UINT32>(affect_obj_resource->Size());
	const UINT32 oj_size = (std::min)(SCAST<UINT32>(affect_objects.size()), max_size);
	const UINT32 size = (std::min)(oj_size + SCAST<UINT32>(affect_fireworks.size()), max_size);
	{
		auto* parent_data = parent_res->Map();
		parent_data->affect_obj_count = size;
		parent_res->Unmap();
	}
	{
		auto* mp_affect_objects = affect_obj_resource->Map();

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
		affect_obj_resource->Unmap();
	}
}

void ParticleManager::UpdateDispatch(KGL::ComPtrC<ID3D12GraphicsCommandList> cmd_list)
{
	cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(counter_res->Data().Get()));

	cmd_list->SetDescriptorHeaps(1, begin_handle.Heap().GetAddressOf());
	cmd_list->SetComputeRootDescriptorTable(2, begin_handle.Gpu());

	cmd_list->SetDescriptorHeaps(1, parent_begin_handle->Heap().GetAddressOf());
	cmd_list->SetComputeRootDescriptorTable(0, parent_begin_handle->Gpu());

	cmd_list->SetDescriptorHeaps(1, affect_obj_begin_handle->Heap().GetAddressOf());
	cmd_list->SetComputeRootDescriptorTable(1, affect_obj_begin_handle->Gpu());

	const UINT ptcl_size = std::min<UINT>(SCAST<UINT>(particle_total_num), SCAST<UINT>(resource->Size()));
	DirectX::XMUINT3 patch = {};
	constexpr UINT patch_max = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	patch.x = (ptcl_size / 64) + ((ptcl_size % 64) > 0 ? 1 : 0);
	patch.x = std::min<uint32_t>(patch.x, patch_max);
	patch.y = 1;
	patch.z = 1;

	cmd_list->Dispatch(patch.x, patch.y, patch.z);
}

void ParticleManager::ResetSortCommands()
{
	for (UINT32 idx = 0u; idx < cmd_count; idx++)
	{
		sort_cmds[idx].allocator->Reset();
		sort_cmds[idx].list->Reset(sort_cmds[idx].allocator.Get(), nullptr);
	}
}

void ParticleManager::AddSortDispatchCommand(
	std::shared_ptr<KGL::ComputePipline> sort_pipline, 
	std::vector<ID3D12CommandList*>* out_cmd_lists)
{
	const UINT ptcl_size = std::min<UINT>(SCAST<UINT>(particle_total_num), SCAST<UINT>(resource->Size()));
	UINT32 lgn = SCAST<UINT32>(std::ceilf(std::log10f(ptcl_size) / std::log10f(2.f)));

	out_cmd_lists->reserve(out_cmd_lists->size() + step_size);
	cmd_count = 0u;

	// LGNリソースを設定
	{
		UINT32* mapped_lgn = lgn_resource->Map();
		*mapped_lgn = lgn;
		lgn_resource->Unmap();
	}

	const UINT32 ptc_value = 1 << lgn;
	constexpr UINT patch_max = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	const UINT x = (std::min)((std::max)(SCAST<UINT>(ptc_value) / 64u, 1u), patch_max);

	for (int block = 2; block <= ptc_value; block *= 2)
	{
		for (int step = block / 2; step >= 1; step /= 2)
		{
			auto& cmd_list = sort_cmds[cmd_count].list;
			//cmd_list->ResourceBarrier(1, &rb);
			sort_pipline->SetState(cmd_list);

			cmd_list->SetDescriptorHeaps(1, desc_mgr->Heap().GetAddressOf());
			cmd_list->SetComputeRootDescriptorTable(0, lgn_cbv_handle->Gpu());
			cmd_list->SetComputeRootDescriptorTable(2, begin_handle.Gpu());

			cmd_list->SetComputeRootDescriptorTable(1, step_cbv_handles[cmd_count]->Gpu());

			cmd_list->Dispatch(x, 1u, 1u);

			cmd_list->Close();

			out_cmd_lists->push_back(cmd_list.Get());

			cmd_count++;
		}
	}
}

void ParticleManager::Update(
	const std::vector<AffectObjects>& affect_objects,
	const std::vector<Fireworks>& affect_fireworks
)
{
	using namespace DirectX;

	auto* p_counter = counter_res->Map();
	auto particles = resource->Map();
	const size_t i_max = std::min<size_t>(particle_total_num, resource->Size());
	const auto* cb = parent_res->Map();
	for (int i = 0; i < i_max; i++)
	{
		if (!particles[i].Alive()) continue;
		particles[i].Update(cb->elapsed_time, cb, affect_objects, affect_fireworks);
		(*p_counter)++;
	}
	parent_res->Unmap();
	resource->Unmap();
	counter_res->Unmap();
}

void ParticleManager::CPUSort()
{
	if (particle_total_num > 0)
	{
		auto next_particle_offset = std::min<size_t>(particle_total_num, resource->Size()) * sizeof(Particle);
		auto particles = resource->Map(0u, CD3DX12_RANGE(0, next_particle_offset));
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
		resource->Unmap(0u, CD3DX12_RANGE(0, next_particle_offset));
	}
}

void ParticleManager::AddToFrameParticle()
{
	if (!frame_particles.empty())
	{
		auto* p_counter = counter_res->Map();
		particle_total_num = *p_counter;

		size_t frame_add_ptc_num = 0u;

		D3D12_RANGE range;

		const size_t resource_size = resource->Size();

		range.Begin = sizeof(Particle) * (std::min)(Size(), SCAST<UINT32>(resource_size));

		range.End = range.Begin + sizeof(Particle) * (SCAST<SIZE_T>(frame_particles.size()));
		const size_t offset_max = sizeof(Particle) * (resource->Size());
		const size_t bi = SCAST<size_t>(particle_total_num);
		if (range.End > offset_max)
			range.End = offset_max;
		const auto check_count_max = (range.End - range.Begin) / sizeof(Particle);
		auto particles = resource->Map(0u, range);
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
		frame_particles.clear();
		resource->Unmap(0u, range);

		particle_total_num += SCAST<UINT32>(frame_add_ptc_num);
		*p_counter = particle_total_num;
		counter_res->Unmap();
	}
}

void ParticleManager::Clear()
{
	auto* p_particles = resource->Map();
	Particle particle_base = {};
	std::fill(&p_particles[0], &p_particles[resource->Size()], particle_base);
	resource->Unmap();

	ResetCounter();

	particle_total_num = 0u;

	frame_particles.clear();
}

UINT32 ParticleManager::ResetCounter()
{
	UINT32 result;
	auto* p_counter = counter_res->Map();
	result = *p_counter;
	*p_counter = 0u;
	counter_res->Unmap();
	return result;
}

UINT32 ParticleManager::Size()
{
	UINT32 result;
	auto* p_counter = counter_res->Map();
	result = *p_counter;
	counter_res->Unmap();
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