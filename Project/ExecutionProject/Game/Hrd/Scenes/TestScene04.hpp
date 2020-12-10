#pragma once

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
#include <Dx12/DescriptorHeap.hpp>
#include <Dx12/ConstantBuffer.hpp>
#include <Dx12/Compute.hpp>
#include <Dx12/3D/Board.hpp>
#include "../RenderTarget.hpp"
#include "../Particle.hpp"
#include "../Fireworks.hpp"
#include "../SkyMap.hpp"
#include "../Bloom.hpp"
#include "../Obj3D.hpp"
#include "../Camera.hpp"
#include "../DepthOfField.hpp"
#include "../FireworkCerialManager.hpp"
#include "../FireworksSpawnerManager.hpp"
#include "../ParticleManager.hpp"
#include "../Debug.hpp"
#include "../MSAA.hpp"
#include "../FXAA.hpp"
#include "../Scene.hpp"
#include "../GUIManager.hpp"
#include "../PlayerShotParameter.hpp"
#include <unordered_map>

class TestScene04 : public SceneBase
{
public:
	enum RT : UINT
	{
		MAIN,
		SUB,
		PTC_NON_BLOOM,
		PTC_BLOOM,
		FXAA_GRAY
	};
private:
	struct SceneBuffers : public SceneBase::SceneBuffers
	{
		DirectX::XMMATRIX inv_view;
		bool zero_texture;
	};
public:
	enum PTC_VT : UINT
	{
		NONE,
		COUNT4,
		COUNT8
	};
	static inline const std::vector<std::string> PTC_VT_TABLE =
	{
		"Particle_gs.hlsl",
		"Particle4_gs.hlsl",
		"Particle8_gs.hlsl"
	};
private:

	float												time_scale;
	bool												rt_gui_windowed;
	bool												sky_gui_windowed;

	DirectX::XMFLOAT4X4									proj;

	SceneBufferDx12<SceneBuffers>						scene_buffer;
	std::shared_ptr<FPSCamera>							camera;
	DirectX::XMFLOAT2									camera_angle;
	bool												use_gui;

	std::vector<std::shared_ptr<KGL::BaseRenderer>>		sprite_renderers;
	std::vector<std::shared_ptr<KGL::BaseRenderer>>		add_sprite_renderers;
	std::shared_ptr<KGL::BaseRenderer>					depth_sprite_renderer;
	std::shared_ptr<KGL::Sprite>						sprite;
	std::vector<std::vector<BoardRenderers>>			board_renderers;
	std::shared_ptr<KGL::Board>							board;
	std::shared_ptr<KGL::DescriptorManager>				b_cbv_descmgr;
	KGL::DescriptorHandle								b_cbv;

	D3D12_VERTEX_BUFFER_VIEW							b_ptc_vbv;
	D3D12_VERTEX_BUFFER_VIEW							b_pl_shot_ptc_vbv;

	KGL::TextureManager									tex_mgr;

	KGL::ComPtr<ID3D12CommandAllocator>					cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>				cmd_list;
	KGL::ComPtr<ID3D12CommandAllocator>					fast_cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>				fast_cmd_list;

	KGL::ComPtr<ID3D12CommandAllocator>					ptc_cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>				ptc_cmd_list;
	std::shared_ptr<KGL::CommandQueue>					ptc_cmd_queue;

	KGL::ComPtr<ID3D12CommandAllocator>					pl_ptc_cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>				pl_ptc_cmd_list;
	std::shared_ptr<KGL::CommandQueue>					pl_ptc_cmd_queue;

	std::shared_ptr<PlayerShotParametor>				pl_shot_param;
	std::vector<KGL::DescriptorHandle>					ptc_tex_srv_gui_handles;
	std::shared_ptr<ParticleTextureManager>				ptc_tex_mgr;
	std::shared_ptr<ParticleManager>					ptc_mgr;
	std::shared_ptr<ParticleManager>					pl_shot_ptc_mgr;

	std::shared_ptr<KGL::ComputePipline>				particle_pipeline;
	std::shared_ptr<KGL::ComputePipline>				particle_sort_pipeline;
	float												spawn_counter;

	std::shared_ptr<std::vector<Fireworks>>				fireworks;
	std::shared_ptr<std::vector<Fireworks>>				player_fireworks;

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

	std::shared_ptr<SkyManager>							sky_mgr;
	std::shared_ptr<BloomGenerator>						bloom_generator;

	std::shared_ptr<DOFGenerator>						dof_generator;

	std::shared_ptr<FCManager>							fc_mgr;
	std::shared_ptr<DebugManager>						debug_mgr;

	std::shared_ptr<std::vector<RenderTargetResource>>	rt_resources;
	std::shared_ptr<KGL::DescriptorManager>				depth_dsv_descriptor;
	std::shared_ptr<KGL::DescriptorManager>				depth_srv_descriptor;
	std::shared_ptr<MSAASelector>						msaa_selector;
	std::shared_ptr<std::vector<std::string>>			msaa_combo_texts;

	std::shared_ptr<FXAAManager>						fxaa_mgr;
	std::shared_ptr<FSManager>							fs_mgr;
	std::shared_ptr<GUIManager>							gui_mgr;
private:
	// レンダーターゲットとデプスステンシルを作成します。
	HRESULT PrepareRTAndDS(const SceneDesc& desc, DXGI_SAMPLE_DESC sample_desc);
public:
	HRESULT Load(const SceneDesc& desc) override;
	HRESULT Init(const SceneDesc& desc) override;
	HRESULT Update(const SceneDesc& desc, float elapsed_time) override;
	HRESULT FastRender(const SceneDesc& desc);
	HRESULT Render(const SceneDesc& desc);
	HRESULT UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene) override;
	void UpdateRenderTargetGui(const SceneDesc& desc);

	TestScene04() = default;
	~TestScene04() = default;
};