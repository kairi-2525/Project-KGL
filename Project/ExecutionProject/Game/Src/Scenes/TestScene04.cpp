#include "../../Hrd/Scenes/TestScene03.hpp"
#include "../../Hrd/Scenes/TestScene04.hpp"
#include "../../Hrd/Scenes/TestScene00.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Helper/Timer.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>
#include <random>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include <algorithm>

#define USE_GPU
#define USE_GPU_OPTION1

HRESULT TestScene04::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();
	const auto max_sample_count = desc.app->GetMaxSampleCount();
	const auto max_sample_quality = desc.app->GetMaxQualityLevel();

	// コンピュート用・描画用のコマンドアロケーター・コマンドリストを初期化
	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);
	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cpt_cmd_allocator, &cpt_cmd_list, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);
	{	// コンピュート用
		// コマンドキューの生成
		D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
		cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		cmd_queue_desc.NodeMask = 0;
		cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

		cpt_cmd_queue = std::make_shared<KGL::CommandQueue>(device, cmd_queue_desc);
		RCHECK(FAILED(hr), "コマンドキューの生成に失敗！", hr);
	}


	{	// スプライト用PSOの作成(半透明、加算、高輝度抽出)
		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
		sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device, desc.dxc, renderer_desc);

		renderer_desc.blend_types[0] = KGL::BDTYPE::ADD;
		add_sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device, desc.dxc, renderer_desc);

		//renderer_desc.ps_desc.hlsl = "./HLSL/2D/SpriteAdd_ps.hlsl";
		renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
		high_sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device, desc.dxc, renderer_desc);
		sprite = std::make_shared<KGL::Sprite>(device);

		renderer_desc.render_targets.clear();
		renderer_desc.depth_desc.DepthEnable = TRUE;
		renderer_desc.depth_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		renderer_desc.depth_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/DepthWriteTextureMS_ps.hlsl";
		depth_sprite_renderer = std::make_shared<KGL::BaseRenderer>(device, desc.dxc, renderer_desc);
	}

	{	// パーティクル用PSOを作成(描画用)
		auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;
		//renderer_desc.depth_desc = {};
		renderer_desc.vs_desc.hlsl = "./HLSL/3D/Particle_vs.hlsl";
		renderer_desc.gs_desc.hlsl = "./HLSL/3D/Particle_gs.hlsl";
		renderer_desc.input_layouts.clear();
		renderer_desc.input_layouts.push_back({
			"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"COLOR_SPEED", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"MASS", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE_WIDTH", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE_FRONT", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE_BACK", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE_SPEED_WIDTH", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE_SPEED_FRONT", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE_SPEED_BACK", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"RESISTIVITY", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"ANGLE", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"RENDER_MODE", 0, DXGI_FORMAT_R32_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"ACCS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"EXIST", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.other_desc.topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		renderer_desc.render_targets.clear();
		renderer_desc.render_targets.reserve(2u);
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/ParticleDepth_ps.hlsl";
		board_renderer_dsv = std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
		renderer_desc.vs_desc.hlsl = "./HLSL/3D/ParticlePos_vs.hlsl";
		board_renderer_dsv_pos = std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);

		renderer_desc.vs_desc.hlsl = "./HLSL/3D/Particle_vs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/Particle_ps.hlsl";
		renderer_desc.blend_types[0] = KGL::BLEND::TYPE::ADD;
		renderer_desc.blend_types[1] = KGL::BLEND::TYPE::ADD;
		renderer_desc.depth_desc = {};
		renderer_desc.render_targets.push_back(DXGI_FORMAT_R8G8B8A8_UNORM);
		renderer_desc.render_targets.push_back(DXGI_FORMAT_R8G8B8A8_UNORM);
		board_renderer = std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);

		renderer_desc.vs_desc.hlsl = "./HLSL/3D/ParticlePos_vs.hlsl";
		board_renderer_pos = std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);

		board = std::make_shared<KGL::Board>(device);
	}

	//hr = cpt_scene_buffer.Load(desc);
	//RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);
	hr = scene_buffer.Load(desc);
	RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);

	ptc_mgr = std::make_shared<ParticleManager>(device, 1000000u);
	pl_shot_ptc_mgr = std::make_shared<ParticleManager>(device, 100000u);

	particle_pipeline = std::make_shared<KGL::ComputePipline>(device, desc.dxc);
	b_cbv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	matrix_resource = std::make_shared<KGL::Resource<CbvParam>>(device, 1u);
	b_tex_data[0].tex = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Particles/particle.png");
	b_tex_data[1].tex = std::make_shared<KGL::Texture>(device);

	b_srv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 2u);

	{
		constexpr UINT grid_size = 100u;
		std::vector<DirectX::XMFLOAT4> grid_vertices(grid_size * grid_size);
		using KGL::SCAST;
		float base_pos = -SCAST<float>(grid_size - 1) / 2;
		for (UINT z = 0u; z < grid_size; z++)
		{
			for (UINT x = 0u; x < grid_size; x++)
			{
				UINT idx = (z * grid_size) + x;
				grid_vertices[idx] = { base_pos + SCAST<float>(x), 0.f, base_pos + SCAST<float>(z), 1.f };
			}
		}
		constexpr UINT grid_idx_size = (grid_size - 1u) * 2u;
		std::vector<UINT16> grid_indices(grid_idx_size * grid_size * 2);
		for (UINT z = 0u; z < grid_size; z++)
		{
			for (UINT x = 0u; x < grid_idx_size; x++)
			{
				UINT idxw = (z * grid_idx_size) + x;
				grid_indices[idxw] = (z * grid_size) + (x / 2) + (x % 2);
				UINT idxh = grid_idx_size * grid_size + (z * grid_idx_size) + x;
				grid_indices[idxh] = z + (((x / 2) + (x % 2)) * grid_size);
			}
		}
		grid_vertex_resource = std::make_shared<KGL::Resource<DirectX::XMFLOAT4>>(device, grid_vertices.size());
		grid_idx_resource = std::make_shared<KGL::Resource<UINT16>>(device, grid_indices.size());

		auto* mapped_vertices = grid_vertex_resource->Map(0, &CD3DX12_RANGE(0, 0));
		std::copy(grid_vertices.cbegin(), grid_vertices.cend(), mapped_vertices);
		grid_vertex_resource->Unmap(0, &CD3DX12_RANGE(0, 0));

		auto* mapped_indices = grid_idx_resource->Map(0, &CD3DX12_RANGE(0, 0));
		std::copy(grid_indices.cbegin(), grid_indices.cend(), mapped_indices);
		grid_idx_resource->Unmap(0, &CD3DX12_RANGE(0, 0));

		grid_vbv.BufferLocation = grid_vertex_resource->Data()->GetGPUVirtualAddress();
		grid_vbv.SizeInBytes = sizeof(grid_vertices[0]) * grid_vertices.size();
		grid_vbv.StrideInBytes = sizeof(grid_vertices[0]);

		grid_ibv.BufferLocation = grid_idx_resource->Data()->GetGPUVirtualAddress();
		grid_ibv.SizeInBytes = sizeof(grid_indices[0]) * grid_indices.size();
		grid_ibv.Format = DXGI_FORMAT_R16_UINT;

		auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;
		renderer_desc.input_layouts.pop_back();
		renderer_desc.input_layouts.pop_back();
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/NearAlpha_ps.hlsl";
		renderer_desc.vs_desc.hlsl = "./HLSL/3D/NearAlpha_vs.hlsl";
		renderer_desc.root_params.pop_back();
		renderer_desc.root_params.pop_back();
		renderer_desc.static_samplers.clear();
		renderer_desc.other_desc.topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		renderer_desc.blend_types[0] = KGL::BLEND::TYPE::ALPHA;

		grid_renderer = std::make_shared<KGL::BaseRenderer>(device, desc.dxc, renderer_desc);

		grid_buffer.Load(desc);
	}
	{	// パーティクルのテクスチャ用SRVを作成
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Format = b_tex_data[0].tex->Data()->GetDesc().Format;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		b_tex_data[0].handle = std::make_shared<KGL::DescriptorHandle>(b_srv_descmgr->Alloc());
		b_tex_data[0].imgui_handle = desc.imgui_heap_mgr->Alloc();
		device->CreateShaderResourceView(b_tex_data[0].tex->Data().Get(), &srv_desc, b_tex_data[0].handle->Cpu());
		device->CreateShaderResourceView(b_tex_data[0].tex->Data().Get(), &srv_desc, b_tex_data[0].imgui_handle.Cpu());
		srv_desc.Format = b_tex_data[1].tex->Data()->GetDesc().Format;
		b_tex_data[1].handle = std::make_shared<KGL::DescriptorHandle>(b_srv_descmgr->Alloc());
		b_tex_data[1].imgui_handle = desc.imgui_heap_mgr->Alloc();
		device->CreateShaderResourceView(b_tex_data[1].tex->Data().Get(), &srv_desc, b_tex_data[1].handle->Cpu());
		device->CreateShaderResourceView(b_tex_data[1].tex->Data().Get(), &srv_desc, b_tex_data[1].imgui_handle.Cpu());
		b_select_srv_handle = b_tex_data[0].handle;
	}
	{
		b_ptc_vbv.BufferLocation = ptc_mgr->Resource()->Data()->GetGPUVirtualAddress();
		b_ptc_vbv.SizeInBytes = ptc_mgr->Resource()->SizeInBytes();
		b_ptc_vbv.StrideInBytes = sizeof(Particle);

		b_pl_shot_ptc_vbv.BufferLocation = pl_shot_ptc_mgr->Resource()->Data()->GetGPUVirtualAddress();
		b_pl_shot_ptc_vbv.SizeInBytes = pl_shot_ptc_mgr->Resource()->SizeInBytes();
		b_pl_shot_ptc_vbv.StrideInBytes = sizeof(Particle);
	}

	fireworks.reserve(10000u);

	
	sky_mgr = std::make_shared<SkyManager>(device, desc.dxc, desc.imgui_heap_mgr, max_sample_count, max_sample_quality);

	{
		bloom_generator = std::make_shared<BloomGenerator>(device, desc.dxc, desc.app->GetRtvBuffers().at(0));
		
		const auto& tex = bloom_generator->GetTextures();
		UINT idx = 0u;
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		for (auto& imgui_handle : bloom_imgui_handles)
		{
			imgui_handle = desc.imgui_heap_mgr->Alloc();
			srv_desc.Format = tex[idx]->Data()->GetDesc().Format;
			device->CreateShaderResourceView(tex[idx]->Data().Get(), &srv_desc, imgui_handle.Cpu());
			idx++;
		}
	}

	{	// 被写界深度
		dof_generator = std::make_shared<DOFGenerator>(device, desc.dxc, desc.app->GetRtvBuffers().at(0));

		const auto& tex = dof_generator->GetTextures();
		UINT idx = 0u;
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		for (auto& imgui_handle : dof_imgui_handles)
		{
			imgui_handle = desc.imgui_heap_mgr->Alloc();
			srv_desc.Format = tex[idx]->Data()->GetDesc().Format;
			device->CreateShaderResourceView(tex[idx]->Data().Get(), &srv_desc, imgui_handle.Cpu());
			idx++;
		}
	}

	fc_mgr = std::make_shared<FCManager>("./Assets/Effects/Fireworks/");

	debug_mgr = std::make_shared<DebugManager>(device, desc.dxc, max_sample_count, max_sample_quality);

	msaa_selector = std::make_shared<MSAASelector>(desc.app->GetMaxSampleCount());
	msaa_combo_texts.push_back("OFF");

	{
		{
			auto& rtrc = rt_resources.emplace_back();
			constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
			
			rtrc.render_targets.emplace_back(std::make_shared<KGL::Texture>(
				device, desc.app->GetRtvBuffers().at(0), clear_value));
			rtrc.render_targets.emplace_back(std::make_shared<KGL::Texture>(
				device, desc.app->GetRtvBuffers().at(0), clear_value));
			const auto& resources = KGL::TEXTURE::GetResources(rtrc.render_targets);
			rtrc.rtvs = std::make_shared<KGL::RenderTargetView>(device, resources);
		}
		{
			constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
			/*ptc_rtv_texs.emplace_back(std::make_shared<KGL::Texture>(
				device, desc.app->GetRtvBuffers().at(0), clear_value));*/

			D3D12_RESOURCE_DESC res_desc = desc.app->GetRtvBuffers().at(0)->GetDesc();
			res_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			res_desc.MipLevels = 8u;
			ptc_rtv_texs.emplace_back(std::make_shared<KGL::Texture>(
				device, res_desc, CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, (float*)&clear_value)));
			res_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			ptc_rtv_texs.emplace_back(std::make_shared<KGL::Texture>(
				device, res_desc, CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, (float*)&clear_value)));
			const auto& resources = KGL::TEXTURE::GetResources(ptc_rtv_texs);
			ptc_rtvs = std::make_shared<KGL::RenderTargetView>(device, resources);

			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srv_desc.Texture2D.MipLevels = 1;
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			for (auto& tex : ptc_rtv_texs)
			{
				auto& imgui_handle = ptc_srv_gui_handles.emplace_back();
				imgui_handle = desc.imgui_heap_mgr->Alloc();

				srv_desc.Format = tex->Data()->GetDesc().Format;
				device->CreateShaderResourceView(tex->Data().Get(), &srv_desc, imgui_handle.Cpu());
			}
		}
	}
	// MSAA用レンダーターゲット / 深度テクスチャ / DSV を作成
	{
		// レンダーターゲットテクスチャ用
		const auto& render_target = desc.app->GetRtvBuffers()[0];
		auto rtv_rs_desc = render_target->GetDesc();
		DirectX::XMFLOAT4 clear_color(0.f, 0.f, 0.f, 0.f);
		D3D12_CLEAR_VALUE clear_value = CD3DX12_CLEAR_VALUE(rtv_rs_desc.Format, (float*)&clear_color);
		rtv_rs_desc.SampleDesc.Count = 1;

		// 深度テクスチャ用
		msaa_dsv_descriptor = std::make_shared<KGL::DescriptorManager>(
			device, SCAST<UINT>(msaa_selector->GetMaxScale()), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
			);
		D3D12_CLEAR_VALUE depth_clear_value = {};
		depth_clear_value.DepthStencil.Depth = 1.0f;		// 深さの最大値でクリア
		depth_clear_value.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;	// 32ビットfloat値としてクリア
		auto dsv_rs_desc = desc.app->GetDsvBuffer()->GetDesc();
		dsv_rs_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		dsv_rs_desc.SampleDesc.Count = 1;
		dsv_rs_desc.MipLevels = 1;
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;	// フラグ無し

		// SRV用
		depth_srv_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, SCAST<UINT>(msaa_selector->GetMaxScale() + 1u));
		depth_srv_handle = depth_srv_desc_mgr->Alloc();
		depth_srv_imgui_handle = desc.imgui_heap_mgr->Alloc();
		{	// MSAA off　用のSRVは先に作っておく
			D3D12_SHADER_RESOURCE_VIEW_DESC rs_desc = {};

			rs_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
			rs_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			rs_desc.Texture2D.MipLevels = 1;
			rs_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			device->CreateShaderResourceView(
				desc.app->GetDsvBuffer().Get(),
				&rs_desc,
				depth_srv_handle.Cpu()
			);
			device->CreateShaderResourceView(
				desc.app->GetDsvBuffer().Get(),
				&rs_desc,
				depth_srv_imgui_handle.Cpu()
			);
		}
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_rs_desc = {};
		srv_rs_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		srv_rs_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		srv_rs_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// 作成
		std::vector<ComPtr<ID3D12Resource>> resources;

		for (; rtv_rs_desc.SampleDesc.Count < desc.app->GetMaxSampleCount();)
		{
			// RT用テクスチャ
			rtv_rs_desc.SampleDesc.Count *= 2;
			auto& it = rt_resources.emplace_back();
			it.render_targets.emplace_back(std::make_shared<KGL::Texture>(
					device, rtv_rs_desc, clear_value,
					D3D12_RESOURCE_STATE_RESOLVE_SOURCE
				));
			resources.push_back(it.render_target->Data());

			it.render_target->Data()->SetName((L"RenderTarget MSAAx" + std::to_wstring(rtv_rs_desc.SampleDesc.Count)).c_str());
			
			// 深度テクスチャ
			dsv_rs_desc.SampleDesc.Count = rtv_rs_desc.SampleDesc.Count;
			it.depth_stencil = std::make_shared<KGL::Texture>(device,
				dsv_rs_desc,
				depth_clear_value,
				D3D12_RESOURCE_STATE_DEPTH_WRITE
				);

			it.depth_stencil->Data()->SetName((L"DepthStencil MSAAx" + std::to_wstring(dsv_rs_desc.SampleDesc.Count)).c_str());

			// DSV作成
			it.dsv_handle = msaa_dsv_descriptor->Alloc();
			device->CreateDepthStencilView(it.depth_stencil->Data().Get(), &dsv_desc, it.dsv_handle.Cpu());
			
			// SRV作成(Imgui用も作成)
			it.depth_srv_handle = depth_srv_desc_mgr->Alloc();
			device->CreateShaderResourceView(
				it.depth_stencil->Data().Get(),
				&srv_rs_desc,
				it.depth_srv_handle.Cpu()
			);

			// Combo用text
			msaa_combo_texts.push_back("x" + std::to_string(rtv_rs_desc.SampleDesc.Count));
		}
		
		// RTVを作成
		msaa_rtvs = std::make_shared<KGL::RenderTargetView>(device, resources, nullptr, D3D12_SRV_DIMENSION_TEXTURE2DMS);
	
	}

	return hr;
}

HRESULT TestScene04::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();

	camera = std::make_shared<DemoCamera>(XMFLOAT3(0.f, 200.f, 0.f), XMFLOAT3(0.f, 200.f, -100.f), 30.f);

	//XMStoreFloat3(&scene_buffer.mapped_data->light_vector, XMVector3Normalize(XMVectorSet(+0.2f, -0.7f, 0.5f, 0.f)));

	const XMMATRIX proj_mat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(window_size.x) / static_cast<float>(window_size.y),	// アスペクト比
		1.0f, 1000.0f // near, far
	);
	XMStoreFloat4x4(&proj, proj_mat);
	
	ptc_mgr->Clear();
	pl_shot_ptc_mgr->Clear();

	XMMATRIX view = camera->GetView();
	{
		grid_pos = { 0.f, 0.f, 0.f };
		XMMATRIX W, S, R, T;
		S = XMMatrixScaling(1.f, 1.f, 1.f);
		R = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
		T = XMMatrixTranslation(grid_pos.x, grid_pos.y, grid_pos.z);
		W = S * R * T;
		XMMATRIX WVP = W * view * proj_mat;
		XMStoreFloat4x4(&grid_buffer.mapped_data->world, W);
		XMStoreFloat4x4(&grid_buffer.mapped_data->wvp, WVP);
		grid_buffer.mapped_data->length_min = 10.f;
		grid_buffer.mapped_data->length_max = 50.f;
		grid_buffer.mapped_data->eye_pos = camera->GetPos();
	}
	{
		XMMATRIX W, S, R, T;
		S = XMMatrixScaling(1.f, 1.f, 1.f);
		R = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
		T = XMMatrixTranslation(0.f, 0.f, -1.f);
		W = S * R * T;

		XMMATRIX WVP = W * view * proj_mat;
		CbvParam* param = matrix_resource->Map();
		XMStoreFloat4x4(&param->mat, WVP);
		param->color = { 1.f, 1.f, 1.f, 1.f };
		matrix_resource->Unmap();
	}

	ParticleParent ptc_parent{};
	ptc_parent.center_pos = { 0.f, -6378.1f * 1000.f, 0.f };
	ptc_parent.center_mass = 5.9724e24f;
	ptc_parent.resistivity = 1.f;
	ptc_mgr->SetParent(ptc_parent);
	pl_shot_ptc_mgr->SetParent(ptc_parent);

	spawn_counter = 0.f;
	ptc_key_spawn_counter = 0.f;

	camera_angle = { 0.f, 0.f };

	sky_mgr->Init(view * proj_mat);

	use_gui = true;

	ResetCounterMax();
	time_scale = 1.f;
	use_gpu = true;
	spawn_fireworks = false;

	dof_flg = false;

	bloom_generator->SetKernel(8u);
	after_blooming = false;

	rt_gui_windowed = false;
	sky_gui_windowed = false;

	{	// キューブを簡易描画するマネージャー(キューブ以外のものも対応予定)
		debug_mgr->ClearStaticObjects();
		std::vector<std::shared_ptr<DebugManager::Object>> objects;
		std::shared_ptr<DebugManager::Cube> target_cube = std::make_shared<DebugManager::Cube>();
		auto loop_pos = target_cube->pos = camera->center;
		target_cube->color = { 1.f, 1.f, 1.f, 1.f };
		target_cube->scale = { 1.f, 1.f, 1.f };
		target_cube->rotate = {};
#if 0
		objects.push_back(std::dynamic_pointer_cast<DebugManager::Object>(target_cube));
#else
		objects.reserve(1000u + 1u);
		for (UINT i = 0u; i < 10u; i++)
		{
			for (UINT i = 0u; i < 10u; i++)
			{
				for (UINT i = 0u; i < 10u; i++)
				{
					std::shared_ptr<DebugManager::Cube> target_cube_copy = std::make_shared<DebugManager::Cube>(*target_cube);
					loop_pos.z += 1.5f;
					target_cube_copy->pos = loop_pos;
					objects.push_back(std::dynamic_pointer_cast<DebugManager::Object>(target_cube_copy));
				}
				loop_pos.z = target_cube->pos.z;
				loop_pos.y += 1.5f;
			}
			loop_pos.y = target_cube->pos.y;
			loop_pos.x += 1.5f;
		}
#endif
		debug_mgr->AddStaticObjects(objects);
	}

	// MSAAをのセレクターに最大値をセット
	msaa_selector->SetScale(msaa_selector->GetMaxScale());

	return S_OK;
}

void TestScene04::ResetCounterMax()
{
	ct_particle = ct_frame_ptc = ct_fw = ct_gpu = ct_cpu = ct_fw_update = ct_map_update = 0u;
}

void TestScene04::UpdateRenderTargetGui(const SceneDesc& desc)
{
	auto gui_size = ImGui::GetWindowSize();
	ImVec2 image_size = { gui_size.x * 0.8f, gui_size.y * 0.8f };
	if (ImGui::TreeNode("DSV"))
	{
		//ImGui::GetWindowDrawList()->AddImage((ImTextureID)it.imgui_handle.Gpu().ptr,
		// ImVec2(0, 0), image_size, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 0, 0, 255));
		
		ImGui::Image((ImTextureID)depth_srv_imgui_handle.Gpu().ptr, image_size);
		msaa_depth_draw = true;
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Particles"))
	{
		for (const auto& handle : ptc_srv_gui_handles)
			ImGui::Image((ImTextureID)handle.Gpu().ptr, image_size);
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Bloom"))
	{
		ImVec2 halh_image_size = { image_size.x * 0.5f, image_size.y * 0.5f };
		UINT idx = 0u;
		for (const auto& handle : bloom_imgui_handles)
		{
			ImGui::Image((ImTextureID)handle.Gpu().ptr, halh_image_size);
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("DOF"))
	{
		ImVec2 halh_image_size = { image_size.x * 0.5f, image_size.y * 0.5f };
		UINT idx = 0u;
		for (const auto& handle : dof_imgui_handles)
		{
			ImGui::Image((ImTextureID)handle.Gpu().ptr, halh_image_size);
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		ImGui::TreePop();
	}

}

HRESULT TestScene04::Update(const SceneDesc& desc, float elapsed_time)
{
	msaa_depth_draw = false;

	const float ptc_update_time = elapsed_time * time_scale;

	auto input = desc.input;

	if (input->IsKeyPressed(KGL::KEYS::ENTER) && input->IsKeyHold(KGL::KEYS::LCONTROL))
	{
		return Init(desc);
	}

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	const float camera_speed = input->IsKeyHold(KGL::KEYS::LSHIFT) ? 100.f : 50.f;
	camera->Update(desc.window, desc.input, elapsed_time, camera_speed, input->IsMouseHold(KGL::MOUSE_BUTTONS::right));
#if 0
	camera->GetPos() = { 0.f, 200.f, -10.f };
	camera->GetFront() = { 0.f, 0.f, 1.f };
#endif
	camera->GetPos().y = std::max(grid_pos.y + 1, camera->GetPos().y);
	const auto& view = camera->GetView();
	const auto& camera_pos = camera->GetPos();
	const auto& camera_front = camera->GetFront();

	if (desc.input->IsKeyPressed(KGL::KEYS::F1))
		use_gui = !use_gui;

	fc_mgr->Update(elapsed_time);
	if (use_gui)
	{
		ParticleParent pp{};
		{
			auto* p_pp = ptc_mgr->ParentResource()->Map(0, &CD3DX12_RANGE(0, 0));
			pp = *p_pp;
			ptc_mgr->ParentResource()->Unmap(0, &CD3DX12_RANGE(0, 0));
		}
		fc_mgr->ImGuiUpdate(desc.app->GetDevice(), &pp);

		if (ImGui::Begin("Info"))
		{
			ImGui::Text("FPS");
			ImGui::Text("%.2f", ImGui::GetIO().Framerate);
			ImGui::Text("Position");
			ImGui::Text("xyz[ %.2f, %.2f, %.2f ]", camera_pos.x, camera_pos.y, camera_pos.z);
			ImGui::Spacing();

			ImGui::Checkbox("Use DOF", &dof_flg);
			ImGui::Spacing();

			{
				std::string select_msaa = msaa_combo_texts[SCAST<UINT>(msaa_selector->GetScale())];

				// 2番目のパラメーターは、コンボを開く前にプレビューされるラベルです。
				if (ImGui::BeginCombo("MSAA", select_msaa.c_str()))
				{
					for (int n = 0; n < msaa_combo_texts.size(); n++)
					{
						// オブジェクトの外側または内側に、選択内容を好きなように保存できます
						bool is_selected = (select_msaa == msaa_combo_texts[n]);
						if (ImGui::Selectable(msaa_combo_texts[n].c_str(), is_selected))
							msaa_selector->SetScale(SCAST<MSAASelector::TYPE>(n));
						if (is_selected)
							// コンボを開くときに初期フォーカスを設定できます（キーボードナビゲーションサポートの場合は+をスクロールします）
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
			ImGui::Spacing();

			bool wire_mode = debug_mgr->GetWireMode();
			ImGui::Checkbox("WireMode", &wire_mode);
			debug_mgr->SetWireMode(wire_mode);
			ImGui::Spacing();

			if (ImGui::TreeNode("Grid"))
			{
				ImGui::SliderFloat3("pos", (float*)&grid_pos, -10.f, 10.f);

				if (ImGui::SliderFloat("length_min", (float*)&grid_buffer.mapped_data->length_min, 1.f, grid_buffer.mapped_data->length_max - 0.01f))
				{
					grid_buffer.mapped_data->length_max = std::max(grid_buffer.mapped_data->length_min + 0.01f, grid_buffer.mapped_data->length_max);
				}
				if (ImGui::SliderFloat("length_max", (float*)&grid_buffer.mapped_data->length_max, grid_buffer.mapped_data->length_min + 0.01f, 100.f))
				{
					grid_buffer.mapped_data->length_min = std::min(grid_buffer.mapped_data->length_min, grid_buffer.mapped_data->length_max - 0.01f);
				}
				ImGui::TreePop();
			}

			ImGui::Checkbox("RenderTargets Window", &rt_gui_windowed);
			if (!rt_gui_windowed)
			{
				if (ImGui::TreeNode("RenderTargets"))
				{
					UpdateRenderTargetGui(desc);
					ImGui::TreePop();
				}
			}

			ImGui::Checkbox("Sky Window", &sky_gui_windowed);
			if (!sky_gui_windowed)
			{
				if (ImGui::TreeNode("Sky"))
				{
					sky_mgr->UpdateGui();
					ImGui::TreePop();
				}
			}
		}
		ImGui::End();

		if (rt_gui_windowed)
		{
			if (ImGui::Begin("RenderTargets"))
			{
				UpdateRenderTargetGui(desc);
			}
			ImGui::End();
		}
		if (sky_gui_windowed)
		{
			if (ImGui::Begin("Sky"))
			{
				sky_mgr->UpdateGui();
			}
			ImGui::End();
		}
	}

	sky_mgr->Update(camera_pos, view * XMLoadFloat4x4(&proj));

	{
		using namespace DirectX;
		XMMATRIX W, S, R, T;
		S = XMMatrixScaling(1.f, 1.f, 1.f);
		R = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
		T = XMMatrixTranslation(camera_pos.x - fmodf(camera_pos.x, 1.f), grid_pos.y, camera_pos.z - fmodf(camera_pos.z, 1.f));
		W = S * R * T;
		XMMATRIX WVP = W * camera->GetView() * XMLoadFloat4x4(&proj);
		XMStoreFloat4x4(&grid_buffer.mapped_data->world, W);
		XMStoreFloat4x4(&grid_buffer.mapped_data->wvp, WVP);
	}

	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<TestScene03>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<TestScene00>(desc);

	if (input->IsKeyPressed(KGL::KEYS::BACKSPACE))
	{
		ptc_mgr->Clear();
		pl_shot_ptc_mgr->Clear();
		fireworks.clear();
	}
	if (use_gui)
	{
		if (ImGui::Begin("Particle"))
		{
			if (ImGui::Button("Clear"))
			{
				ptc_mgr->Clear();
				pl_shot_ptc_mgr->Clear();
				fireworks.clear();
			}
		}
		ImGui::End();
	}

	using namespace DirectX;
	KGL::Timer timer;
	auto particle_total_num = ptc_mgr->ResetCounter();
	auto pl_shot_particle_total_num = pl_shot_ptc_mgr->ResetCounter();
	{
		XMFLOAT4X4 viewf;
		XMStoreFloat4x4(&viewf, view);

		constexpr float center_speed = 1.f;
		/*if (input->IsKeyHold(KGL::KEYS::D))
		{
			cpt_scene_buffer.mapped_data->center_pos.x += center_speed * elapsed_time;
		}
		if (input->IsKeyHold(KGL::KEYS::A))
		{
			cpt_scene_buffer.mapped_data->center_pos.x -= center_speed * elapsed_time;
		}
		if (input->IsKeyHold(KGL::KEYS::W))
		{
			cpt_scene_buffer.mapped_data->center_pos.z += center_speed * elapsed_time;
		}
		if (input->IsKeyHold(KGL::KEYS::S))
		{
			cpt_scene_buffer.mapped_data->center_pos.z -= center_speed * elapsed_time;
		}
		if (input->IsKeyHold(KGL::KEYS::UP))
		{
			cpt_scene_buffer.mapped_data->center_pos.y += center_speed * elapsed_time;
		}
		if (input->IsKeyHold(KGL::KEYS::DOWN))
		{
			cpt_scene_buffer.mapped_data->center_pos.y -= center_speed * elapsed_time;
		}*/

		// 一秒秒間に[spawn_late]個のパーティクルを発生させる
		//constexpr UINT spawn_late = 2500;


		{
			auto* ptc_parent = ptc_mgr->ParentResource()->Map(0, &CD3DX12_RANGE(0, 0));
			ptc_parent->elapsed_time = ptc_update_time;
			ptc_mgr->ParentResource()->Unmap(0, &CD3DX12_RANGE(0, 0));
		}
		{
			auto* ptc_parent = pl_shot_ptc_mgr->ParentResource()->Map(0, &CD3DX12_RANGE(0, 0));
			ptc_parent->elapsed_time = ptc_update_time;
			pl_shot_ptc_mgr->ParentResource()->Unmap(0, &CD3DX12_RANGE(0, 0));
		}
		timer.Restart();
		if (use_gpu)
		{
			particle_pipeline->SetState(cpt_cmd_list);

			if (particle_total_num > 0)
				ptc_mgr->Dispatch(cpt_cmd_list);
			if (pl_shot_particle_total_num > 0)
				pl_shot_ptc_mgr->Dispatch(cpt_cmd_list);

			cpt_cmd_list->Close();
			//コマンドの実行
			ID3D12CommandList* cmd_lists[] = {
			   cpt_cmd_list.Get(),
			};

			cpt_cmd_queue->Data()->ExecuteCommandLists(1, cmd_lists);
			cpt_cmd_queue->Signal();
		}
		else
		{
			if (particle_total_num > 0)
				ptc_mgr->Update();
		}

		float key_spawn_late = 5.f;
		const float key_spawn_time = 1.f / key_spawn_late;
		if (input->IsKeyPressed(KGL::KEYS::NUMPADPLUS)) spawn_fireworks = !spawn_fireworks;
		if (spawn_fireworks)
			ptc_key_spawn_counter += ptc_update_time;
		else if (input->IsKeyPressed(KGL::KEYS::MINUS))
			ptc_key_spawn_counter += key_spawn_time;

		while (ptc_key_spawn_counter >= key_spawn_time)
		{
			FireworksDesc desc{};
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<UINT> rmd_int(0u, 100u);
			switch (rmd_int(mt))
			{
			case 0u: case 1u: case 2u: case 3u: case 4u:
			case 5u: case 6u: case 7u: case 8u: case 9u:
			{
				desc = FIREWORK_EFFECTS::Get(0);
				break;
			}
			case 10u: case 11u: case 12u: case 13u: case 14u:
			case 15u: case 16u: case 17u: case 18u: case 19u:
			{
				desc = FIREWORK_EFFECTS::Get(1);
				break;
			}
			case 20u: case 21u:
			{
				desc = FIREWORK_EFFECTS::Get(2);
				break;
			}
			default:
			{
				desc = FIREWORK_EFFECTS::Get(3);
				break;
			}
			}
			desc.pos = { 0.f, 0.f, 0.f };

			std::uniform_real_distribution<float> rmd_speed(0.9f, 1.f);
			desc.velocity = { 0.f, desc.speed * rmd_speed(mt), 0.f };

			std::uniform_real_distribution<float> rmdpos(-50.f, +50.f);
			desc.pos.x += rmdpos(mt);
			desc.pos.z += rmdpos(mt);

			XMFLOAT2 nmangle;
			nmangle.x = XMConvertToRadians(0.f) / XM_PI;
			nmangle.y = XMConvertToRadians(5.f) / XM_PI;
			std::uniform_real_distribution<float> rmdangle(nmangle.x, nmangle.y);
			std::uniform_real_distribution<float> rmdangle360(0.f, XM_2PI);
			constexpr float radian90f = XMConvertToRadians(90.f);
			XMVECTOR right_axis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
			float side_angle = asinf((2.f * rmdangle(mt)) - 1.f) + radian90f;
			XMVECTOR side_axis;
			XMVECTOR velocity = XMLoadFloat3(&desc.velocity);
			XMVECTOR axis = XMVector3Normalize(velocity);
			if (XMVector3Length(XMVectorSubtract(right_axis, axis)).m128_f32[0] <= FLT_EPSILON)
				side_axis = XMVector3Normalize(XMVector3Cross(axis, XMVectorSet(0.f, 1.f, 0.f, 0.f)));
			else
				side_axis = XMVector3Normalize(XMVector3Cross(axis, right_axis));
			XMMATRIX R = XMMatrixRotationAxis(side_axis, side_angle);
			R *= XMMatrixRotationAxis(axis, rmdangle360(mt));
			XMVECTOR spawn_v = XMVector3Transform(axis, R);
			XMStoreFloat3(&desc.velocity, spawn_v * XMVector3Length(velocity));

			fireworks.emplace_back(desc);
			ptc_key_spawn_counter -= key_spawn_time;
		}
		// プレイヤーショット
		if (input->IsMousePressed(KGL::MOUSE_BUTTONS::left))
		{
			auto desc = fc_mgr->GetSelectDesc();
			if (desc)
			{
				desc->pos = camera_pos;
				XMVECTOR xm_xmfront = XMLoadFloat3(&camera_front);
				XMStoreFloat3(&desc->velocity, XMVector3Normalize(xm_xmfront) * desc->speed);
				fireworks.emplace_back(*desc);
			}
		}

		scene_buffer.mapped_data->view = view;
		scene_buffer.mapped_data->proj = XMLoadFloat4x4(&proj);
		scene_buffer.mapped_data->eye = camera_pos;
		scene_buffer.mapped_data->light_vector = { 0.f, 0.f, 1.f };
		grid_buffer.mapped_data->eye_pos = camera_pos;
	}
	ParticleParent ptc_cb, pl_shot_ptc_cb;
	{
		const auto* cbp = ptc_mgr->ParentResource()->Map(0, &CD3DX12_RANGE(0, 0));
		ptc_cb = *cbp;
		ptc_mgr->ParentResource()->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
	{
		const auto* cbp = pl_shot_ptc_mgr->ParentResource()->Map(0, &CD3DX12_RANGE(0, 0));
		pl_shot_ptc_cb = *cbp;
		pl_shot_ptc_mgr->ParentResource()->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
	/*constexpr float spawn_elapsed = 1.f / spawn_late;
	spawn_counter += ptc_update_time;
	UINT spawn_num = 0u;
	if (spawn_counter >= spawn_elapsed)
	{
		float spawn_timer = spawn_counter - spawn_elapsed;
		spawn_num = KGL::SCAST<UINT>(spawn_counter / spawn_elapsed);
		spawn_counter = std::fmodf(spawn_counter, spawn_elapsed);

		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_real_distribution<float> score(-10.f, 10.f);
		std::uniform_real_distribution<float> unorm(0.f, 1.f);
		constexpr float rad_360min = XMConvertToRadians(360.f - FLT_MIN);

		for (int i = 0; i < spawn_num; i++)
		{
			auto random_vec =
				XMVector3Transform(
					XMVectorSet(0.f, 0.f, 1.f, 0.f),
					XMMatrixRotationRollPitchYaw(0, unorm(mt) * rad_360min, 0)
				);

			if (frame_particles.capacity() == frame_particles.size()) break;
			auto& particle = frame_particles.emplace_back();
			particle.exist_time = 5.f;
			particle.color = (i % 5 == 0) ? XMFLOAT4{ 1.f, 0.0f, 0.5f, 0.1f } : XMFLOAT4{ 1.0f, 0.5f, 0.0f, 0.1f };
			particle.position = { 0.f, 0.f, 0.f };
			particle.mass = 1.f;
			particle.scale_width = 0.2f;
			particle.scale_front = 0.1f;
			particle.scale_back = 0.1f;
			XMStoreFloat3(&particle.velocity, random_vec * 3.1);
			particle.Update(spawn_timer, cb);
			spawn_timer -= spawn_elapsed;
		}
	}*/
	UINT64 cputime = timer.GetTime(KGL::Timer::SEC::MICRO);
	timer.Restart();
	for (auto i = 0; i < fireworks.size(); i++)
	{
		if (!fireworks[i].Update(ptc_update_time, &ptc_mgr->frame_particles, &ptc_cb, &fireworks))
		{
			fireworks[i] = fireworks.back();
			fireworks.pop_back();
			i--;
		}
	}
	UINT64 firework_update = timer.GetTime(KGL::Timer::SEC::MICRO);

	timer.Restart();
	if (use_gpu)
	{
		cpt_cmd_queue->Wait();
		cpt_cmd_allocator->Reset();
		cpt_cmd_list->Reset(cpt_cmd_allocator.Get(), nullptr);
	}
	UINT64 gputime = timer.GetTime(KGL::Timer::SEC::MICRO);
	timer.Restart();

	if (particle_total_num > 0)
		ptc_mgr->Sort();
	if (pl_shot_particle_total_num > 0)
		pl_shot_ptc_mgr->Sort();

	const auto frame_ptc_size = ptc_mgr->frame_particles.size() + pl_shot_ptc_mgr->frame_particles.size();
	ptc_mgr->AddToFrameParticle();
	pl_shot_ptc_mgr->AddToFrameParticle();
	UINT64 map_update = timer.GetTime(KGL::Timer::SEC::MICRO);
	timer.Restart();
	cputime += firework_update + map_update;

	{
		const auto counter = ptc_mgr->Size() + pl_shot_ptc_mgr->Size();
		KGLDebugOutPutStringNL("\r particle : " + std::to_string(counter) + std::string(10, ' '));
		if (use_gui)
		{
			if (ImGui::Begin("Particle"))
			{
				ImGui::Checkbox("SpawnFireworks", &spawn_fireworks);
				bool use_gpu_log = use_gpu;
				if (ImGui::RadioButton("GPU", use_gpu)) use_gpu = true;
				ImGui::SameLine();
				if (ImGui::RadioButton("CPU", !use_gpu)) use_gpu = false;
				const bool reset_max_counter0 = use_gpu_log != use_gpu;
				const bool reset_max_counter1 = ImGui::Button("Reset Max Counter");
				ImGui::SliderFloat("Time Scale", &time_scale, 0.f, 2.f); ImGui::SameLine();
				ImGui::InputFloat("", &time_scale);
				ct_particle = std::max<UINT64>(ct_particle, counter);
				ImGui::Text("Particle Count Total [ %5d ] : [ %5d ]", counter, ct_particle);
				ct_frame_ptc = std::max<UINT64>(ct_frame_ptc, frame_ptc_size);
				ImGui::Text("Particle Count Frame [ %5d ] : [ %5d ]", frame_ptc_size, ct_frame_ptc);
				ct_fw = std::max<UINT64>(ct_fw, fireworks.size());
				ImGui::Text("Firework Count Total [ %5d ] : [ %5d ]", fireworks.size(), ct_fw);
				ct_gpu = std::max<UINT64>(ct_gpu, gputime);
				ImGui::Text("GPU Time             [ %5d ] : [ %5d ]", gputime, ct_gpu);
				ct_cpu = std::max<UINT64>(ct_cpu, cputime);
				ImGui::Text("CPU Time             [ %5d ] : [ %5d ]", cputime, ct_cpu);
				ct_fw_update = std::max<UINT64>(ct_fw_update, firework_update);
				ImGui::Text("Firework Update Time [ %5d ] : [ %5d ]", firework_update, ct_fw_update);
				ct_map_update = std::max<UINT64>(ct_map_update, map_update);
				ImGui::Text("Map Update Time      [ %5d ] : [ %5d ]", map_update, ct_map_update);

				ImGui::NextColumn();
				int bloom_scale = KGL::SCAST<int>(bloom_generator->GetKernel());
				if (ImGui::SliderInt("Bloom Scale", &bloom_scale, 0, KGL::SCAST<int>(BloomGenerator::RTV_MAX)))
				{
					bloom_generator->SetKernel(KGL::SCAST<UINT8>(bloom_scale));
				}
				if (ImGui::TreeNode("Weights"))
				{
					auto weights = bloom_generator->GetWeights();
					bool weights_changed = false;
					for (UINT i = 0u; i < BloomGenerator::RTV_MAX; i++)
					{
						bool changed = ImGui::SliderFloat(std::to_string(i).c_str(), &weights[i], 0.f, 1.f);
						weights_changed = weights_changed || changed;
					}
					if (weights_changed) bloom_generator->SetWeights(weights);
					ImGui::TreePop();
				}
				ImGui::Checkbox("After blooming", &after_blooming);

				if (reset_max_counter0 || reset_max_counter1) ResetCounterMax();
				{
					const auto imgui_window_size = ImGui::GetWindowSize();
					ImGui::BeginChild("scrolling", ImVec2(imgui_window_size.x * 0.9f, std::max<float>(std::min<float>(imgui_window_size.y - 100, 200.f), 0)), ImGuiWindowFlags_NoTitleBar);
					for (auto& it : b_tex_data)
					{
						const ImVec2 image_size = { 90, 90 };
						ImGui::Image((ImTextureID)it.imgui_handle.Gpu().ptr,
							image_size, ImVec2(0, 0), ImVec2(1, 1));
						ImGui::SameLine();
						const auto imgui_window_child_size = ImGui::GetWindowSize();
						std::string path = it.tex->GetPath().string();
						auto path_size = ImGui::CalcTextSize(path.c_str());
						const float string_draw_width = std::max(0.f, (imgui_window_child_size.x - image_size.x) - 20);
						UINT str_line_count = KGL::SCAST<UINT>(path_size.x / string_draw_width);
						UINT line_size = KGL::SCAST<UINT>(string_draw_width / (path_size.x / path.length()));
						for (UINT i = 0u; i < str_line_count; i++)
						{
							path.insert(i + ((i + 1) * line_size), "\n");
						}
						if (it.handle == b_select_srv_handle)
						{
							ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, path.c_str());
						}
						else if (ImGui::Button(path.c_str()))
						{
							b_select_srv_handle = it.handle;
						}
					}
					ImGui::EndChild();
				}
				ImGui::NewLine();
				//ImGui::
			}
			ImGui::End();
		}
	}

	{
		static float angle = 0.f;
		angle += DirectX::XMConvertToRadians(90.f) * elapsed_time;
		XMMATRIX W, S, R, T;
		S = XMMatrixScaling(1.f, 1.f, 1.f);
		R = XMMatrixRotationRollPitchYaw(0.f, angle, 0.f);
		T = XMMatrixTranslation(0.f, 0.f, -1.f);
		W = S * R * T;

		XMMATRIX WVP = W * view * XMLoadFloat4x4(&proj);
		CbvParam* param = matrix_resource->Map();
		XMStoreFloat4x4(&param->mat, WVP);
		param->color = { 1.f, 1.f, 1.f, 1.f };
		matrix_resource->Unmap();
	}

	{
		DebugManager::TransformConstants tc;
		XMStoreFloat4x4(&tc.view_projection, view * XMLoadFloat4x4(&proj));
		XMStoreFloat4x4(&tc.sky_projection, view * XMLoadFloat4x4(&proj));

		DebugManager::ShadingConstants sc;
		sc.eye_position = camera->GetPos();
		DebugManager::ShadingConstants::Light light{};
		sc.lights[0] = light;
		sc.lights[1] = light;
		sc.lights[2] = light;

		sc.lights[0].direction = { 0.5f, -1.f, -0.8f };
		XMStoreFloat3(&sc.lights[0].direction, XMVector3Normalize(XMLoadFloat3(&sc.lights[0].direction)));

		sc.lights[0].radiance = { 1.f, 1.f, 1.f };

		debug_mgr->Update(tc, sc, use_gui);
	}

	return Render(desc);
}

#if 0
HRESULT TestScene04::Render(const SceneDesc& desc)
{
	using KGL::SCAST;
	HRESULT hr = S_OK;

	auto window_size = desc.window->GetClientSize();

	D3D12_VIEWPORT viewport = {};
	viewport.Width = SCAST<FLOAT>(window_size.x);
	viewport.Height = SCAST<FLOAT>(window_size.y);
	viewport.TopLeftX = 0;//出力先の左上座標X
	viewport.TopLeftY = 0;//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;//深度最大値
	viewport.MinDepth = 0.0f;//深度最小値

	auto scissorrect = CD3DX12_RECT(
		0, 0,
		window_size.x, window_size.y
	);

	cmd_list->RSSetViewports(1, &viewport);
	cmd_list->RSSetScissorRects(1, &scissorrect);
	constexpr auto clear_value = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.f);

	{
		const UINT rtv_num = 0u;
		//cmd_list->ResourceBarrier(1u, &rtvs->GetRtvResourceBarrier(true, rtv_num));
		//const auto* dsv_handle = &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
		//rtvs->Set(cmd_list, dsv_handle, rtv_num);
		//rtvs->Clear(cmd_list, rtv_textures[rtv_num]->GetClearColor(), rtv_num);
		//desc.app->ClearDsv(cmd_list);

		//sky_mgr->Render(cmd_list);

		if (msaa_selector->GetScale() == MSAASelector::TYPE::MSAA_OFF)
		{
			cmd_list->ResourceBarrier(1u, &rtvs->GetRtvResourceBarrier(true, rtv_num));
			const auto* dsv_handle = &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
			rtvs->Set(cmd_list, dsv_handle, rtv_num);
			rtvs->Clear(cmd_list, rtv_textures[rtv_num]->GetClearColor(), rtv_num);
			desc.app->ClearDsv(cmd_list);
			sky_mgr->Render(cmd_list, msaa_selector->GetScale());
			debug_mgr->Render(cmd_list, msaa_selector->GetScale());
			cmd_list->ResourceBarrier(1u, &rtvs->GetRtvResourceBarrier(false, rtv_num));
		}
		else
		{
			const UINT msaa_rtv_num = msaa_selector->GetScale() - 1u;
			const auto& msaa_tex = msaa_textures[msaa_rtv_num];

			{	// MSAA用RTをRT用にバリア
				D3D12_RESOURCE_BARRIER barriers[] =
				{
					CD3DX12_RESOURCE_BARRIER::Transition(
						msaa_tex.render_target->Data().Get(),
						D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
						D3D12_RESOURCE_STATE_RENDER_TARGET)
				};
				cmd_list->ResourceBarrier(SCAST<UINT>(std::size(barriers)), barriers);
			}

			msaa_rtvs->Set(cmd_list, &msaa_tex.dsv_handle.Cpu(), msaa_rtv_num);
			desc.app->ClearDsv(cmd_list);
			msaa_rtvs->Clear(cmd_list, msaa_tex.render_target->GetClearColor(), msaa_rtv_num);
			cmd_list->ClearDepthStencilView(msaa_tex.dsv_handle.Cpu(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

			sky_mgr->Render(cmd_list, msaa_selector->GetScale());
			debug_mgr->Render(cmd_list, msaa_selector->GetScale());

			{	// MSAA用RTを元リソース用にバリア / RTVSのRTを宛先リソース用にバリア
				D3D12_RESOURCE_BARRIER barriers[] =
				{
					CD3DX12_RESOURCE_BARRIER::Transition(
						msaa_tex.render_target->Data().Get(),
						D3D12_RESOURCE_STATE_RENDER_TARGET,
						D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
					CD3DX12_RESOURCE_BARRIER::Transition(
						rtvs->GetResources()[rtv_num].Get(),
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						D3D12_RESOURCE_STATE_RESOLVE_DEST)
				};
				cmd_list->ResourceBarrier(SCAST<UINT>(std::size(barriers)), barriers);
			}

			// コピー
			cmd_list->ResolveSubresource(
				rtvs->GetResources()[rtv_num].Get(), 0,
				msaa_tex.render_target->Data().Get(), 0,
				msaa_tex.render_target->Data()->GetDesc().Format
			);

			{
				D3D12_RESOURCE_BARRIER barriers[] =
				{	// RTVSのRTをシェーダーリソース用にバリア
					CD3DX12_RESOURCE_BARRIER::Transition(
						rtvs->GetResources()[rtv_num].Get(),
						D3D12_RESOURCE_STATE_RESOLVE_DEST,
						D3D12_RESOURCE_STATE_RENDER_TARGET),
					// Depthをピクセルシェーダー用にバリア
					CD3DX12_RESOURCE_BARRIER::Transition(
						msaa_tex.depth_stencil->Data().Get(),
						D3D12_RESOURCE_STATE_DEPTH_WRITE,
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
				};
				cmd_list->ResourceBarrier(SCAST<UINT>(std::size(barriers)), barriers);
			}

			if (msaa_depth_draw)
			{
				// 深度値をコピーする
				desc.app->SetDsv(cmd_list);
				cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				depth_sprite_renderer->SetState(cmd_list);
				cmd_list->SetDescriptorHeaps(1, depth_srv_desc_mgr->Heap().GetAddressOf());
				cmd_list->SetGraphicsRootDescriptorTable(0, msaa_tex.depth_srv_handle.Gpu());
				sprite->Render(cmd_list);
			}
			
			{
				D3D12_RESOURCE_BARRIER barriers[] =
				{	// DepthをDSV用にバリア
					CD3DX12_RESOURCE_BARRIER::Transition(
						msaa_tex.depth_stencil->Data().Get(),
						D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						D3D12_RESOURCE_STATE_DEPTH_WRITE)
				};
				cmd_list->ResourceBarrier(SCAST<UINT>(std::size(barriers)), barriers);
			}
			const auto* dsv_handle = &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
			rtvs->Set(cmd_list, dsv_handle, rtv_num);

			//sky_mgr->Render(cmd_list);

			cmd_list->ResourceBarrier(1u, &rtvs->GetRtvResourceBarrier(false, rtv_num));
		}
	}
	{
		const auto& rtrbs = ptc_rtvs->GetRtvResourceBarriers(true);
		const UINT rtrbs_size = rtrbs.size();
		cmd_list->ResourceBarrier(rtrbs_size, rtrbs.data());

		const auto* dsv_handle = &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
		ptc_rtvs->SetAll(cmd_list, dsv_handle);
		
		for (UINT idx = 0u; idx < rtrbs_size; idx++)
		{
			ptc_rtvs->Clear(cmd_list, ptc_rtv_texs[idx]->GetClearColor(), idx);
		}

		// パーティクル描画
		{
			cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
			board_renderer->SetState(cmd_list);
			cmd_list->SetDescriptorHeaps(1, scene_buffer.handle.Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(0, scene_buffer.handle.Gpu());
			//cmd_list->SetDescriptorHeaps(1, b_cbv.Heap().GetAddressOf());
			//cmd_list->SetGraphicsRootDescriptorTable(1, b_cbv.Gpu());
			if (b_select_srv_handle)
			{
				cmd_list->SetDescriptorHeaps(1, b_select_srv_handle->Heap().GetAddressOf());
				cmd_list->SetGraphicsRootDescriptorTable(2, b_select_srv_handle->Gpu());
			}

			const auto ptc_size = ptc_mgr->Size();
			if (ptc_size > 0)
			{
				cmd_list->IASetVertexBuffers(0, 1, &b_ptc_vbv);
				cmd_list->DrawInstanced(SCAST<UINT>(ptc_size), 1, 0, 0);
			}

			board_renderer_pos->SetState(cmd_list);
			fc_mgr->Render(cmd_list);

			if (dof_flg)
			{
				board_renderer_dsv->SetState(cmd_list);
				cmd_list->SetDescriptorHeaps(1, scene_buffer.handle.Heap().GetAddressOf());
				cmd_list->SetGraphicsRootDescriptorTable(0, scene_buffer.handle.Gpu());
				if (b_select_srv_handle)
				{
					cmd_list->SetDescriptorHeaps(1, b_select_srv_handle->Heap().GetAddressOf());
					cmd_list->SetGraphicsRootDescriptorTable(2, b_select_srv_handle->Gpu());
				}
				if (ptc_size > 0)
				{
					cmd_list->IASetVertexBuffers(0, 1, &b_ptc_vbv);
					cmd_list->DrawInstanced(SCAST<UINT>(ptc_size), 1, 0, 0);
				}

				board_renderer_dsv_pos->SetState(cmd_list);
				fc_mgr->Render(cmd_list);
			}
		}

		const auto& srvrbs = ptc_rtvs->GetRtvResourceBarriers(false);
		cmd_list->ResourceBarrier(srvrbs.size(), srvrbs.data());
	}
	/*{
		const UINT rtv_num = 1u;
		cmd_list->ResourceBarrier(1u, &ptc_rtvs->GetRtvResourceBarrier(true, rtv_num));

		ptc_rtvs->Set(cmd_list, nullptr, rtv_num);
		ptc_rtvs->Clear(cmd_list, ptc_rtv_texs[rtv_num]->GetClearColor(), rtv_num);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		high_sprite_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, ptc_rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, ptc_rtvs->GetSRVGPUHandle(0));
		sprite->Render(cmd_list);

		cmd_list->ResourceBarrier(1u, &ptc_rtvs->GetRtvResourceBarrier(false, rtv_num));
	}*/
	{
		bloom_generator->Generate(cmd_list, ptc_rtvs->GetSRVHeap(), ptc_rtvs->GetSRVGPUHandle(1), viewport);
	}
	{
		const UINT rtv_num = 1u;
		cmd_list->ResourceBarrier(1u, &rtvs->GetRtvResourceBarrier(true, rtv_num));
		rtvs->Set(cmd_list, nullptr, rtv_num);
		rtvs->Clear(cmd_list, rtv_textures[rtv_num]->GetClearColor(), rtv_num);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		sprite_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(0));
		sprite->Render(cmd_list);


		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		add_sprite_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, ptc_rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, ptc_rtvs->GetSRVGPUHandle(0));
		sprite->Render(cmd_list);
		cmd_list->SetDescriptorHeaps(1, ptc_rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, ptc_rtvs->GetSRVGPUHandle(1));
		sprite->Render(cmd_list);

		if (!after_blooming)
		{
			bloom_generator->Render(cmd_list);
		}

		cmd_list->ResourceBarrier(1u, &rtvs->GetRtvResourceBarrier(false, rtv_num));

		if (dof_flg)
			dof_generator->Generate(cmd_list, rtvs->GetSRVHeap(), rtvs->GetSRVGPUHandle(rtv_num), viewport);
	}
	{
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));

		desc.app->SetRtv(cmd_list);
		desc.app->ClearRtv(cmd_list, DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f));

		//cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		//sprite_renderer->SetState(cmd_list);
		//cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
		//cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(1));
		//sprite->Render(cmd_list);

		if (dof_flg)
		{
			const auto* dsv_handle = &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
			dof_generator->Render(cmd_list, depth_srv_handle.Heap(), depth_srv_handle.Gpu());
		}
		else
		{
			sprite_renderer->SetState(cmd_list);
			cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(1));
			sprite->Render(cmd_list);
		}
		
		if (after_blooming)
		{
			bloom_generator->Render(cmd_list);
		}

		{
			desc.app->SetRtvDsv(cmd_list);

			// グリッド描画
			cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
			grid_renderer->SetState(cmd_list);
			cmd_list->SetDescriptorHeaps(1, grid_buffer.handle.Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(0, grid_buffer.handle.Gpu());
			cmd_list->IASetVertexBuffers(0, 1, &grid_vbv);
			cmd_list->IASetIndexBuffer(&grid_ibv);
			cmd_list->DrawIndexedInstanced(grid_idx_resource->Size(), 1, 0, 0, 0);

			desc.app->SetRtv(cmd_list);
		}
		ImGui::Render();
		cmd_list->SetDescriptorHeaps(1, desc.imgui_handle.Heap().GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list.Get());

		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));
	}

	cmd_list->Close();
	ID3D12CommandList* cmd_lists[] = { cmd_list.Get() };
	desc.app->GetQueue()->Data()->ExecuteCommandLists(1, cmd_lists);
	desc.app->GetQueue()->Signal();
	desc.app->GetQueue()->Wait();

	cmd_allocator->Reset();
	cmd_list->Reset(cmd_allocator.Get(), nullptr);

	if (desc.app->IsTearingSupport())
		desc.app->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	else
		desc.app->GetSwapchain()->Present(1, 1);

	return hr;
}
#else
HRESULT TestScene04::Render(const SceneDesc& desc)
{
	using KGL::SCAST;
	HRESULT hr = S_OK;

	auto window_size = desc.window->GetClientSize();
	const DirectX::XMFLOAT2 window_sizef = { SCAST<FLOAT>(window_size.x), SCAST<FLOAT>(window_size.y) };
	
	// ビューとシザーをセット
	D3D12_VIEWPORT viewport = 
		CD3DX12_VIEWPORT(window_sizef.x, window_sizef.y, 0.f, 0.f);
	auto scissorrect = 
		CD3DX12_RECT(0, 0, window_size.x, window_size.y);
	cmd_list->RSSetViewports(1, &viewport);
	cmd_list->RSSetScissorRects(1, &scissorrect);

	// MSAA識別用
	const UINT msaa_scale = msaa_selector->GetScale();
	const bool msaa = msaa_scale != 0u;

	{	// 0番目のテクスチャをRTに
		const UINT rtv_num = 0u;
	}
}
#endif
HRESULT TestScene04::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	sky_mgr->Uninit(desc.imgui_heap_mgr);

	for (auto& it : b_tex_data)
	{
		desc.imgui_heap_mgr->Free(it.imgui_handle);
	}

	for (const auto& handle : ptc_srv_gui_handles)
	{
		desc.imgui_heap_mgr->Free(handle);
	}

	return S_OK;
}