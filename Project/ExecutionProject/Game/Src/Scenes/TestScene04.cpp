#include "../../Hrd/Scenes/Scenes.hpp"

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

	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);
	hr = KGL::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cpt_cmd_allocator, &cpt_cmd_list, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	RCHECK(FAILED(hr), "コマンドアロケーター/リストの作成に失敗", hr);
	{
		// コマンドキューの生成
		D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
		cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		cmd_queue_desc.NodeMask = 0;
		cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

		cpt_cmd_queue = std::make_shared<KGL::CommandQueue>(device, cmd_queue_desc);
		RCHECK(FAILED(hr), "コマンドキューの生成に失敗！", hr);
	}


	{
		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
		sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device, renderer_desc);

		renderer_desc.blend_types[0] = KGL::BDTYPE::ADD;
		add_sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device, renderer_desc);

		renderer_desc.ps_desc.hlsl = "./HLSL/2D/SpriteAdd_ps.hlsl";
		renderer_desc.blend_types[0] = KGL::BDTYPE::ALPHA;
		high_sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device, renderer_desc);
		sprite = std::make_shared<KGL::Sprite>(device);
	}

	{
		auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;
		//renderer_desc.depth_desc = {};
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/Particle_ps.hlsl";
		renderer_desc.vs_desc.hlsl = "./HLSL/3D/Particle_vs.hlsl";
		renderer_desc.gs_desc.hlsl = "./HLSL/3D/Particle_gs.hlsl";
		renderer_desc.input_layouts.clear();
		renderer_desc.input_layouts.push_back({
			"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
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
			"CENTER_PROPOTION", 0, DXGI_FORMAT_R32_FLOAT, 0,
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
			"ACCS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"EXIST", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.other_desc.topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		renderer_desc.blend_types[0] = KGL::BLEND::TYPE::ADD;
		renderer_desc.depth_desc = {};
		renderer_desc.render_targets.push_back(renderer_desc.render_targets[0]);
		board_renderer = std::make_shared<KGL::_3D::Renderer>(device, renderer_desc);
		board = std::make_shared<KGL::Board>(device);
	}

	hr = cpt_scene_buffer.Load(desc);
	RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);
	hr = scene_buffer.Load(desc);
	RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);

	{
		{
			constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
			rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
				device, desc.app->GetRtvBuffers().at(0), clear_value));
			const auto& resources = KGL::TEXTURE::GetResources(rtv_textures);
			rtvs = std::make_shared<KGL::RenderTargetView>(device, resources);
		}
		{
			constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
			ptc_rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
				device, desc.app->GetRtvBuffers().at(0), clear_value));
			ptc_rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
				device, desc.app->GetRtvBuffers().at(0), clear_value));
			const auto& resources = KGL::TEXTURE::GetResources(ptc_rtv_textures);
			ptc_rtvs = std::make_shared<KGL::RenderTargetView>(device, resources);

			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srv_desc.Texture2D.MipLevels = 1;
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			for (auto& tex : ptc_rtv_textures)
			{
				auto& imgui_handle = ptc_srv_gui_handles.emplace_back();
				imgui_handle = desc.imgui_heap_mgr->Alloc();

				srv_desc.Format = tex->Data()->GetDesc().Format;
				device->CreateShaderResourceView(tex->Data().Get(), &srv_desc, imgui_handle.Cpu());
			}
		}
	}

	D3D12_HEAP_PROPERTIES prop = {};
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.CreationNodeMask = 1;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	prop.Type = D3D12_HEAP_TYPE_CUSTOM;
	prop.VisibleNodeMask = 1;
	particle_resource = std::make_shared<KGL::Resource<Particle>>(device, 1000000u, &prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	frame_particles.reserve(particle_resource->Size());
	particle_counter_res = std::make_shared<KGL::Resource<UINT32>>(device, 1u, &prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	particle_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	particle_pipeline = std::make_shared<KGL::ComputePipline>(device);
	b_cbv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	matrix_resource = std::make_shared<KGL::Resource<CbvParam>>(device, 1u);
	b_tex_data[0].tex = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Particles/particle.png");
	b_tex_data[1].tex = std::make_shared<KGL::Texture>(device);

	b_srv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 2u);
	
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.Buffer.NumElements = KGL::SCAST<UINT>(particle_resource->Size());
	uav_desc.Buffer.StructureByteStride = sizeof(Particle);

	uav_desc.Buffer.CounterOffsetInBytes = 0u;

	particle_begin_handle = particle_desc_mgr->Alloc();
	device->CreateUnorderedAccessView(particle_resource->Data().Get(), particle_counter_res->Data().Get(), &uav_desc, particle_begin_handle.Cpu());
	
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

		grid_renderer = std::make_shared<KGL::BaseRenderer>(device, renderer_desc);

		grid_buffer.Load(desc);
	}
	{
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
		b_vbv.BufferLocation = particle_resource->Data()->GetGPUVirtualAddress();
		b_vbv.SizeInBytes = particle_resource->SizeInBytes();
		b_vbv.StrideInBytes = sizeof(Particle);
	}

	fireworks.reserve(10000u);

	
	sky_mgr = std::make_shared<SkyManager>(device, desc.imgui_heap_mgr);

	{
		bloom_generator = std::make_shared<BloomGenerator>(device, desc.app->GetRtvBuffers().at(0));
		bloom_imgui_handle = desc.imgui_heap_mgr->Alloc();
		const auto& tex = bloom_generator->GetTexture();

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Format = tex->Data()->GetDesc().Format;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		device->CreateShaderResourceView(tex->Data().Get(), &srv_desc, bloom_imgui_handle.Cpu());
	}

	return hr;
}

HRESULT TestScene04::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();

	camera.eye = { 0.f, 10.f, -50.f };
	camera.focus_vec = { 0.f, 0.f, 1.f };
	camera.up = { 0.f, 1.f, 0.f };

	//XMStoreFloat3(&scene_buffer.mapped_data->light_vector, XMVector3Normalize(XMVectorSet(+0.2f, -0.7f, 0.5f, 0.f)));

	const XMMATRIX proj_mat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(window_size.x) / static_cast<float>(window_size.y),	// アスペクト比
		1.0f, 1000.0f // near, far
	);
	XMStoreFloat4x4(&proj, proj_mat);
	auto* p_particles = particle_resource->Map(0, &CD3DX12_RANGE(0, 0));
	Particle particle_base = {};

	std::fill(&p_particles[0], &p_particles[particle_resource->Size()], particle_base);
	particle_resource->Unmap(0, &CD3DX12_RANGE(0, 0));

	{
		auto* p_counter = particle_counter_res->Map(0, &CD3DX12_RANGE(0, 0));
		*p_counter = 0u;
		particle_counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	}

	next_particle_offset = 0u;

	XMMATRIX view = KGL::CAMERA::GetView(camera);
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
		grid_buffer.mapped_data->eye_pos = camera.eye;
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

	cpt_scene_buffer.mapped_data->center_pos = { 0.f, -6378.1f * 1000.f, 0.f };
	cpt_scene_buffer.mapped_data->center_mass = 5.9724e24f;
	cpt_scene_buffer.mapped_data->resistivity = 0.1f;

	spawn_counter = 0.f;
	ptc_key_spawn_counter = 0.f;
	particle_total_num = 0u;

	frame_particles.clear();

	camera_angle = { 0.f, 0.f };

	sky_mgr->Init(view * proj_mat);

	use_gui = true;

	ResetCounterMax();
	time_scale = 1.f;
	use_gpu = true;

	return S_OK;
}

void TestScene04::ResetCounterMax()
{
	ct_particle = ct_frame_ptc = ct_fw = ct_gpu = ct_cpu = ct_fw_update = ct_map_update = 0u;
}

HRESULT TestScene04::Update(const SceneDesc& desc, float elapsed_time)
{
	const float ptc_update_time = elapsed_time * time_scale;

	auto input = desc.input;

	if (input->IsKeyPressed(KGL::KEYS::ENTER))
	{
		return Init(desc);;
	}

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (desc.input->IsKeyPressed(KGL::KEYS::F1))
		use_gui = !use_gui;

	if (use_gui)
	{
		if (ImGui::Begin("RenderTargets"))
		{
			auto gui_size = ImGui::GetWindowSize();
			for (const auto& handle : ptc_srv_gui_handles)
				ImGui::Image((ImTextureID)handle.Gpu().ptr, { gui_size.x * 0.8f, gui_size.y * 0.8f });
			ImGui::Image((ImTextureID)bloom_imgui_handle.Gpu().ptr, { gui_size.x * 0.8f, gui_size.y * 0.8f });
		}
		ImGui::End();
		ImGui::Begin("Camera");
		ImGui::SliderFloat3("pos", (float*)&camera.eye, -500.f, 500.f);
		{
			using namespace DirectX;
			DirectX::XMFLOAT2 degree = { XMConvertToDegrees(camera_angle.x), XMConvertToDegrees(camera_angle.y) };
			ImGui::SliderFloat2("angle", (float*)&degree, -180.f, 180.f);
			degree.y = std::clamp(degree.y, -90.f + 0.01f, +90.f - 0.01f);
			camera_angle = { XMConvertToRadians(degree.x), XMConvertToRadians(degree.y) };
			XMVECTOR z_v = XMVectorSet(0.f, 0.f, 1.f, 0.f);
			XMStoreFloat3(&camera.focus_vec, XMVector3Transform(z_v, XMMatrixRotationX(camera_angle.y) * XMMatrixRotationY(camera_angle.x)));
		}
		ImGui::End();
		ImGui::Begin("Grid");
		ImGui::SliderFloat3("pos", (float*)&grid_pos, -10.f, 10.f);
		{
			using namespace DirectX;
			XMMATRIX W, S, R, T;
			S = XMMatrixScaling(1.f, 1.f, 1.f);
			R = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
			T = XMMatrixTranslation(camera.eye.x - fmodf(camera.eye.x, 1.f), grid_pos.y, camera.eye.z - fmodf(camera.eye.z, 1.f));
			W = S * R * T;
			XMMATRIX WVP = W * KGL::CAMERA::GetView(camera) * XMLoadFloat4x4(&proj);
			XMStoreFloat4x4(&grid_buffer.mapped_data->world, W);
			XMStoreFloat4x4(&grid_buffer.mapped_data->wvp, WVP);
		}
		if (ImGui::SliderFloat("length_min", (float*)&grid_buffer.mapped_data->length_min, 1.f, grid_buffer.mapped_data->length_max - 0.01f))
		{
			grid_buffer.mapped_data->length_max = std::max(grid_buffer.mapped_data->length_min + 0.01f, grid_buffer.mapped_data->length_max);
		}
		if (ImGui::SliderFloat("length_max", (float*)&grid_buffer.mapped_data->length_max, grid_buffer.mapped_data->length_min + 0.01f, 100.f))
		{
			grid_buffer.mapped_data->length_min = std::min(grid_buffer.mapped_data->length_min, grid_buffer.mapped_data->length_max - 0.01f);
		}
		ImGui::End();
		
		sky_mgr->ImGuiUpdate(KGL::CAMERA::GetView(camera) * XMLoadFloat4x4(&proj));
	}

	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<TestScene03>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<TestScene05>(desc);

	using namespace DirectX;

	{
		XMMATRIX view = KGL::CAMERA::GetView(camera);
		XMFLOAT4X4 viewf;
		XMStoreFloat4x4(&viewf, view);

		constexpr float center_speed = 1.f;
		if (input->IsKeyHold(KGL::KEYS::D))
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
		}

		float key_spawn_late = 10.f;
		const float key_spawn_time = 1.f / key_spawn_late;
		if (input->IsKeyHold(KGL::KEYS::NUMPADPLUS))
			ptc_key_spawn_counter += ptc_update_time;
		else if (input->IsKeyPressed(KGL::KEYS::SPACE))
			ptc_key_spawn_counter += key_spawn_time;

		while (ptc_key_spawn_counter >= key_spawn_time)
		{
			FireworksDesc desc;
			desc.pos = { 0.f, 0.f, 0.f };
			desc.velocity = { 0.f, 50.f, 0.f };

			std::random_device rd;
			std::mt19937 mt(rd());

			std::uniform_real_distribution<float> rmdpos(-10.f, +10.f);
			desc.pos.x += rmdpos(mt);
			desc.pos.z += rmdpos(mt);

			XMFLOAT2 nmangle;
			nmangle.x = XMConvertToRadians(0.f) / XM_PI;
			nmangle.y = XMConvertToRadians(10.f) / XM_PI;
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

			desc.effects = FIREWORK_EFFECTS::A;
			desc.mass = 1.f;
			desc.effects[2].child = desc;
			desc.effects[2].has_child = true;
			desc.effects[2].child.effects[0].late = { 10.f, 10.f };
			desc.effects[2].child.effects[1].late = { 100.f, 100.f };
			desc.effects[2].child.effects[1].start_accel = 1.f;
			fireworks.emplace_back(desc);
			ptc_key_spawn_counter -= key_spawn_time;
		}

		cpt_scene_buffer.mapped_data->elapsed_time = ptc_update_time;

		scene_buffer.mapped_data->view = view;
		scene_buffer.mapped_data->proj = XMLoadFloat4x4(&proj);
		scene_buffer.mapped_data->eye = camera.eye;
		scene_buffer.mapped_data->light_vector = { 0.f, 0.f, 1.f };
		grid_buffer.mapped_data->eye_pos = camera.eye;
	}

	// 一秒秒間に[spawn_late]個のパーティクルを発生させる
	constexpr UINT spawn_late = 2500;
	KGL::Timer timer;

	{
		auto* p_counter = particle_counter_res->Map(0, &CD3DX12_RANGE(0, 0));
		*p_counter = 0u;
		particle_counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
	if (particle_total_num > 0)
	{
		if (use_gpu)
		{
			cpt_cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(particle_counter_res->Data().Get()));

			particle_pipeline->SetState(cpt_cmd_list);
			//ID3D12DescriptorHeap* const heaps[] = { cpt_scene_buffer.handle.Heap().Get(), particle_begin_handle.Heap().Get() };

			//cpt_cmd_list->SetDescriptorHeaps(1, b_counter_handle.Heap().GetAddressOf());
			//cpt_cmd_list->SetComputeRootDescriptorTable(1, b_counter_handle.Gpu());
			cpt_cmd_list->SetDescriptorHeaps(1, particle_begin_handle.Heap().GetAddressOf());
			cpt_cmd_list->SetComputeRootDescriptorTable(1, particle_begin_handle.Gpu());

			cpt_cmd_list->SetDescriptorHeaps(1, cpt_scene_buffer.handle.Heap().GetAddressOf());
			cpt_cmd_list->SetComputeRootDescriptorTable(0, cpt_scene_buffer.handle.Gpu());

			const UINT ptcl_size = particle_total_num;
			DirectX::XMUINT3 patch = {};
			constexpr UINT patch_max = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
			patch.x = (ptcl_size / 64) + ((ptcl_size % 64) > 0 ? 1 : 0);
			patch.x = std::min(patch.x, patch_max);
			patch.y = 1;
			patch.z = 1;

			cpt_cmd_list->Dispatch(patch.x, patch.y, patch.z);
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
			auto* p_counter = particle_counter_res->Map(0, &CD3DX12_RANGE(0, 0));
			auto particles = particle_resource->Map(0, &CD3DX12_RANGE(0, 0));
			const size_t i_max = particle_total_num;
			XMVECTOR resultant;
			const auto* cb = cpt_scene_buffer.mapped_data;
			for (int i = 0; i < i_max; i++)
			{
				if (!particles[i].Alive()) continue;
				particles[i].Update(cpt_scene_buffer.mapped_data->elapsed_time, cb);
				(*p_counter)++;
			}
			particle_resource->Unmap(0, &CD3DX12_RANGE(0, 0));
			particle_counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
		}
	}

	const auto* cb = cpt_scene_buffer.mapped_data;
	constexpr float spawn_elapsed = 1.f / spawn_late;
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
	}
	UINT64 cputime = timer.GetTime(KGL::Timer::SEC::MICRO);
	timer.Restart();
	for (auto i = 0; i < fireworks.size(); i++)
	{
		if (!fireworks[i].Update(ptc_update_time, &frame_particles, cb, &fireworks))
		{
			fireworks[i] = fireworks.back();
			fireworks.pop_back();
			i--;
		}
	}
	UINT64 firework_update = timer.GetTime(KGL::Timer::SEC::MICRO);

	timer.Restart();
	if (particle_total_num > 0 && use_gpu)
	{
		cpt_cmd_queue->Wait();
		cpt_cmd_allocator->Reset();
		cpt_cmd_list->Reset(cpt_cmd_allocator.Get(), nullptr);
	}
	UINT64 gputime = timer.GetTime(KGL::Timer::SEC::MICRO);
	timer.Restart();

	if (particle_total_num > 0)
	{
		next_particle_offset = particle_total_num * sizeof(Particle);
		auto particles = particle_resource->Map(0u, &CD3DX12_RANGE(0, next_particle_offset));
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
		particle_resource->Unmap(0u, &CD3DX12_RANGE(0, next_particle_offset));
		next_particle_offset = sizeof(Particle) * alive_count;
	}

	const size_t frame_ptc_size = frame_particles.size();
	size_t frame_add_ptc_num = 0u;
	if (!frame_particles.empty())
	{
		D3D12_RANGE range;
		range.Begin = next_particle_offset;
		range.End = range.Begin + sizeof(Particle) * (KGL::SCAST<SIZE_T>(frame_ptc_size));
		const size_t offset_max = sizeof(Particle) * (particle_resource->Size());
		const size_t bi = range.Begin / sizeof(Particle);
		if (range.End > offset_max)
			range.End = offset_max;
		const auto check_count_max = (range.End - range.Begin) / sizeof(Particle);
		auto particles = particle_resource->Map(0u, &range);
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
		particle_resource->Unmap(0u, &range);
		next_particle_offset = range.Begin + sizeof(Particle) * check_count++;
	}
	UINT64 map_update = timer.GetTime(KGL::Timer::SEC::MICRO);
	timer.Restart();
	cputime += firework_update + map_update;

	{
		auto* p_counter = particle_counter_res->Map(0, &CD3DX12_RANGE(0, 0));
		*p_counter += frame_add_ptc_num;
		particle_total_num = *p_counter;
		KGLDebugOutPutStringNL("\r particle : " + std::to_string(*p_counter) + std::string(10, ' '));
		if (use_gui)
		{
			if (ImGui::Begin("Particle"))
			{
				bool use_gpu_log = use_gpu;
				if (ImGui::RadioButton("GPU", use_gpu)) use_gpu = true;
				ImGui::SameLine();
				if (ImGui::RadioButton("CPU", !use_gpu)) use_gpu = false;
				const bool reset_max_counter0 = use_gpu_log != use_gpu;
				const bool reset_max_counter1 = ImGui::Button("Reset Max Counter");
				ImGui::SliderFloat("Time Scale", &time_scale, 0.f, 2.f); ImGui::SameLine();
				ImGui::InputFloat("", &time_scale);
				ct_particle = std::max<UINT64>(ct_particle, *p_counter);
				ImGui::Text("Particle Count Total [ %5d ] : [ %5d ]", *p_counter, ct_particle);
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

				if (reset_max_counter0 || reset_max_counter1) ResetCounterMax();
				{
					const auto imgui_window_size = ImGui::GetWindowSize();
					ImGui::BeginChild("scrolling", ImVec2(imgui_window_size.x * 0.9f, std::max<float>(std::min<float>(imgui_window_size.y - 100, 200.f), 0)), ImGuiWindowFlags_NoTitleBar);
					for (auto& it : b_tex_data)
					{
						const ImVec2 image_size = { 90, 90 };
						ImGui::Image((ImTextureID)it.imgui_handle.Gpu().ptr, image_size);
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
		particle_counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	}

	{
		static float angle = 0.f;
		angle += DirectX::XMConvertToRadians(90.f) * elapsed_time;
		XMMATRIX W, S, R, T;
		S = XMMatrixScaling(1.f, 1.f, 1.f);
		R = XMMatrixRotationRollPitchYaw(0.f, angle, 0.f);
		T = XMMatrixTranslation(0.f, 0.f, -1.f);
		W = S * R * T;

		XMMATRIX WVP = W * KGL::CAMERA::GetView(camera) * XMLoadFloat4x4(&proj);
		CbvParam* param = matrix_resource->Map();
		XMStoreFloat4x4(&param->mat, WVP);
		param->color = { 1.f, 1.f, 1.f, 1.f };
		matrix_resource->Unmap();
	}

	return Render(desc);
}

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
		cmd_list->ResourceBarrier(1u, &rtvs->GetRtvResourceBarrier(true, rtv_num));
		const auto* dsv_handle = &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
		rtvs->Set(cmd_list, dsv_handle, rtv_num);
		rtvs->Clear(cmd_list, rtv_textures[rtv_num]->GetClearColor(), rtv_num);
		desc.app->ClearDsv(cmd_list);

		// SKY描画
		sky_mgr->Render(cmd_list);

		// グリッド描画
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		grid_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, grid_buffer.handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, grid_buffer.handle.Gpu());
		cmd_list->IASetVertexBuffers(0, 1, &grid_vbv);
		cmd_list->IASetIndexBuffer(&grid_ibv);
		cmd_list->DrawIndexedInstanced(grid_idx_resource->Size(), 1, 0, 0, 0);

		cmd_list->ResourceBarrier(1u, &rtvs->GetRtvResourceBarrier(false, rtv_num));
	}
	{
		const UINT rtv_num = 0u;
		cmd_list->ResourceBarrier(1u, &ptc_rtvs->GetRtvResourceBarrier(true, rtv_num));

		const auto* dsv_handle = &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart();
		ptc_rtvs->Set(cmd_list, dsv_handle, rtv_num);
		ptc_rtvs->Clear(cmd_list, ptc_rtv_textures[rtv_num]->GetClearColor(), rtv_num);

		// パーティクル描画
		if (particle_total_num > 0)
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

			cmd_list->IASetVertexBuffers(0, 1, &b_vbv);
			cmd_list->DrawInstanced(SCAST<UINT>(particle_total_num), 1, 0, 0);
		}

		cmd_list->ResourceBarrier(1u, &ptc_rtvs->GetRtvResourceBarrier(false, rtv_num));
	}
	{
		const UINT rtv_num = 1u;
		cmd_list->ResourceBarrier(1u, &ptc_rtvs->GetRtvResourceBarrier(true, rtv_num));

		ptc_rtvs->Set(cmd_list, nullptr, rtv_num);
		ptc_rtvs->Clear(cmd_list, ptc_rtv_textures[rtv_num]->GetClearColor(), rtv_num);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		high_sprite_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, ptc_rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, ptc_rtvs->GetSRVGPUHandle(0));
		sprite->Render(cmd_list);

		cmd_list->ResourceBarrier(1u, &ptc_rtvs->GetRtvResourceBarrier(false, rtv_num));
	}
	{
		bloom_generator->Generate(cmd_list, ptc_rtvs->GetSRVHeap(), ptc_rtvs->GetSRVGPUHandle(1), viewport);
	}
	{
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));

		desc.app->SetRtv(cmd_list);

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

		bloom_generator->Render(cmd_list);

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