#include "../../Hrd/Scenes/Scenes.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>
#include <random>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include <algorithm>

HRESULT TestScene04::PrepareRTAndDS(const SceneDesc& desc, DXGI_SAMPLE_DESC sample_desc)
{
	HRESULT hr = S_OK;
	auto device = desc.app->GetDevice();
	auto& rtrc = rt_resources.emplace_back();

	// レンダーターゲット
	constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	D3D12_RESOURCE_DESC res_desc = desc.app->GetRtvBuffers().at(0)->GetDesc();
	res_desc.SampleDesc = sample_desc;

	// NON_BLOOM
	rtrc.render_targets.push_back({ std::make_shared<KGL::Texture>(
		device, res_desc, CD3DX12_CLEAR_VALUE(res_desc.Format, (float*)&clear_value)) });
	// BLOOM
	rtrc.render_targets.push_back({ std::make_shared<KGL::Texture>(
		device, res_desc, CD3DX12_CLEAR_VALUE(res_desc.Format, (float*)&clear_value)) });
	// PTC
	res_desc.MipLevels = sample_desc.Count == 1 ? 8u : 1u;
	rtrc.render_targets.push_back({ std::make_shared<KGL::Texture>(
		device, res_desc, CD3DX12_CLEAR_VALUE(res_desc.Format, (float*)&clear_value)) });
	rtrc.render_targets.push_back({ std::make_shared<KGL::Texture>(
		device, res_desc, CD3DX12_CLEAR_VALUE(res_desc.Format, (float*)&clear_value)) });
	// FXAA_GRAY
	constexpr auto non_alpha_clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	rtrc.render_targets.push_back({ std::make_shared<KGL::Texture>(
		device, res_desc, CD3DX12_CLEAR_VALUE(res_desc.Format, (float*)&non_alpha_clear_value)) });
	std::vector<ComPtr<ID3D12Resource>> resources;
	resources.reserve(rtrc.render_targets.size());
	for (auto& it : rtrc.render_targets) resources.push_back(it.tex->Data());
	rtrc.rtvs = std::make_shared<KGL::RenderTargetView>(device, resources, nullptr,
		sample_desc.Count == 1 ? D3D12_SRV_DIMENSION_TEXTURE2D : D3D12_SRV_DIMENSION_TEXTURE2DMS);

	// MSAAの場合のみ深度テクスチャを新たに作成しDSVを準備
	if (sample_desc.Count > 1u)
	{
		rtrc.dsv_handle = depth_dsv_descriptor->Alloc();

		D3D12_CLEAR_VALUE depth_clear_value = {};
		depth_clear_value.DepthStencil.Depth = 1.0f;		// 深さの最大値でクリア
		depth_clear_value.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;	// 32ビットfloat値としてクリア
		auto dsv_rs_desc = desc.app->GetDsvBuffer()->GetDesc();
		dsv_rs_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		dsv_rs_desc.SampleDesc = sample_desc;
		dsv_rs_desc.MipLevels = 1;
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;	// フラグ無し

		rtrc.depth_stencil = std::make_shared<KGL::Texture>(device,
			dsv_rs_desc,
			depth_clear_value,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
			);

		device->CreateDepthStencilView(rtrc.depth_stencil->Data().Get(), &dsv_desc, rtrc.dsv_handle.Cpu());

		// Combo用text
		msaa_combo_texts.push_back("x" + std::to_string(sample_desc.Count));
	}
	else
	{
		// Combo用text
		msaa_combo_texts.push_back("OFF");
	}

	// 深度用のSRV作成 (+ IMGUI用SRV)
	rtrc.depth_srv_handle = depth_srv_descriptor->Alloc();
	rtrc.depth_gui_srv_handle = desc.imgui_heap_mgr->Alloc();
	{
		ComPtr<ID3D12Resource> dsv_resource;
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_rs_desc = {};
		srv_rs_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		srv_rs_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		if (sample_desc.Count == 1u)
		{
			dsv_resource = desc.app->GetDsvBuffer();
			srv_rs_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srv_rs_desc.Texture2D.MipLevels = desc.app->GetDsvBuffer()->GetDesc().MipLevels;
		}
		else
		{
			dsv_resource = rtrc.depth_stencil->Data();
			srv_rs_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		}
		device->CreateShaderResourceView(
			dsv_resource.Get(),
			&srv_rs_desc,
			rtrc.depth_srv_handle.Cpu()
		);
		device->CreateShaderResourceView(
			dsv_resource.Get(),
			&srv_rs_desc,
			rtrc.depth_gui_srv_handle.Cpu()
		);
	}

	// RTのIMGUI用SRV作成
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		
		for (auto& rt : rtrc.render_targets)
		{
			rt.gui_srv_handle = desc.imgui_heap_mgr->Alloc();
			const auto& texture_desc = rt.tex->Data()->GetDesc();

			if (sample_desc.Count == 1u)	// 非MSAA用
			{
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srv_desc.Texture2D.MipLevels = SCAST<UINT>(texture_desc.MipLevels);
			}
			else							// MSAA用
			{
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
			}
			srv_desc.Format = texture_desc.Format;
			device->CreateShaderResourceView(rt.tex->Data().Get(), &srv_desc, rt.gui_srv_handle.Cpu());
		}
	}

	return hr;
}

HRESULT TestScene04::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();
	DXGI_SAMPLE_DESC max_sample_desc = { desc.app->GetMaxSampleCount(), desc.app->GetMaxQualityLevel() };

	msaa_selector = std::make_shared<MSAASelector>(desc.app->GetMaxSampleCount());
	const UINT msaa_type_count = SCAST<UINT>(msaa_selector->GetMaxScale()) + 1u;
	// コンピュート用・描画用のコマンドアロケーター・コマンドリストを初期化
	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);
	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &fast_cmd_allocator, &fast_cmd_list);
	RCHECK(FAILED(hr), "コマンドリストの作成に失敗", hr);
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

	ptc_tex_mgr = std::make_shared<ParticleTextureManager>(device, "./Assets/Textures/Particles");
	ptc_tex_mgr->CreateSRV(device, &ptc_tex_srv_gui_handles, desc.imgui_heap_mgr);
	ptc_mgr = std::make_shared<ParticleManager>(device, 1000000u);
	pl_shot_ptc_mgr = std::make_shared<ParticleManager>(device, 100000u);

	particle_pipeline = std::make_shared<KGL::ComputePipline>(device, desc.dxc);
	b_cbv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	matrix_resource = std::make_shared<KGL::Resource<CbvParam>>(device, 1u);


	{	// スプライト用PSOの作成(半透明、加算、高輝度抽出)
		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
		sprite_renderers.emplace_back(std::make_shared<KGL::_2D::Renderer>(device, desc.dxc, renderer_desc));
		renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
		for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
		{
			renderer_desc.other_desc.sample_desc.Count *= 2;
			sprite_renderers.emplace_back(std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc));
		}
		renderer_desc.other_desc.sample_desc.Count = 1u;
		renderer_desc.rastarizer_desc.MultisampleEnable = FALSE;

		renderer_desc.blend_types[0] = KGL::BDTYPE::ADD;
		add_sprite_renderers.reserve(msaa_type_count);
		add_sprite_renderers.push_back(std::make_shared<KGL::_2D::Renderer>(device, desc.dxc, renderer_desc));
		renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
		renderer_desc.ps_desc.entry_point = "PSMainMS";
		for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
		{
			renderer_desc.other_desc.sample_desc.Count *= 2;
			add_sprite_renderers.push_back(std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc));
		}
		renderer_desc.rastarizer_desc.MultisampleEnable = FALSE;
		renderer_desc.other_desc.sample_desc.Count = 1u;
		renderer_desc.ps_desc.entry_point = "PSMain";

		renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
		sprite = std::make_shared<KGL::Sprite>(device);

		renderer_desc.render_targets.clear();
		renderer_desc.depth_desc.DepthEnable = TRUE;
		renderer_desc.depth_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		renderer_desc.depth_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/DepthWriteTextureMS_ps.hlsl";
		depth_sprite_renderer = std::make_shared<KGL::BaseRenderer>(device, desc.dxc, renderer_desc);
	}

	board_renderers.resize(3u);
	UINT count = 0u;
	for (auto& bd_rndrs : board_renderers)
	{	// パーティクル用PSOを作成(描画用)
		bd_rndrs.resize(msaa_selector->GetMaxScale() + 1u);
		auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;
		renderer_desc.static_samplers[0].MaxLOD = 1.f;
		renderer_desc.vs_desc.hlsl = "./HLSL/3D/Particle_vs.hlsl";
		renderer_desc.gs_desc.hlsl = "./HLSL/3D/" + PTC_VT_TABLE.at(SCAST<PTC_VT>(count));
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
		renderer_desc.input_layouts.push_back({
			"MOVE_LENGTH", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"TEXTURE_NUM", 0, DXGI_FORMAT_R32_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE_FRONT_MAX", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE_BACK_MAX", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.other_desc.topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		renderer_desc.render_targets.clear();
		renderer_desc.render_targets.reserve(2u);
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/ParticleDepth_ps.hlsl";
		bd_rndrs[MSAASelector::TYPE::MSAA_OFF].dsv
			= std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
		renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
		UINT idx = 1u;
		for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
		{
			renderer_desc.other_desc.sample_desc.Count *= 2;
			bd_rndrs[idx++].dsv =
				std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
		}
		renderer_desc.rastarizer_desc.MultisampleEnable = FALSE;
		renderer_desc.other_desc.sample_desc.Count = 1u;
		renderer_desc.vs_desc.hlsl = "./HLSL/3D/ParticlePos_vs.hlsl";
		bd_rndrs[MSAASelector::TYPE::MSAA_OFF].dsv_add_pos =
			std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
		renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
		idx = 1u;
		for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
		{
			renderer_desc.other_desc.sample_desc.Count *= 2;
			bd_rndrs[idx++].dsv_add_pos =
				std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
		}
		renderer_desc.rastarizer_desc.MultisampleEnable = FALSE;
		renderer_desc.other_desc.sample_desc.Count = 1u;


		renderer_desc.vs_desc.hlsl = "./HLSL/3D/Particle_vs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/Particle_ps.hlsl";
		renderer_desc.blend_types[0] = KGL::BLEND::TYPE::ADD;
		renderer_desc.blend_types[1] = KGL::BLEND::TYPE::ADD;
		renderer_desc.depth_desc = {};
		renderer_desc.render_targets.push_back(DXGI_FORMAT_R8G8B8A8_UNORM);
		renderer_desc.render_targets.push_back(DXGI_FORMAT_R8G8B8A8_UNORM);
		bd_rndrs[MSAASelector::TYPE::MSAA_OFF].simple =
			std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
		renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		bd_rndrs[MSAASelector::TYPE::MSAA_OFF].simple_wire =
			std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
		renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
		renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
		idx = 1u;
		for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
		{
			renderer_desc.other_desc.sample_desc.Count *= 2;
			bd_rndrs[idx].simple =
				std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
			renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
			bd_rndrs[idx++].simple_wire =
				std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
			renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
		}
		renderer_desc.rastarizer_desc.MultisampleEnable = FALSE;
		renderer_desc.other_desc.sample_desc.Count = 1u;

		renderer_desc.vs_desc.hlsl = "./HLSL/3D/ParticlePos_vs.hlsl";

		bd_rndrs[MSAASelector::TYPE::MSAA_OFF].add_pos =
			std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
		renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		bd_rndrs[MSAASelector::TYPE::MSAA_OFF].add_pos_wire =
			std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
		renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
		renderer_desc.rastarizer_desc.MultisampleEnable = TRUE;
		idx = 1u;
		for (; renderer_desc.other_desc.sample_desc.Count < max_sample_desc.Count;)
		{
			renderer_desc.other_desc.sample_desc.Count *= 2;
			bd_rndrs[idx].add_pos =
				std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
			renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_WIREFRAME;
			bd_rndrs[idx++].add_pos_wire =
				std::make_shared<KGL::_3D::Renderer>(device, desc.dxc, renderer_desc);
			renderer_desc.rastarizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
		}
		count++;
	}
	board = std::make_shared<KGL::Board>(device);

	hr = scene_buffer.Load(desc);
	RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);

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
		grid_vbv.SizeInBytes = SCAST<UINT>(sizeof(grid_vertices[0]) * grid_vertices.size());
		grid_vbv.StrideInBytes = sizeof(grid_vertices[0]);

		grid_ibv.BufferLocation = grid_idx_resource->Data()->GetGPUVirtualAddress();
		grid_ibv.SizeInBytes = SCAST<UINT>(sizeof(grid_indices[0]) * grid_indices.size());
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
	{
		b_ptc_vbv.BufferLocation = ptc_mgr->Resource()->Data()->GetGPUVirtualAddress();
		b_ptc_vbv.SizeInBytes = SCAST<UINT>(ptc_mgr->Resource()->SizeInBytes());
		b_ptc_vbv.StrideInBytes = sizeof(Particle);

		b_pl_shot_ptc_vbv.BufferLocation = pl_shot_ptc_mgr->Resource()->Data()->GetGPUVirtualAddress();
		b_pl_shot_ptc_vbv.SizeInBytes = SCAST<UINT>(pl_shot_ptc_mgr->Resource()->SizeInBytes());
		b_pl_shot_ptc_vbv.StrideInBytes = sizeof(Particle);
	}

	fireworks.reserve(10000u);
	player_fireworks.reserve(10000u);

	
	sky_mgr = std::make_shared<SkyManager>(device, desc.dxc, desc.imgui_heap_mgr, max_sample_desc);

	{
		bloom_generator = std::make_shared<BloomGenerator>(device, desc.dxc, desc.app->GetRtvBuffers().at(0));
		
		const auto& tex_c = bloom_generator->GetTexturesC();
		const auto& tex_w = bloom_generator->GetTexturesW();
		const auto& tex_h = bloom_generator->GetTexturesH();
		const auto& tex = bloom_generator->GetTexture();

		UINT idx = 0u;
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		for (auto& imgui_handle : bl_c_imgui_handles)
		{
			imgui_handle = desc.imgui_heap_mgr->Alloc();
			srv_desc.Format = tex_c[idx]->Data()->GetDesc().Format;
			device->CreateShaderResourceView(tex_c[idx]->Data().Get(), &srv_desc, imgui_handle.Cpu());
			idx++;
		}
		idx = 0u;
		for (auto& imgui_handle : bl_w_imgui_handles)
		{
			imgui_handle = desc.imgui_heap_mgr->Alloc();
			srv_desc.Format = tex_w[idx]->Data()->GetDesc().Format;
			device->CreateShaderResourceView(tex_w[idx]->Data().Get(), &srv_desc, imgui_handle.Cpu());
			idx++;
		}
		idx = 0u;
		for (auto& imgui_handle : bl_h_imgui_handles)
		{
			imgui_handle = desc.imgui_heap_mgr->Alloc();
			srv_desc.Format = tex_h[idx]->Data()->GetDesc().Format;
			device->CreateShaderResourceView(tex_h[idx]->Data().Get(), &srv_desc, imgui_handle.Cpu());
			idx++;
		}

		bl_bloom_imgui_handle = desc.imgui_heap_mgr->Alloc();
		srv_desc.Format = tex->Data()->GetDesc().Format;
		device->CreateShaderResourceView(tex->Data().Get(), &srv_desc, bl_bloom_imgui_handle.Cpu());
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

	fc_mgr = std::make_shared<FCManager>("./Assets/Effects/Fireworks/", ptc_tex_mgr->GetTextures());
	fs_mgr = std::make_shared<FSManager>("./Assets/Effects/Spawners/", fc_mgr->GetDescList());

	debug_mgr = std::make_shared<DebugManager>(device, desc.dxc, max_sample_desc);

	// 深度DSV用
	depth_dsv_descriptor = 
		std::make_shared<KGL::DescriptorManager>(
			device, SCAST<UINT>(msaa_selector->GetMaxScale()),
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		);
	// 深度SRV用
	depth_srv_descriptor = std::make_shared<KGL::DescriptorManager>(device, SCAST<UINT>(msaa_selector->GetMaxScale() + 1u));

	// 対応可能なMSAA分のRT/DSを作成し、それぞれのDSV/SRVを準備
	DXGI_SAMPLE_DESC sample_desc = {};
	sample_desc.Count = 1u;
	//sample_desc.Quality = desc.app->GetMaxQualityLevel();
	while (sample_desc.Count <= desc.app->GetMaxSampleCount())
	{
		PrepareRTAndDS(desc, sample_desc);
		sample_desc.Count *= 2;
	}

	fxaa_mgr = std::make_shared<FXAAManager>(device, desc.dxc, desc.app->GetResolution());

	ptc_tex_mgr->LoadWait();

	return hr;
}

HRESULT TestScene04::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();
	auto resolution = desc.app->GetResolution();

	dof_generator->SetRtvNum(3u);

	camera = std::make_shared<DemoCamera>(XMFLOAT3(0.f, 200.f, 0.f), XMFLOAT3(0.f, 200.f, -100.f), 30000.f);

	//XMStoreFloat3(&scene_buffer.mapped_data->light_vector, XMVector3Normalize(XMVectorSet(+0.2f, -0.7f, 0.5f, 0.f)));

	const XMMATRIX proj_mat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(resolution.x) / static_cast<float>(resolution.y),	// アスペクト比
		0.1f, 1000.0f // near, far
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
	//ptc_parent.center_pos = { 0.f, -6378.1f * 1000.f, 0.f };
	ptc_parent.affect_obj_count = 0.f;
	//ptc_parent.center_mass = 5.9724e24f;
	ptc_parent.resistivity = 1.f;
	ptc_mgr->SetParent(ptc_parent);
	ptc_mgr->SetAffectObjects(fc_mgr->affect_objects, player_fireworks);
	pl_shot_ptc_mgr->SetParent(ptc_parent);
	pl_shot_ptc_mgr->SetAffectObjects(fc_mgr->affect_objects, {});
	fc_mgr->CreateSelectDemo(desc.app->GetDevice(), &ptc_parent);

	spawn_counter = 0.f;
	ptc_key_spawn_counter = 0.f;

	camera_angle = { 0.f, 0.f };

	sky_mgr->Init(view * proj_mat);

	use_gui = true;
	stop_time = false;

	ResetCounterMax();
	time_scale = 1.f;
	use_gpu = true;
	spawn_fireworks = false;

	dof_flg = true;
	ptc_dof_flg = false;

	bloom_generator->SetKernel(8u);
	after_blooming = false;

	rt_gui_windowed = false;
	sky_gui_windowed = false;

	sky_draw = true;
	particle_wire = false;

	ptc_vt_type = PTC_VT::COUNT4;

	{	// キューブを簡易描画するマネージャー(キューブ以外のものも対応予定)
		debug_mgr->ClearStaticObjects();
		std::vector<std::shared_ptr<DebugManager::Object>> objects;
		std::shared_ptr<DebugManager::Cube> target_cube = std::make_shared<DebugManager::Cube>();
		auto loop_pos = target_cube->pos = camera->center;
		target_cube->color = { 1.f, 1.f, 1.f, 1.f };
		target_cube->scale = { 1.f, 1.f, 1.f };
		target_cube->rotate = {};
#if 1
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
	ct_fw_max = 0u;
	ct_ptc_total_max = 0u;
	ct_ptc_frame_max = 0u;

	tm_update.Clear();
	tm_render.Clear();
	tm_ptc_update_gpu.Clear();
	tm_ptc_update_cpu.Clear();
	tm_ptc_sort.Clear();
}

#define BORDER_COLOR(color) ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(1.f, 1.f, 1.f, 1.f), color
void TestScene04::UpdateRenderTargetGui(const SceneDesc& desc)
{
	const auto bd_color = ImGui::GetStyle().Colors[ImGuiCol_Border];
	auto gui_size = ImGui::GetWindowSize();
	ImVec2 image_size = { gui_size.x * 0.8f, gui_size.y * 0.8f };
	if (ImGui::TreeNode("DSV"))
	{
		//ImGui::GetWindowDrawList()->AddImage((ImTextureID)it.imgui_handle.Gpu().ptr,
		// ImVec2(0, 0), image_size, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 0, 0, 255));
		
		ImGui::Image((ImTextureID)rt_resources[MSAASelector::TYPE::MSAA_OFF].depth_gui_srv_handle.Gpu().ptr, image_size, BORDER_COLOR(bd_color));
		msaa_depth_draw = true;
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Particles"))
	{
		ImGui::Image((ImTextureID)rt_resources[MSAASelector::TYPE::MSAA_OFF].render_targets[PTC_NON_BLOOM].gui_srv_handle.Gpu().ptr, image_size, BORDER_COLOR(bd_color));
		ImGui::Image((ImTextureID)rt_resources[MSAASelector::TYPE::MSAA_OFF].render_targets[PTC_BLOOM].gui_srv_handle.Gpu().ptr, image_size, BORDER_COLOR(bd_color));
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Bloom"))
	{
		ImVec2 halh_image_size = { image_size.x * 0.5f, image_size.y * 0.5f };
		UINT idx = 0u;
		ImGui::Text("Compression");
		for (const auto& handle : bl_c_imgui_handles)
		{
			ImGui::Image((ImTextureID)handle.Gpu().ptr, halh_image_size, BORDER_COLOR(bd_color));
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		idx = 0u;
		ImGui::Text("Width Blur");
		for (const auto& handle : bl_h_imgui_handles)
		{
			ImGui::Image((ImTextureID)handle.Gpu().ptr, halh_image_size, BORDER_COLOR(bd_color));
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		idx = 0u;
		ImGui::Text("Height Blur");
		for (const auto& handle : bl_w_imgui_handles)
		{
			ImGui::Image((ImTextureID)handle.Gpu().ptr, halh_image_size, BORDER_COLOR(bd_color));
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		ImGui::Text("Result");
		ImGui::Image((ImTextureID)bl_bloom_imgui_handle.Gpu().ptr, halh_image_size, BORDER_COLOR(bd_color));
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("DOF"))
	{
		ImVec2 halh_image_size = { image_size.x * 0.5f, image_size.y * 0.5f };
		UINT idx = 0u;
		for (const auto& handle : dof_imgui_handles)
		{
			ImGui::Image((ImTextureID)handle.Gpu().ptr, halh_image_size, BORDER_COLOR(bd_color));
			if (idx % 2 == 0) ImGui::SameLine();
			idx++;
		}
		ImGui::TreePop();
	}

}

HRESULT TestScene04::Update(const SceneDesc& desc, float elapsed_time)
{
	tm_update.Restart();

	msaa_depth_draw = false;

	const float ptc_update_time = stop_time ? 0.f : elapsed_time * time_scale;

	auto input = desc.input;

	if (input->IsKeyPressed(KGL::KEYS::ENTER) && input->IsKeyHold(KGL::KEYS::LCONTROL))
	{
		return Init(desc);
	}

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	{
		float camera_speed = input->IsKeyHold(KGL::KEYS::LSHIFT) ? 100.f : 50.f;
		if (input->IsKeyHold(KGL::KEYS::LCONTROL)) camera_speed *= 0.1f;
		camera->Update(desc.window, desc.input, elapsed_time, camera_speed, input->IsMouseHold(KGL::MOUSE_BUTTONS::right));
	}
#if 0
	camera->GetPos() = { 0.f, 200.f, -10.f };
	camera->GetFront() = { 0.f, 0.f, 1.f };
#endif
	camera->GetPos().y = std::max(grid_pos.y + 1, camera->GetPos().y);
	const auto& view = camera->GetView();
	const auto& camera_pos = camera->GetPos();
	const auto& camera_front = camera->GetFront();

	if (desc.input->IsKeyPressed(KGL::KEYS::F1))
	{
		use_gui = !use_gui;
		if (use_gui)
			desc.input->SetMouseVisible(true);
	}
	if (!use_gui)
		desc.input->SetMouseVisible(false);

	fc_mgr->Update(elapsed_time);

	{	// MSAAスケールをキーで変更
		auto msaa_scale = SCAST<UINT>(msaa_selector->GetScale());
		if (input->IsKeyPressed(KGL::KEYS::NUMPAD8))
			msaa_scale++;
		if (input->IsKeyPressed(KGL::KEYS::NUMPAD2))
			msaa_scale--;
		msaa_selector->SetScale(SCAST<MSAASelector::TYPE>(msaa_scale));
	}
	{
		if (input->IsKeyPressed(KGL::KEYS::NUMPAD5))
			fxaa_mgr->SetActiveFlg(!fxaa_mgr->IsActive());
	}
	{	// 空をキーで変更
		if (input->IsKeyPressed(KGL::KEYS::NUMPAD1))
			sky_mgr->ChangeBack();
		if (input->IsKeyPressed(KGL::KEYS::NUMPAD3))
			sky_mgr->ChangeNext();
	}
	if (use_gui)
	{
		ParticleParent pparent{};
		{
			auto* p_pp = ptc_mgr->ParentResource()->Map(0, &CD3DX12_RANGE(0, 0));
			pparent = *p_pp;
			ptc_mgr->ParentResource()->Unmap(0, &CD3DX12_RANGE(0, 0));
		}
		fc_mgr->ImGuiUpdate(desc.app->GetDevice(), &pparent, ptc_tex_srv_gui_handles);

		if (ImGui::Begin("Info"))
		{
			ImGui::Text("FPS");
			ImGui::Text("%.2f", ImGui::GetIO().Framerate);
			ImGui::Text("Position");
			ImGui::Text("xyz[ %.2f, %.2f, %.2f ]", camera_pos.x, camera_pos.y, camera_pos.z);
			ImGui::Spacing();

			ImGui::Checkbox("Draw Sky", &sky_draw);
			ImGui::Spacing();

			ImGui::Checkbox("Use DOF", &dof_flg);
			ImGui::Checkbox("Particle Dof", &ptc_dof_flg);
			auto dof_rtv_num = SCAST<int>(dof_generator->GetRtvNum());
			if (ImGui::SliderInt("Scale", &dof_rtv_num, 1, 8))
			{
				dof_generator->SetRtvNum(SCAST<UINT8>(dof_rtv_num));
			}
			ImGui::Spacing();

			fxaa_mgr->ImGuiTreeUpdate(desc.app->GetResolution());
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

	fxaa_mgr->UpdateBuffer();
	sky_mgr->Update(camera_pos, view * XMLoadFloat4x4(&proj));

	HRESULT hr = FastRender(desc);
	RCHECK_HR(hr, "FastRenderに失敗");

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
		SetNextScene<LoadScene00<TestScene03>>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<LoadScene00<TestScene00>>(desc);

	if (input->IsKeyPressed(KGL::KEYS::BACKSPACE))
	{
		ptc_mgr->Clear();
		pl_shot_ptc_mgr->Clear();
		fireworks.clear();
		player_fireworks.clear();
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
				player_fireworks.clear();
			}
		}
		ImGui::End();

		fs_mgr->GUIUpdate(fc_mgr->GetDescList());
	}

	using namespace DirectX;
	auto particle_total_num = ptc_mgr->ResetCounter();
	auto pl_shot_particle_total_num = pl_shot_ptc_mgr->ResetCounter();
	{
		XMFLOAT4X4 viewf;
		XMStoreFloat4x4(&viewf, view);

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
		ptc_mgr->SetAffectObjects(fc_mgr->affect_objects, player_fireworks);
		pl_shot_ptc_mgr->SetAffectObjects(fc_mgr->affect_objects, {});
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
			tm_ptc_update_cpu.Restart();
			if (particle_total_num > 0)
				ptc_mgr->Update(fc_mgr->affect_objects, player_fireworks);
			if (pl_shot_particle_total_num > 0)
				pl_shot_ptc_mgr->Update(fc_mgr->affect_objects, {});
			tm_ptc_update_cpu.Count();
		}
		
		float key_spawn_late = 5.f;
		const float key_spawn_time = 1.f / key_spawn_late;
		if (input->IsKeyHold(KGL::KEYS::LCONTROL) && input->IsKeyPressed(KGL::KEYS::NUMPADPLUS))
			spawn_fireworks = !spawn_fireworks;
		if (input->IsKeyHold(KGL::KEYS::LCONTROL) && input->IsKeyPressed(KGL::KEYS::NUMPADMINUS))
			stop_time = !stop_time;
		if (spawn_fireworks)
			ptc_key_spawn_counter += ptc_update_time;
		else if (input->IsKeyPressed(KGL::KEYS::MINUS))
			ptc_key_spawn_counter += key_spawn_time;

		// スポナーからFireworksを生成
		if (spawn_fireworks) fs_mgr->Update(ptc_update_time, &fireworks);

		// プレイヤーショット
		if (input->IsMousePressed(KGL::MOUSE_BUTTONS::left))
		{
			auto desc = fc_mgr->GetSelectDesc();
			if (desc)
			{
				desc->pos = camera_pos;
				XMVECTOR xm_xmfront = XMLoadFloat3(&camera_front);
				XMStoreFloat3(&desc->velocity, XMVector3Normalize(xm_xmfront) * desc->speed);
				auto set_desc = *desc;
				// ランダムカラーをセット
				FS_Obj::SetRandomColor(&set_desc);
				set_desc.mass = -5.9724e24f / 100000000;
				player_fireworks.emplace_back(set_desc);
			}
		}

		scene_buffer.mapped_data->view = view;
		scene_buffer.mapped_data->inv_view = XMMatrixInverse(nullptr, view);
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

	for (auto i = 0; i < fireworks.size(); i++)
	{
		if (!fireworks[i].Update(ptc_update_time, &ptc_mgr->frame_particles, &ptc_cb, &fireworks, fc_mgr->affect_objects, player_fireworks))
		{
			fireworks[i] = fireworks.back();
			fireworks.pop_back();
			i--;
		}
	}
	for (auto i = 0; i < player_fireworks.size(); i++)
	{
		if (!player_fireworks[i].Update(ptc_update_time, &pl_shot_ptc_mgr->frame_particles, &ptc_cb, &player_fireworks, fc_mgr->affect_objects, {}))
		{
			player_fireworks[i] = player_fireworks.back();
			player_fireworks.pop_back();
			i--;
		}
	}

	if (use_gpu)
	{
		tm_ptc_update_gpu.Restart();
		cpt_cmd_queue->Wait();
		tm_ptc_update_gpu.Count();
		cpt_cmd_allocator->Reset();
		cpt_cmd_list->Reset(cpt_cmd_allocator.Get(), nullptr);
	}

	tm_ptc_sort.Restart();
	if (particle_total_num > 0)
		ptc_mgr->Sort();
	if (pl_shot_particle_total_num > 0)
		pl_shot_ptc_mgr->Sort();
	tm_ptc_sort.Count();

	const auto frame_ptc_size = ptc_mgr->frame_particles.size() + pl_shot_ptc_mgr->frame_particles.size();
	ptc_mgr->AddToFrameParticle();
	pl_shot_ptc_mgr->AddToFrameParticle();

	{
		const auto counter = ptc_mgr->Size() + pl_shot_ptc_mgr->Size() + fc_mgr->Size();
		KGLDebugOutPutStringNL("\r particle : " + std::to_string(counter) + std::string(10, ' '));
		if (use_gui)
		{
			if (ImGui::Begin("Particle"))
			{
				{
					std::string select_vt = PTC_VT_TABLE[ptc_vt_type];

					// 2番目のパラメーターは、コンボを開く前にプレビューされるラベルです。
					if (ImGui::BeginCombo("Vertex Type", select_vt.c_str()))
					{
						for (int n = 0; n < PTC_VT_TABLE.size(); n++)
						{
							// オブジェクトの外側または内側に、選択内容を好きなように保存できます
							bool is_selected = (select_vt == PTC_VT_TABLE[n]);
							if (ImGui::Selectable(PTC_VT_TABLE[n].c_str(), is_selected))
								ptc_vt_type = SCAST<PTC_VT>(n);
							if (is_selected)
								// コンボを開くときに初期フォーカスを設定できます（キーボードナビゲーションサポートの場合は+をスクロールします）
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}
				ImGui::Checkbox("Wire Frame", &particle_wire);
				ImGui::Checkbox("Particle Dof", &ptc_dof_flg);
				ImGui::Checkbox("SpawnFireworks", &spawn_fireworks);
				bool use_gpu_log = use_gpu;
				if (ImGui::RadioButton("GPU", use_gpu)) use_gpu = true;
				ImGui::SameLine();
				if (ImGui::RadioButton("CPU", !use_gpu)) use_gpu = false;
				const bool reset_max_counter0 = use_gpu_log != use_gpu;
				const bool reset_max_counter1 = ImGui::Button("Reset Max Counter");
				ImGui::Checkbox("Time Stop", &stop_time);
				ImGui::SliderFloat("Time Scale", &time_scale, 0.f, 2.f); ImGui::SameLine();
				ImGui::InputFloat("", &time_scale);
				constexpr auto TimerGui = [](const std::string& title, const KGL::Timer& timer, KGL::Timer::SEC sec_type = KGL::Timer::SEC::MICRO)
				{
					std::string title_text = (title + " [ %5d ][ %5d ][ %5d ][ %5d ]");
					switch (sec_type)
					{
						case KGL::Timer::SEC::MICRO:
							ImGui::Text(title_text.c_str(), timer.Last().micro, timer.Average().micro, timer.Min().micro, timer.Max().micro);
							break;
						case KGL::Timer::SEC::NANO:
							ImGui::Text(title_text.c_str(), timer.Last().nano, timer.Average().nano, timer.Min().nano, timer.Max().nano);
							break;
						default:
							ImGui::Text(title_text.c_str(), timer.Last().milli, timer.Average().milli, timer.Min().milli, timer.Max().milli);
							break;
					}
				};
				TimerGui("Update Count Total ", tm_update);
				TimerGui("Render Count Total ", tm_render);
				TimerGui("Particle Update Gpu", tm_ptc_update_gpu);
				TimerGui("Particle Update Cpu", tm_ptc_update_cpu);
				TimerGui("Particle Update Sort", tm_ptc_sort);

				ct_ptc_total_max = (std::max)(counter, ct_ptc_total_max);
				ImGui::Text("Particle Count Total [ %5d ] : [ %5d ]", counter, ct_ptc_total_max);

				ct_fw_max = (std::max)(fireworks.size() + player_fireworks.size(), ct_fw_max);
				ImGui::Text("Firework Count Total [ %5d ] : [ %5d ]", fireworks.size(), ct_fw_max);
				ct_ptc_frame_max = (std::max)(frame_ptc_size, ct_ptc_frame_max);
				ImGui::Text("Particle Count Frame [ %5d ] : [ %5d ]", frame_ptc_size, ct_ptc_frame_max);

				//ImGui::Text("GPU Time             [ %5d ] : [ %5d ]", gputime, ct_gpu);
				//ImGui::Text("CPU Time             [ %5d ] : [ %5d ]", cputime, ct_cpu);
				//ImGui::Text("Firework Update Time [ %5d ] : [ %5d ]", firework_update, ct_fw_update);
				//ImGui::Text("Map Update Time      [ %5d ] : [ %5d ]", map_update, ct_map_update);

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

				if (reset_max_counter0 || reset_max_counter1) 
					ResetCounterMax();
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

	scene_buffer.mapped_data->zero_texture = particle_wire;

	tm_update.Count();

	ImGui::Render();
	return Render(desc);
}

HRESULT TestScene04::FastRender(const SceneDesc& desc)
{
	using KGL::SCAST;
	HRESULT hr = S_OK;

	//auto window_size = desc.window->GetClientSize();
	auto resolution = desc.app->GetResolution();
	const DirectX::XMFLOAT2 resolutionf = { SCAST<FLOAT>(resolution.x), SCAST<FLOAT>(resolution.y) };

	// ビューとシザーをセット
	D3D12_VIEWPORT viewport =
		CD3DX12_VIEWPORT(0.f, 0.f, resolutionf.x, resolutionf.y);
	auto scissorrect =
		CD3DX12_RECT(0, 0, resolution.x, resolution.y);
	fast_cmd_list->RSSetViewports(1, &viewport);
	fast_cmd_list->RSSetScissorRects(1, &scissorrect);

	// MSAA識別用
	const bool fxaa = fxaa_mgr->GetDesc().type == FXAAManager::TYPE::FXAA_ON;
	const UINT msaa_scale = fxaa ? SCAST<UINT>(MSAASelector::MSAA_OFF) : SCAST<UINT>(msaa_selector->GetScale());
	const bool msaa = msaa_scale != SCAST<UINT>(MSAASelector::MSAA_OFF);

	const auto& rtrc = rt_resources[msaa_scale];
	const auto& rtrc_off = rt_resources[MSAASelector::MSAA_OFF];
	const auto* dsv_handle = msaa ? &rtrc.dsv_handle.Cpu() : &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();

	// Sky と Debug を RT::NON_BLOOM に描画
	fast_cmd_list->ResourceBarrier(1u, &rtrc.rtvs->GetRtvResourceBarrier(true, RT::MAIN));
	rtrc.rtvs->Set(fast_cmd_list, dsv_handle, RT::MAIN);
	rtrc.rtvs->Clear(fast_cmd_list, rtrc.render_targets[RT::MAIN].tex->GetClearColor(), RT::MAIN);
	fast_cmd_list->ClearDepthStencilView(*dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	if (sky_draw) sky_mgr->Render(fast_cmd_list, msaa_scale);
	debug_mgr->Render(fast_cmd_list, msaa_scale);
	fast_cmd_list->ResourceBarrier(1u, &rtrc.rtvs->GetRtvResourceBarrier(false, RT::MAIN));

	fast_cmd_list->Close();
	ID3D12CommandList* cmd_lists[] = { fast_cmd_list.Get() };
	desc.app->GetQueue()->Data()->ExecuteCommandLists(1, cmd_lists);
	desc.app->GetQueue()->Signal();

	return hr;
}

HRESULT TestScene04::Render(const SceneDesc& desc)
{
	using KGL::SCAST;
	HRESULT hr = S_OK;

	tm_render.Restart();

	//auto window_size = desc.window->GetClientSize();
	auto resolution = desc.app->GetResolution();
	const DirectX::XMFLOAT2 resolutionf = { SCAST<FLOAT>(resolution.x), SCAST<FLOAT>(resolution.y) };
	
	// ビューとシザーをセット
	D3D12_VIEWPORT viewport = 
		CD3DX12_VIEWPORT(0.f, 0.f, resolutionf.x, resolutionf.y);
	auto scissorrect = 
		CD3DX12_RECT(0, 0, resolution.x, resolution.y);
	cmd_list->RSSetViewports(1, &viewport);
	cmd_list->RSSetScissorRects(1, &scissorrect);

	// MSAA識別用
	const bool fxaa = fxaa_mgr->GetDesc().type == FXAAManager::TYPE::FXAA_ON;
	const UINT msaa_scale = fxaa ? SCAST<UINT>(MSAASelector::MSAA_OFF) : SCAST<UINT>(msaa_selector->GetScale());
	const bool msaa = msaa_scale != SCAST<UINT>(MSAASelector::MSAA_OFF);

	const auto& rtrc = rt_resources[msaa_scale];
	const auto& rtrc_off = rt_resources[MSAASelector::MSAA_OFF];
	const auto* dsv_handle = msaa ? &rtrc.dsv_handle.Cpu() : &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();

	const auto ptc_size = ptc_mgr->Size();
	const auto pl_shot_ptc_size = pl_shot_ptc_mgr->Size();
	const auto fc_mgr_size = fc_mgr->Size();
	const bool ptc_render = ptc_size + pl_shot_ptc_size + fc_mgr_size > 0;
	if (ptc_render)
	{	// パーティクルを描画
		const UINT rt_num = 2u;
		const auto& rtrbs = rtrc.rtvs->GetRtvResourceBarriers(true, RT::PTC_NON_BLOOM, rt_num);
		auto& ptc_renderer = board_renderers[ptc_vt_type][msaa_scale];
		cmd_list->ResourceBarrier(SCAST<UINT>(rtrbs.size()), rtrbs.data());
		rtrc.rtvs->Set(cmd_list, dsv_handle, RT::PTC_NON_BLOOM, rt_num);
		rtrc.rtvs->Clear(cmd_list, rtrc.render_targets[RT::PTC_NON_BLOOM].tex->GetClearColor(), RT::PTC_NON_BLOOM, rt_num);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

		particle_wire ? 
			ptc_renderer.simple_wire->SetState(cmd_list) :
			ptc_renderer.simple->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, scene_buffer.handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, scene_buffer.handle.Gpu());
		
		cmd_list->SetDescriptorHeaps(1, ptc_tex_mgr->GetHandle().Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(2, ptc_tex_mgr->GetHandle().Gpu());

		if (ptc_size > 0)
		{
			cmd_list->IASetVertexBuffers(0, 1, &b_ptc_vbv);
			cmd_list->DrawInstanced(SCAST<UINT>(ptc_size), 1, 0, 0);
		}
		if (pl_shot_ptc_size > 0)
		{
			cmd_list->IASetVertexBuffers(0, 1, &b_pl_shot_ptc_vbv);
			cmd_list->DrawInstanced(SCAST<UINT>(pl_shot_ptc_size), 1, 0, 0);
		}

		particle_wire ? 
			ptc_renderer.add_pos_wire->SetState(cmd_list) : 
			ptc_renderer.add_pos->SetState(cmd_list);

		if (fc_mgr_size > 0) fc_mgr->Render(cmd_list);

		// 被写界深度用にパーティクルの深度値を書き込む
		if (dof_flg && ptc_dof_flg)
		{
			ptc_renderer.dsv->SetState(cmd_list);
			cmd_list->SetDescriptorHeaps(1, scene_buffer.handle.Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(0, scene_buffer.handle.Gpu());
			
			cmd_list->SetDescriptorHeaps(1, ptc_tex_mgr->GetHandle().Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(2, ptc_tex_mgr->GetHandle().Gpu());

			if (ptc_size > 0)
			{
				cmd_list->IASetVertexBuffers(0, 1, &b_ptc_vbv);
				cmd_list->DrawInstanced(SCAST<UINT>(ptc_size), 1, 0, 0);
			}
			if (pl_shot_ptc_size > 0)
			{
				cmd_list->IASetVertexBuffers(0, 1, &b_pl_shot_ptc_vbv);
				cmd_list->DrawInstanced(SCAST<UINT>(pl_shot_ptc_size), 1, 0, 0);
			}

			ptc_renderer.dsv_add_pos->SetState(cmd_list);
			if (fc_mgr_size > 0) fc_mgr->Render(cmd_list);
		}

		const auto& prrbs = rtrc.rtvs->GetRtvResourceBarriers(false, RT::PTC_NON_BLOOM, rt_num);
		cmd_list->ResourceBarrier(SCAST<UINT>(prrbs.size()), prrbs.data());

		// パーティクルをメインRTに描画（ブルームはまだ）
		cmd_list->ResourceBarrier(1u, &rtrc.rtvs->GetRtvResourceBarrier(true, RT::MAIN));
		rtrc.rtvs->Set(cmd_list, dsv_handle, RT::MAIN);
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		// パーティクルは加算合成描画
		add_sprite_renderers[msaa_scale]->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, rtrc.rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtrc.rtvs->GetSRVGPUHandle(RT::PTC_NON_BLOOM));
		sprite->Render(cmd_list);
		cmd_list->SetGraphicsRootDescriptorTable(0, rtrc.rtvs->GetSRVGPUHandle(RT::PTC_BLOOM));
		sprite->Render(cmd_list);
		cmd_list->ResourceBarrier(1u, &rtrc.rtvs->GetRtvResourceBarrier(false, RT::MAIN));
	}

	if (msaa)
	{
		// MSAA用RTを元リソース用にバリア / RTVSのRTを宛先リソース用にバリア
		{
			D3D12_RESOURCE_BARRIER barriers[] =
			{
				rtrc.render_targets[RT::MAIN].tex->RB(D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
				rtrc_off.render_targets[RT::MAIN].tex->RB(D3D12_RESOURCE_STATE_RESOLVE_DEST),
				rtrc.render_targets[RT::PTC_BLOOM].tex->RB(D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
				rtrc_off.render_targets[RT::PTC_BLOOM].tex->RB(D3D12_RESOURCE_STATE_RESOLVE_DEST)
			};
			cmd_list->ResourceBarrier(SCAST<UINT>(std::size(barriers)), barriers);
		}
		// コピー
		cmd_list->ResolveSubresource(
			rtrc_off.render_targets[RT::MAIN].tex->Data().Get(), 0,
			rtrc.render_targets[RT::MAIN].tex->Data().Get(), 0,
			rtrc_off.render_targets[RT::MAIN].tex->Data()->GetDesc().Format
		);
		// コピー
		cmd_list->ResolveSubresource(
			rtrc_off.render_targets[RT::PTC_BLOOM].tex->Data().Get(), 0,
			rtrc.render_targets[RT::PTC_BLOOM].tex->Data().Get(), 0,
			rtrc_off.render_targets[RT::PTC_BLOOM].tex->Data()->GetDesc().Format
		);
		{

			D3D12_RESOURCE_BARRIER barriers[] =
			{
				rtrc.render_targets[RT::MAIN].tex->RB(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				rtrc_off.render_targets[RT::MAIN].tex->RB(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				rtrc.render_targets[RT::PTC_BLOOM].tex->RB(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				rtrc_off.render_targets[RT::PTC_BLOOM].tex->RB(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			};
			cmd_list->ResourceBarrier(SCAST<UINT>(std::size(barriers)), barriers);
		}
		if (dof_flg || msaa_depth_draw)
		{	// 深度値をコピーする
			desc.app->SetDsv(cmd_list);
			desc.app->ClearDsv(cmd_list);
			cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			depth_sprite_renderer->SetState(cmd_list);
			cmd_list->SetDescriptorHeaps(1, rtrc.depth_srv_handle.Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(0, rtrc.depth_srv_handle.Gpu());
			sprite->Render(cmd_list);
		}
	}

	if (ptc_render)
	{
		bloom_generator->Generate(cmd_list, rtrc_off.rtvs->GetSRVHeap(), rtrc_off.rtvs->GetSRVGPUHandle(RT::PTC_BLOOM), viewport);
		// ブルームはMSAAで行わない
		cmd_list->ResourceBarrier(1u, &rtrc_off.rtvs->GetRtvResourceBarrier(true, RT::MAIN));
		rtrc_off.rtvs->Set(cmd_list, nullptr, RT::MAIN);
		bloom_generator->Render(cmd_list);
		cmd_list->ResourceBarrier(1u, &rtrc_off.rtvs->GetRtvResourceBarrier(false, RT::MAIN));
	}

	// DOFの場合はGeneratorから描画させ、そうでない場合はSpriteで描画する
	if (dof_flg)
	{
		dof_generator->Generate(cmd_list, rtrc_off.rtvs->GetSRVHeap(), rtrc_off.rtvs->GetSRVGPUHandle(RT::MAIN), viewport);
		cmd_list->ResourceBarrier(1u, &rtrc_off.rtvs->GetRtvResourceBarrier(true, RT::MAIN));
		rtrc_off.rtvs->Set(cmd_list, nullptr, RT::MAIN);
		rtrc_off.rtvs->Clear(cmd_list, rtrc_off.render_targets[RT::MAIN].tex->GetClearColor(), RT::MAIN);
		dof_generator->Render(cmd_list, rtrc_off.depth_srv_handle.Heap(), rtrc_off.depth_srv_handle.Gpu());
		cmd_list->ResourceBarrier(1u, &rtrc_off.rtvs->GetRtvResourceBarrier(false, RT::MAIN));
	}

	cmd_list->ResourceBarrier(1u, &desc.app->GetRtvResourceBarrier(true));
	
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	if (fxaa)
	{
		cmd_list->ResourceBarrier(1u, &rtrc_off.rtvs->GetRtvResourceBarrier(true, RT::FXAA_GRAY));
		rtrc_off.rtvs->Set(cmd_list, nullptr, RT::FXAA_GRAY);
		rtrc_off.rtvs->Clear(cmd_list, rtrc_off.render_targets[RT::FXAA_GRAY].tex->GetClearColor(), RT::FXAA_GRAY);
		fxaa_mgr->SetGrayState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, rtrc_off.rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtrc_off.rtvs->GetSRVGPUHandle(RT::MAIN));
		sprite->Render(cmd_list);
		cmd_list->ResourceBarrier(1u, &rtrc_off.rtvs->GetRtvResourceBarrier(false, RT::FXAA_GRAY));

		desc.app->SetRtv(cmd_list);
		fxaa_mgr->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, rtrc_off.rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtrc_off.rtvs->GetSRVGPUHandle(RT::FXAA_GRAY));
		sprite->Render(cmd_list);
	}
	else
	{
		desc.app->SetRtv(cmd_list);
		sprite_renderers[MSAASelector::MSAA_OFF]->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, rtrc_off.rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtrc_off.rtvs->GetSRVGPUHandle(RT::MAIN));
		sprite->Render(cmd_list);
	}

	cmd_list->SetDescriptorHeaps(1, desc.imgui_handle.Heap().GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list.Get());
	cmd_list->ResourceBarrier(1u, &desc.app->GetRtvResourceBarrier(false));
	
	cmd_list->Close();

	// FastRenderの完了を待つ
	desc.app->GetQueue()->Wait();
	fast_cmd_allocator->Reset();
	fast_cmd_list->Reset(fast_cmd_allocator.Get(), nullptr);

	ID3D12CommandList* cmd_lists[] = { cmd_list.Get() };
	desc.app->GetQueue()->Data()->ExecuteCommandLists(1, cmd_lists);
	desc.app->GetQueue()->Signal();
	desc.app->GetQueue()->Wait();

	tm_render.Count();

	cmd_allocator->Reset();
	cmd_list->Reset(cmd_allocator.Get(), nullptr);

	if (desc.app->IsTearingSupport())
		desc.app->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	else
		desc.app->GetSwapchain()->Present(1, 1);

	return S_OK;
}

HRESULT TestScene04::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	sky_mgr->Uninit(desc.imgui_heap_mgr);

	for (auto& handle : ptc_tex_srv_gui_handles)
	{
		if (handle.Heap())
			desc.imgui_heap_mgr->Free(handle);
	}

	for (const auto& rtrs : rt_resources)
	{
		for (const auto& rt : rtrs.render_targets)
		{
			if (rt.gui_srv_handle.Heap())
				desc.imgui_heap_mgr->Free(rt.gui_srv_handle);
		}
	}

	return S_OK;
}