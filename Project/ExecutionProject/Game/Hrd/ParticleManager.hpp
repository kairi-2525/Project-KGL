#pragma once
#include "Particle.hpp"
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/Texture.hpp>
#include <Base/Directory.hpp>
#include <vector>
#include <Dx12/CommandQueue.hpp>
#include "Fireworks.hpp"
#include <Dx12/Compute.hpp>

class ParticleManager
{
private:
	struct CommandAllocatorAndList
	{
		KGL::ComPtr<ID3D12CommandAllocator> allocator;
		KGL::ComPtr<ID3D12GraphicsCommandList> list;
	};
	struct StepBuffer
	{
		UINT32	block_step;
		UINT32	sub_block_step;
	};
private:
	UINT32												step_size;
	UINT32												capacity;
	std::vector<CommandAllocatorAndList>				sort_cmds;
	UINT32												cmd_count;

	std::shared_ptr<KGL::Resource<ParticleParent>>		parent_res;
	std::shared_ptr<KGL::Resource<AffectObjects>>		affect_obj_resource;
	std::shared_ptr<KGL::DescriptorHandle>				parent_begin_handle;
	std::shared_ptr<KGL::DescriptorHandle>				affect_obj_begin_handle;
	std::shared_ptr<KGL::DescriptorHandle>				lgn_cbv_handle;
	std::vector<std::shared_ptr<KGL::DescriptorHandle>> step_cbv_handles;

	std::shared_ptr<KGL::Resource<Particle>>			resource;
	std::shared_ptr<KGL::Resource<UINT32>>				counter_res;
	std::shared_ptr<KGL::Resource<UINT32>>				lgn_resource;
	std::shared_ptr<KGL::MultiResource<StepBuffer>>		step_resource;
	std::shared_ptr<KGL::DescriptorManager>				desc_mgr;
	KGL::DescriptorHandle								begin_handle;

	UINT64												particle_total_num;
public:
	std::vector<Particle>								frame_particles;
	std::vector<AffectObjects>							affect_objects;
public:
	explicit ParticleManager(KGL::ComPtrC<ID3D12Device> device, UINT32 capacity) noexcept;
	void SetParent(const ParticleParent& particle_parent);
	void UpdateAffectObjects(const std::vector<Fireworks>& affect_fireworks = {});
	void UpdateDispatch(KGL::ComPtrC<ID3D12GraphicsCommandList>);
	void AddSortDispatchCommand(std::shared_ptr<KGL::ComputePipline> sort_pipline, std::vector<ID3D12CommandList*>* out_cmd_lists);
	void ResetSortCommands();
	void CPUUpdate(const std::vector<Fireworks>& affect_fireworks = {});
	void CPUSort();
	void AddToFrameParticle();
	void Clear();
	UINT UpdateGui(UINT idx = 0);
	UINT32 ResetCounter();
	std::shared_ptr<KGL::Resource<Particle>> Resource() const noexcept { return resource; }
	std::shared_ptr<KGL::Resource<ParticleParent>> ParentResource() const noexcept { return parent_res; }
	UINT32 Size();
	UINT32 Capacity() const noexcept { return capacity; }
	std::vector<float> Get() const noexcept 
	{
		const UINT32 size = SCAST<UINT32>(resource->Size());
		std::vector<float> result(size);
		Particle* p = resource->Map();
		for (UINT32 i = 0u; i < size; i++)
		{
			result[i] = p[i].exist_time;
		}
		resource->Unmap();
		return result;
	}
};


class ParticleTextureManager
{
private:
	KGL::Files files;
	std::shared_ptr<KGL::TextureManager>		tex_mgr;
	std::vector<std::shared_ptr<KGL::Texture>>	textures;
	std::shared_ptr<KGL::DescriptorManager>		srv_descriptor;
	std::vector<KGL::DescriptorHandle>			handles;

	// ì«Ç›çûÇ›éûÇ…égópÇµÅALoadWaitÇ≈îjä¸Ç≥ÇÍÇÈïœêî
	KGL::ComPtr<ID3D12CommandAllocator>			cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>		cmd_list;
	std::unique_ptr<KGL::CommandQueue>			cmd_queue;
	std::vector<ComPtr<ID3D12Resource>>			upload_heaps;

public:
	ParticleTextureManager(
		KGL::ComPtrC<ID3D12Device> device,
		const std::filesystem::path& dir,
		std::shared_ptr<KGL::DX12::TextureManager> texture_mgr = nullptr
	);
	void CreateSRV(
		KGL::ComPtrC<ID3D12Device> device,
		std::vector<KGL::DescriptorHandle>* out_handles,
		std::shared_ptr<KGL::DescriptorManager> srv_descriptor
	);
	void LoadWait();
	bool IsLoaded() const { return cmd_queue.operator bool(); }
	const std::vector<std::shared_ptr<KGL::Texture>>& GetTextures() const { return textures; }

	const KGL::DescriptorHandle& GetHandle() const { return *(handles.begin()); }
} typedef PTCTexMgr;