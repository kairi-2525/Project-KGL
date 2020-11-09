#pragma once
#include "Particle.hpp"
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/Texture.hpp>
#include <Base/Directory.hpp>
#include <vector>
#include <Dx12/CommandQueue.hpp>
#include "Fireworks.hpp"

class ParticleManager
{
private:

	std::shared_ptr<KGL::Resource<ParticleParent>>	parent_res;
	std::shared_ptr<KGL::Resource<AffectObjects>>	affect_obj_resource;
	KGL::DescriptorHandle							parent_begin_handle;
	KGL::DescriptorHandle							affect_obj_begin_handle;

	std::shared_ptr<KGL::Resource<Particle>>		resource;
	std::shared_ptr<KGL::Resource<UINT32>>			counter_res;
	std::shared_ptr<KGL::DescriptorManager>			desc_mgr;
	KGL::DescriptorHandle							begin_handle;

	UINT64											particle_total_num;
	size_t											next_particle_offset;
public:
	std::vector<Particle>							frame_particles;
public:
	explicit ParticleManager(KGL::ComPtrC<ID3D12Device> device, UINT64 capacity) noexcept;
	void SetParent(const ParticleParent& particle_parent);
	void SetAffectObjects(const std::vector<AffectObjects>& affect_objects, const std::vector<Fireworks>& affect_fireworks);
	void Dispatch(KGL::ComPtrC<ID3D12GraphicsCommandList>);
	void Update(const std::vector<AffectObjects>& affect_objects, const std::vector<Fireworks>& affect_fireworks);
	void Sort();
	void AddToFrameParticle();
	void Clear();
	UINT32 ResetCounter();
	std::shared_ptr<KGL::Resource<Particle>> Resource() const noexcept { return resource; }
	std::shared_ptr<KGL::Resource<ParticleParent>> ParentResource() const noexcept { return parent_res; }
	UINT32 Size();
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