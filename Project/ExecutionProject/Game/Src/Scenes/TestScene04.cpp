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

	sprite_renderer = std::make_shared<KGL::_2D::Renderer>(device);
	sprite = std::make_shared<KGL::Sprite>(device);

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
			"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"MASS", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE_POWER", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"ANGLE", 0, DXGI_FORMAT_R32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"VELOCITY", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
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
		board_renderer = std::make_shared<KGL::_3D::Renderer>(device, renderer_desc);
		board = std::make_shared<KGL::Board>(device);
	}

	hr = cpt_scene_buffer.Load(desc);
	RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);
	hr = scene_buffer.Load(desc);
	RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);

	{
		constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
		/*rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
			device, desc.app->GetRtvBuffers().at(0), clear_value));
		rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
			device, desc.app->GetRtvBuffers().at(0), clear_value));*/
		rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
			device, desc.app->GetRtvBuffers().at(0), clear_value));
		const auto& resources = KGL::TEXTURE::GetResources(rtv_textures);
		rtvs = std::make_shared<KGL::RenderTargetView>(device, resources);
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
	b_texture = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Particles/particle.png");
	b_srv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	
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
		srv_desc.Format = b_texture->Data()->GetDesc().Format;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		b_srv = b_srv_descmgr->Alloc();
		device->CreateShaderResourceView(b_texture->Data().Get(), &srv_desc, b_srv.Cpu());
	}
	{
		b_vbv.BufferLocation = particle_resource->Data()->GetGPUVirtualAddress();
		b_vbv.SizeInBytes = particle_resource->SizeInBytes();
		b_vbv.StrideInBytes = sizeof(Particle);
	}

	fireworks.reserve(10000u);

	
	{
		auto LoadSkyTex = [&](std::shared_ptr<SkyTex> data, std::string name0, std::string name1, std::string extension)
		{
			data->tex[CUBE::FRONT] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_0_Front+Z" + extension);
			data->tex[CUBE::BACK] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_1_Back-Z" + extension);
			data->tex[CUBE::RIGHT] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_2_Left+X" + extension);
			data->tex[CUBE::LEFT] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_3_Right-X" + extension);
			data->tex[CUBE::TOP] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_4_Up+Y" + extension);
			data->tex[CUBE::BOTTOM] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_5_Down-Y" + extension);
		};
		static const std::vector<std::pair<std::string, std::string>> TEXTURES =
		{
			{ "Cartoon Base BlueSky",		"Sky_Day_BlueSky_Nothing_" },
			{ "Cartoon Base NightSky",		"Cartoon Base NightSky_" },
			{ "Cold Night",					"Cold Night__" },
			{ "Cold Sunset",				"Cold Sunset__" },
			{ "Deep Dusk",					"Deep Dusk__" },
			{ "Epic_BlueSunset",			"Epic_BlueSunset_" },
			{ "Epic_GloriousPink",			"Epic_GloriousPink_" },
			{ "Night MoonBurst",			"Night Moon Burst_" },
			{ "Overcast Low",				"Sky_AllSky_Overcast4_Low_" },
			{ "Space_AnotherPlanet",		"AllSky_Space_AnotherPlanet_" },
		};

		std::string extension = ".DDS";
		const size_t tex_num = TEXTURES.size();
		for (size_t i = 0u; i < tex_num; i++)
		{
			auto& data = sky_tex_data[TEXTURES[i].first];
			data = std::make_shared<SkyTex>();
			LoadSkyTex(data, TEXTURES[i].first, TEXTURES[i].second, extension);
		}
		
		/*for (int i = 0; i < 100; i++)
		{
			bool ping = i % 2 == 0;
			extension = ping ? ".png" : ".DDS";
			std::chrono::system_clock::time_point  start;
			start = std::chrono::system_clock::now();
			sky_tex[CUBE::FRONT] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_0_Front+Z" + extension);
			sky_tex[CUBE::BACK] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_1_Back-Z" + extension);
			sky_tex[CUBE::RIGHT] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_2_Left+X" + extension);
			sky_tex[CUBE::LEFT] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_3_Right-X" + extension);
			sky_tex[CUBE::TOP] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_4_Up+Y" + extension);
			sky_tex[CUBE::BOTTOM] = std::make_shared<KGL::Texture>(device, "./Assets/Textures/Sky/" + name0 + "/" + name1 + "Cam_5_Down-Y" + extension);
			double elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - start).count();
			KGLDebugOutPutString((ping ? "[PNG]" : "[DDS]") + std::to_string(elapsed));
		}*/

		std::vector<SkyVertex> sky_vertices(4 * CUBE::NUM);
		enum { TL, TR, BL, BR };
		std::vector<DirectX::XMFLOAT2> uv(4);
		uv[TL] = { 0.f, 0.f };
		uv[TR] = { 1.f, 0.f };
		uv[BL] = { 0.f, 1.f };
		uv[BR] = { 1.f, 1.f };

		const auto sky_tex_size = sky_tex_data.begin()->second->tex[0]->Data()->GetDesc().Width;
		float inner = 1.f / sky_tex_size;

		sky_vertices[CUBE::FRONT * 4 + TL] = { { -0.5f, +0.5f, +0.5f - inner }, uv[TL] };
		sky_vertices[CUBE::FRONT * 4 + TR] = { { +0.5f, +0.5f, +0.5f - inner }, uv[TR] };
		sky_vertices[CUBE::FRONT * 4 + BL] = { { -0.5f, -0.5f, +0.5f - inner }, uv[BL] };
		sky_vertices[CUBE::FRONT * 4 + BR] = { { +0.5f, -0.5f, +0.5f - inner }, uv[BR] };

		sky_vertices[CUBE::BACK * 4 + TL] = { { +0.5f, +0.5f, -0.5f + inner }, uv[TL] };
		sky_vertices[CUBE::BACK * 4 + TR] = { { -0.5f, +0.5f, -0.5f + inner }, uv[TR] };
		sky_vertices[CUBE::BACK * 4 + BL] = { { +0.5f, -0.5f, -0.5f + inner }, uv[BL] };
		sky_vertices[CUBE::BACK * 4 + BR] = { { -0.5f, -0.5f, -0.5f + inner }, uv[BR] };

		sky_vertices[CUBE::RIGHT * 4 + TL] = { { +0.5f - inner, +0.5f, +0.5f }, uv[TL] };
		sky_vertices[CUBE::RIGHT * 4 + TR] = { { +0.5f - inner, +0.5f, -0.5f }, uv[TR] };
		sky_vertices[CUBE::RIGHT * 4 + BL] = { { +0.5f - inner, -0.5f, +0.5f }, uv[BL] };
		sky_vertices[CUBE::RIGHT * 4 + BR] = { { +0.5f - inner, -0.5f, -0.5f }, uv[BR] };

		sky_vertices[CUBE::LEFT * 4 + TL] = { { -0.5f + inner, +0.5f, -0.5f }, uv[TL] };
		sky_vertices[CUBE::LEFT * 4 + TR] = { { -0.5f + inner, +0.5f, +0.5f }, uv[TR] };
		sky_vertices[CUBE::LEFT * 4 + BL] = { { -0.5f + inner, -0.5f, -0.5f }, uv[BL] };
		sky_vertices[CUBE::LEFT * 4 + BR] = { { -0.5f + inner, -0.5f, +0.5f }, uv[BR] };

		sky_vertices[CUBE::TOP * 4 + TL] = { { -0.5f, +0.5f - inner, -0.5f }, uv[TL] };
		sky_vertices[CUBE::TOP * 4 + TR] = { { +0.5f, +0.5f - inner, -0.5f }, uv[TR] };
		sky_vertices[CUBE::TOP * 4 + BL] = { { -0.5f, +0.5f - inner, +0.5f }, uv[BL] };
		sky_vertices[CUBE::TOP * 4 + BR] = { { +0.5f, +0.5f - inner, +0.5f }, uv[BR] };

		sky_vertices[CUBE::BOTTOM * 4 + TL] = { { -0.5f, -0.5f + inner, +0.5f }, uv[TL] };
		sky_vertices[CUBE::BOTTOM * 4 + TR] = { { +0.5f, -0.5f + inner, +0.5f }, uv[TR] };
		sky_vertices[CUBE::BOTTOM * 4 + BL] = { { -0.5f, -0.5f + inner, -0.5f }, uv[BL] };
		sky_vertices[CUBE::BOTTOM * 4 + BR] = { { +0.5f, -0.5f + inner, -0.5f }, uv[BR] };

		sky_vbr = std::make_shared<KGL::Resource<SkyVertex>>(device, sky_vertices.size());
		auto* mapped_vertices = sky_vbr->Map(0, &CD3DX12_RANGE(0, 0));
		std::copy(sky_vertices.cbegin(), sky_vertices.cend(), mapped_vertices);
		sky_vbr->Unmap(0, &CD3DX12_RANGE(0, 0));

		auto buffer_location = sky_vbr->Data()->GetGPUVirtualAddress();
		for (auto& vbv : sky_vbv)
		{
			vbv.BufferLocation = buffer_location;
			vbv.SizeInBytes = sizeof(sky_vertices[0]) * 4;
			vbv.StrideInBytes = sizeof(sky_vertices[0]);
			buffer_location += vbv.SizeInBytes;
		}

		auto renderer_desc = KGL::_3D::Renderer::DEFAULT_DESC;
		renderer_desc.input_layouts.pop_back();
		renderer_desc.input_layouts.pop_back();
		renderer_desc.input_layouts.push_back(KGL::_3D::Renderer::INPUT_LAYOUTS[2]);
		renderer_desc.ps_desc.hlsl = "./HLSL/3D/UnNormal_ps.hlsl";
		renderer_desc.vs_desc.hlsl = "./HLSL/3D/UnNormal_vs.hlsl";
		renderer_desc.root_params.clear();
		D3D12_ROOT_PARAMETER param0 = { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			{ { KGL::SCAST<UINT>(KGL::_3D::Renderer::DESCRIPTOR_RANGES0.size()), KGL::_3D::Renderer::DESCRIPTOR_RANGES0.data() } },
			D3D12_SHADER_VISIBILITY_VERTEX
		};
		D3D12_ROOT_PARAMETER param1 = { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			{ { KGL::SCAST<UINT>(KGL::_3D::Renderer::DESCRIPTOR_RANGES2.size()), KGL::_3D::Renderer::DESCRIPTOR_RANGES2.data() } },
			D3D12_SHADER_VISIBILITY_PIXEL
		};
		renderer_desc.root_params.push_back(param0);
		renderer_desc.root_params.push_back(param1);

		renderer_desc.blend_types[0] = KGL::BLEND::TYPE::ALPHA;
		renderer_desc.rastarizer_desc.CullMode = D3D12_CULL_MODE_BACK;

		sky_renderer = std::make_shared<KGL::BaseRenderer>(device, renderer_desc);

		sky_buffer.Load(desc);

		
		sky_tex_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, CUBE::NUM * sky_tex_data.size());
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		
		for (auto& data : sky_tex_data)
		{
			for (int i = 0; i < CUBE::NUM; i++)
			{
				data.second->handle[i] = sky_tex_desc_mgr->Alloc();
				data.second->imgui_handle[i] = desc.imgui_heap_mgr->Alloc();
				srv_desc.Format = data.second->tex[i]->Data()->GetDesc().Format;
				device->CreateShaderResourceView(
					data.second->tex[i]->Data().Get(),
					&srv_desc,
					data.second->handle[i].Cpu()
				);
				device->CreateShaderResourceView(
					data.second->tex[i]->Data().Get(),
					&srv_desc,
					data.second->imgui_handle[i].Cpu()
				);
			}
		}
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
		sky_scale = 1000.f;
		XMMATRIX W, S, R, T;
		S = XMMatrixScaling(sky_scale, sky_scale, sky_scale);
		R = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
		T = XMMatrixTranslation(camera.eye.x, camera.eye.y, camera.eye.z);
		W = S * R * T;
		XMMATRIX WVP = W * view * proj_mat;
		XMStoreFloat4x4(sky_buffer.mapped_data, WVP);
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

	select_sky = sky_tex_data.begin()->second;
	use_gui = true;

	ResetCounterMax();
	time_scale = 1.f;

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
		ImGui::Begin("Camera");
		ImGui::SliderFloat3("pos", (float*)&camera.eye, -50.f, 50.f);
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
		ImGui::Begin("Sky");
		const auto window_size = desc.window->GetClientSize();
		const auto imgui_window_size = ImGui::GetWindowSize();
		ImGui::BeginChild("scrolling", ImVec2(imgui_window_size.x * 0.9f, std::max<float>(imgui_window_size.y - 100, 0)), ImGuiWindowFlags_NoTitleBar);
		for (auto& it : sky_tex_data)
		{
			ImGui::Image((ImTextureID)it.second->imgui_handle[CUBE::FRONT].Gpu().ptr, ImVec2(90, 90));
			ImGui::SameLine();
			if (it.second == select_sky)
			{
				ImGui::TextColored({ 0.8f, 0.8f, 0.8f, 1.f }, it.first.c_str());
			}
			else if (ImGui::Button(it.first.c_str()))
			{
				select_sky = it.second;
			}
		}
		ImGui::EndChild();
		ImGui::SliderFloat("Scale", &sky_scale, 1.f, 1000.f);
		{
			using namespace DirectX;
			XMMATRIX W, S, R, T;
			S = XMMatrixScaling(sky_scale, sky_scale, sky_scale);
			R = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
			T = XMMatrixTranslation(0.f, 0.f, 0.f);
			W = S * R * T;
			XMMATRIX WVP = W * KGL::CAMERA::GetView(camera) * XMLoadFloat4x4(&proj);
			XMStoreFloat4x4(sky_buffer.mapped_data, WVP);
		}
		ImGui::End();
	}

	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<TestScene03>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<TestScene05>(desc);

	if (input->IsKeyPressed(KGL::KEYS::BACKSPACE))
	{
		ResetCounterMax();
	}

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
	constexpr UINT spawn_late = 5000;
	KGL::Timer timer;
#ifdef USE_GPU
	{
		{
			auto* p_counter = particle_counter_res->Map(0, &CD3DX12_RANGE(0, 0));
			*p_counter = 0u;
			particle_counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
		}

		if (particle_total_num > 0)
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

#ifndef USE_GPU_OPTION1
		cpt_cmd_queue->Wait();
		cpt_cmd_allocator->Reset();
		cpt_cmd_list->Reset(cpt_cmd_allocator.Get(), nullptr);
#endif
	}
#else
	{
		auto particles = particle_resource->Map(0, &CD3DX12_RANGE(0, 0));
		const size_t i_max = particle_resource->Size();
		UINT ct = 0u;
		XMVECTOR resultant;
		const auto* cb = cpt_scene_buffer.mapped_data;
		for (int i = 0; i < i_max; i++)
		{
			if (!particles[i].Alive()) continue;
			particles[i].Update(elapsed_time, cb);
			ct++;
		}
		particle_resource->Unmap(0, &CD3DX12_RANGE(0, 0));
		KGLDebugOutPutStringNL("\r particle : " + std::to_string(ct) + std::string(10, ' '));
	}
#endif
#if !defined(USE_GPU_OPTION1) || ( defined(USE_GPU_OPTION1) && !defined(USE_GPU) )
	constexpr float spawn_elapsed = 1.f / spawn_late;

	spawn_counter += elapsed_time;
	if (spawn_counter >= spawn_elapsed)
	{
		float spawn_timer = spawn_counter - spawn_elapsed;
		UINT spawn_num = KGL::SCAST<UINT>(spawn_counter / spawn_elapsed);
		spawn_counter = std::fmodf(spawn_counter, spawn_elapsed);
		UINT spawn_count = 0u;
		const auto* cb = cpt_scene_buffer.mapped_data;
		for (int wi = 0; wi < 5; wi++)
		{
			D3D12_RANGE range;
			range.Begin = next_particle_offset;
			range.End = range.Begin + sizeof(Particle) * (spawn_num * 2);
			const size_t offset_max = sizeof(Particle) * (particle_resource->Size());
			const size_t bi = range.Begin / sizeof(Particle);
			const float rad_360min = XMConvertToRadians(360.f - FLT_MIN);
			if (range.End > offset_max)
				range.End = offset_max;

			{
				std::random_device rd;
				std::mt19937 mt(rd());
				std::uniform_real_distribution<float> score(-10.f, 10.f);
				std::uniform_real_distribution<float> unorm(0.f, 1.f);
				const auto i_max = (range.End - range.Begin) / sizeof(Particle);
				auto particles = particle_resource->Map(0u, &range);
				
				for (int i = 0; i < i_max; i++)
				{
					const size_t ti = bi + i;
					auto random_vec =
					XMVector3Transform(
						XMVectorSet(0.f, 0.f, 1.f, 0.f),
						XMMatrixRotationRollPitchYaw(0, unorm(mt) * rad_360min, 0)
					);

					if (particles[ti].exist_time > 0.f) continue;
					particles[ti].exist_time = 10.f;
					particles[ti].color = { 1.f, 0.5f, 0.0f, 0.1f };
					particles[ti].position = { 0.f, 0.f, 0.f };
					particles[ti].mass = 1.f;
					particles[ti].scale = 0.1f;
					XMStoreFloat3(&particles[ti].velocity, random_vec * 3.1);
					particles[ti].Update(spawn_timer, cb);
					spawn_timer -= spawn_elapsed;
					spawn_count++;
					if (spawn_count == spawn_num) break;
				}
				particle_resource->Unmap(0u, &range);
			}
			next_particle_offset = range.End;
			if (next_particle_offset == offset_max)
			{
				KGLDebugOutPutString("reset");
				next_particle_offset = 0u;
				if (spawn_count < spawn_num)
				{
					continue;
				}
			}
			break;
		}
	}
#else
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
			particle.exist_time = 20.f;
			particle.color = (i % 5 == 0) ? XMFLOAT4{ 1.f, 0.0f, 0.5f, 0.1f } : XMFLOAT4{ 1.0f, 0.5f, 0.0f, 0.1f };
			particle.position = { 0.f, 0.f, 0.f };
			particle.mass = 1.f;
			particle.scale = 0.1f;
			XMStoreFloat3(&particle.velocity, random_vec * 3.1);
			particle.Update(spawn_timer, cb);
			spawn_timer -= spawn_elapsed;
		}
	}

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
	if (particle_total_num > 0)
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
	UINT64 cputime = firework_update + map_update;

	{
		auto* p_counter = particle_counter_res->Map(0, &CD3DX12_RANGE(0, 0));
		*p_counter += frame_add_ptc_num;
		particle_total_num = *p_counter;
		KGLDebugOutPutStringNL("\r particle : " + std::to_string(*p_counter) + std::string(10, ' '));
		ImGui::Begin("Particle");
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
		ImGui::End();
		particle_counter_res->Unmap(0, &CD3DX12_RANGE(0, 0));
	}
#endif

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
	//{
	//	const auto& rbs_rt = rtvs->GetRtvResourceBarriers(true);
	//	const UINT rtv_size = KGL::SCAST<UINT>(rbs_rt.size());
	//	cmd_list->ResourceBarrier(rtv_size, rbs_rt.data());
	//	rtvs->SetAll(cmd_list, &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart());
	//	for (UINT i = 0u; i < rtv_size; i++) rtvs->Clear(cmd_list, clear_value, i);
	//	desc.app->ClearDsv(cmd_list);

	//	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//	board_renderer->SetState(cmd_list);
	//	cmd_list->SetDescriptorHeaps(1, scene_buffer.handle.Heap().GetAddressOf());
	//	cmd_list->SetGraphicsRootDescriptorTable(0, scene_buffer.handle.Gpu());
	//	cmd_list->SetDescriptorHeaps(1, b_cbv.Heap().GetAddressOf());
	//	cmd_list->SetGraphicsRootDescriptorTable(1, b_cbv.Gpu());
	//	cmd_list->SetDescriptorHeaps(1, b_srv.Heap().GetAddressOf());
	//	cmd_list->SetGraphicsRootDescriptorTable(0, b_srv.Gpu());

	//	board->Render(cmd_list);


	//	const auto& rbs_sr = rtvs->GetRtvResourceBarriers(false);
	//	cmd_list->ResourceBarrier(rtv_size, rbs_sr.data());
	//}
	{
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->SetRtvDsv(cmd_list);

		desc.app->ClearRtvDsv(cmd_list, clear_value);

		// SKY描画
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		sky_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, sky_buffer.handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, sky_buffer.handle.Gpu());
		for (int i = 0; i < CUBE::NUM; i++)
		{
			cmd_list->IASetVertexBuffers(0, 1, &sky_vbv[i]);
			cmd_list->SetDescriptorHeaps(1, select_sky->handle[i].Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(1, select_sky->handle[i].Gpu());
			cmd_list->DrawInstanced(4, 1, 0, 0);
		}

		// グリッド描画
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		grid_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, grid_buffer.handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, grid_buffer.handle.Gpu());
		cmd_list->IASetVertexBuffers(0, 1, &grid_vbv);
		cmd_list->IASetIndexBuffer(&grid_ibv);
		cmd_list->DrawIndexedInstanced(grid_idx_resource->Size(), 1, 0, 0, 0);

		// パーティクル描画
		if (particle_total_num > 0)
		{
			cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
			board_renderer->SetState(cmd_list);
			cmd_list->SetDescriptorHeaps(1, scene_buffer.handle.Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(0, scene_buffer.handle.Gpu());
			//cmd_list->SetDescriptorHeaps(1, b_cbv.Heap().GetAddressOf());
			//cmd_list->SetGraphicsRootDescriptorTable(1, b_cbv.Gpu());
			cmd_list->SetDescriptorHeaps(1, b_srv.Heap().GetAddressOf());
			cmd_list->SetGraphicsRootDescriptorTable(2, b_srv.Gpu());

			cmd_list->IASetVertexBuffers(0, 1, &b_vbv);
			cmd_list->DrawInstanced(SCAST<UINT>(particle_total_num), 1, 0, 0);
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

HRESULT TestScene04::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	for (auto& it : sky_tex_data)
	{
		for (int i = 0; i < CUBE::NUM; i++)
		{
			if (it.second)
			{
				desc.imgui_heap_mgr->Free(it.second->imgui_handle[i]);
			}
		}
	}
	return S_OK;
}