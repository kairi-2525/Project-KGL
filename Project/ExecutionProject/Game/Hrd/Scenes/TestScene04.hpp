#pragma once

#include "../Scene.hpp"
#include <Dx12/Texture.hpp>
#include <Loader/PMDLoader.hpp>
#include <Loader/VMDLoader.hpp>
#include <Dx12/3D/PMDModel.hpp>
#include <Dx12/3D/PMDRenderer.hpp>
#include <Dx12/3D/Renderer.hpp>
#include <Dx12/2D/Renderer.hpp>
#include <Dx12/2D/Sprite.hpp>
#include <Dx12/PipelineState.hpp>
#include <Dx12/Shader.hpp>
#include <Base/Camera.hpp>
#include <Dx12/RenderTargetView.hpp>
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/Compute.hpp>
#include <Dx12/3D/Board.hpp>
#include "../Particle.hpp"
#include "../Fireworks.hpp"

#include "../Obj3D.hpp"

class TestScene04 : public SceneBase
{
	struct CbvParam
	{
		DirectX::XMFLOAT4X4 mat;
		DirectX::XMFLOAT4 color;
	};
private:
	UINT64 ct_particle, ct_frame_ptc, ct_fw, ct_gpu, ct_cpu, ct_fw_update, ct_map_update;

	DirectX::XMFLOAT4X4							proj;

	SceneBufferDx12<ParticleParent>				cpt_scene_buffer;
	SceneBufferDx12<SceneBase::SceneBuffers>	scene_buffer;
	KGL::VecCamera								camera;
	DirectX::XMFLOAT2							camera_angle;
	bool										use_gui;

	std::shared_ptr<KGL::BaseRenderer>			sprite_renderer;
	std::shared_ptr<KGL::Sprite>				sprite;
	std::shared_ptr<KGL::BaseRenderer>			board_renderer;
	std::shared_ptr<KGL::Board>					board;
	std::shared_ptr<KGL::DescriptorManager>		b_cbv_descmgr;
	KGL::DescriptorHandle						b_cbv;
	std::shared_ptr<KGL::DescriptorManager>		b_srv_descmgr;
	KGL::DescriptorHandle						b_srv;
	std::shared_ptr<KGL::Texture>				b_texture;
	D3D12_VERTEX_BUFFER_VIEW					b_vbv;

	std::shared_ptr<KGL::Resource<CbvParam>>	matrix_resource;

	KGL::TextureManager							tex_mgr;

	KGL::ComPtr<ID3D12CommandAllocator>			cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>		cmd_list;

	KGL::ComPtr<ID3D12CommandAllocator>			cpt_cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>		cpt_cmd_list;
	std::shared_ptr<KGL::CommandQueue>			cpt_cmd_queue;

	std::shared_ptr<KGL::RenderTargetView>		rtvs;
	std::vector<std::shared_ptr<KGL::Texture>>	rtv_textures;

	std::shared_ptr<KGL::Resource<Particle>>	particle_resource;
	std::vector<Particle>						frame_particles;
	std::shared_ptr<KGL::Resource<UINT32>>		particle_counter_res;
	std::shared_ptr<KGL::DescriptorManager>		particle_desc_mgr;
	std::shared_ptr<KGL::ComputePipline>		particle_pipeline;
	KGL::DescriptorHandle						particle_begin_handle;
	size_t										next_particle_offset;
	float										spawn_counter;

	std::vector<Fireworks>						fireworks;

	struct AlphaBuffer
	{
		DirectX::XMFLOAT4X4 wvp;
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT3	eye_pos;
		float				length_min;
		float				length_max;
	};
	DirectX::XMFLOAT3									grid_pos;
	std::shared_ptr<KGL::Resource<DirectX::XMFLOAT4>>	grid_vertex_resource;
	D3D12_VERTEX_BUFFER_VIEW							grid_vbv;
	std::shared_ptr<KGL::Resource<UINT16>>				grid_idx_resource;
	D3D12_INDEX_BUFFER_VIEW								grid_ibv;
	std::shared_ptr<KGL::BaseRenderer>					grid_renderer;
	SceneBufferDx12<AlphaBuffer>						grid_buffer;

	enum CUBE
	{
		FRONT,
		BACK,
		RIGHT,
		LEFT,
		TOP,
		BOTTOM,
		NUM
	};
	struct SkyVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};
	struct SkyTex
	{
		std::array<KGL::DescriptorHandle, CUBE::NUM>			handle;
		std::array<std::shared_ptr<KGL::Texture>, CUBE::NUM>	tex;
	};

	float														sky_scale;
	std::array<D3D12_VERTEX_BUFFER_VIEW, CUBE::NUM>				sky_vbv;
	std::shared_ptr<SkyTex>										select_sky;
	std::map<std::string, std::shared_ptr<SkyTex>>				sky_tex_data;
	std::shared_ptr<KGL::Resource<SkyVertex>>					sky_vbr;
	std::shared_ptr<KGL::BaseRenderer>							sky_renderer;
	SceneBufferDx12<DirectX::XMFLOAT4X4>						sky_buffer;
	std::shared_ptr<KGL::DescriptorManager>						sky_tex_desc_mgr;

public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;

	TestScene04() = default;
	~TestScene04() = default;
};