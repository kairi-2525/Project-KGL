#pragma once
#include "Particle.hpp"
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/Texture.hpp>
#include <Base/Directory.hpp>
#include <vector>

class ParticleManager
{
private:

	std::shared_ptr<KGL::Resource<ParticleParent>>	parent_res;
	KGL::DescriptorHandle							parent_begin_handle;

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
	void Dispatch(KGL::ComPtrC<ID3D12GraphicsCommandList>);
	void Update();
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
public:
	ParticleTextureManager(
		KGL::ComPtrC<ID3D12Device> device,
		const std::filesystem::path& dir,
		std::shared_ptr<KGL::DX12::TextureManager> texture_mgr = nullptr
	);
} typedef PTCTexMgr;