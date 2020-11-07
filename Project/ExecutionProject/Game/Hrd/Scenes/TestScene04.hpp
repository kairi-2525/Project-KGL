#pragma once

#include <Dx12/Texture.hpp>
#include <Helper/Timer.hpp>
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
#include <unordered_map>

class TestScene04 : public SceneBase
{
	enum RT : UINT
	{
		MAIN,
		SUB,
		PTC_NON_BLOOM,
		PTC_BLOOM,
		FXAA_GRAY
	};
	enum PTC_VT : UINT
	{
		NONE,
		COUNT4,
		COUNT8
	};
	struct CbvParam
	{
		DirectX::XMFLOAT4X4 mat;
		DirectX::XMFLOAT4 color;
	};
	struct RenderTargetResource
	{
		struct Texture
		{
			std::shared_ptr<KGL::Texture>		tex;
			KGL::DescriptorHandle				gui_srv_handle;
		};
		std::vector<Texture>						render_targets;
		std::shared_ptr<KGL::RenderTargetView>		rtvs;
		KGL::DescriptorHandle						dsv_handle;
		std::shared_ptr<KGL::Texture>				depth_stencil;
		KGL::DescriptorHandle						depth_srv_handle;
		KGL::DescriptorHandle						depth_gui_srv_handle;
	};
	struct BoardRenderers
	{
		std::shared_ptr<KGL::BaseRenderer>			simple;
		std::shared_ptr<KGL::BaseRenderer>			add_pos;
		std::shared_ptr<KGL::BaseRenderer>			simple_wire;
		std::shared_ptr<KGL::BaseRenderer>			add_pos_wire;
		std::shared_ptr<KGL::BaseRenderer>			dsv;
		std::shared_ptr<KGL::BaseRenderer>			dsv_add_pos;
	};
	struct SceneBuffers : public SceneBase::SceneBuffers
	{
		DirectX::XMMATRIX inv_view;
		bool zero_texture;
	};
private:
	static inline const std::vector<std::string> PTC_VT_TABLE =
	{
		"Particle_gs.hlsl",
		"Particle4_gs.hlsl",
		"Particle8_gs.hlsl"
	};
private:
	KGL::Timer											tm_update;
	KGL::Timer											tm_render;
	KGL::Timer											tm_ptc_sort;
	KGL::Timer											tm_ptc_update_gpu;
	KGL::Timer											tm_ptc_update_cpu;
	UINT64												ct_ptc_total_max;
	UINT64												ct_ptc_frame_max;
	UINT64												ct_fw_max;

	float												ptc_key_spawn_counter;
	float												time_scale;
	bool												use_gpu;
	bool												spawn_fireworks;
	bool												after_blooming;
	bool												rt_gui_windowed;
	bool												sky_gui_windowed;

	DirectX::XMFLOAT4X4									proj;

	SceneBufferDx12<SceneBuffers>						scene_buffer;
	std::shared_ptr<DemoCamera>							camera;
	DirectX::XMFLOAT2									camera_angle;
	bool												use_gui;
	bool												stop_time;
	bool												particle_wire;

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

	std::shared_ptr<KGL::Resource<CbvParam>>			matrix_resource;

	KGL::TextureManager									tex_mgr;

	KGL::ComPtr<ID3D12CommandAllocator>					cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>				cmd_list;
	KGL::ComPtr<ID3D12CommandAllocator>					fast_cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>				fast_cmd_list;

	KGL::ComPtr<ID3D12CommandAllocator>					cpt_cmd_allocator;
	KGL::ComPtr<ID3D12GraphicsCommandList>				cpt_cmd_list;
	std::shared_ptr<KGL::CommandQueue>					cpt_cmd_queue;

	PTC_VT												ptc_vt_type;
	bool												ptc_dof_flg;
	std::vector<KGL::DescriptorHandle>					ptc_tex_srv_gui_handles;
	std::shared_ptr<ParticleTextureManager>				ptc_tex_mgr;
	std::shared_ptr<ParticleManager>					ptc_mgr;
	std::shared_ptr<ParticleManager>					pl_shot_ptc_mgr;
	std::shared_ptr<KGL::ComputePipline>				particle_pipeline;
	float												spawn_counter;

	std::vector<Fireworks>								fireworks;
	std::vector<Fireworks>								player_fireworks;

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

	bool												sky_draw;
	std::shared_ptr<SkyManager>							sky_mgr;
	std::shared_ptr<BloomGenerator>						bloom_generator;
	std::array<KGL::DescriptorHandle, BloomGenerator::RTV_MAX> bl_c_imgui_handles;
	std::array<KGL::DescriptorHandle, BloomGenerator::RTV_MAX> bl_w_imgui_handles;
	std::array<KGL::DescriptorHandle, BloomGenerator::RTV_MAX> bl_h_imgui_handles;
	KGL::DescriptorHandle								bl_bloom_imgui_handle;

	bool												dof_flg;
	std::shared_ptr<DOFGenerator>						dof_generator;
	std::array<KGL::DescriptorHandle, 8u>				dof_imgui_handles;

	std::shared_ptr<FCManager>							fc_mgr;
	std::shared_ptr<DebugManager>						debug_mgr;

	std::vector<RenderTargetResource>					rt_resources;
	std::shared_ptr<KGL::DescriptorManager>				depth_dsv_descriptor;
	std::shared_ptr<KGL::DescriptorManager>				depth_srv_descriptor;
	std::shared_ptr<MSAASelector>						msaa_selector;
	std::vector<std::string>							msaa_combo_texts;
	bool												msaa_depth_draw;	// MSAA描画時に深度テクスチャをチェックしている際に使用されます。

	std::shared_ptr<FXAAManager>						fxaa_mgr;
	std::shared_ptr<FSManager>							fs_mgr;
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
	void ResetCounterMax();
	void UpdateRenderTargetGui(const SceneDesc& desc);

	TestScene04() = default;
	~TestScene04() = default;
};