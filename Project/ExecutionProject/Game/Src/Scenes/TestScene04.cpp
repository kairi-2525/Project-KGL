#include "../../Hrd/Scenes/TestScene04.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>
#include <random>

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
			"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"SCALE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
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
	particle_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, particle_resource->Size());
	particle_pipeline = std::make_shared<KGL::ComputePipline>(device);
	b_cbv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	matrix_resource = std::make_shared<KGL::Resource<CbvParam>>(device, 1u);
	b_texture = std::make_shared<KGL::Texture>(device, "./Assets/PNG.png");
	b_srv_descmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.Buffer.NumElements = KGL::SCAST<UINT>(particle_resource->Size());
	uav_desc.Buffer.StructureByteStride = sizeof(Particle);

	particle_begin_handle = particle_desc_mgr->Alloc();
	device->CreateUnorderedAccessView(particle_resource->Data().Get(), nullptr, &uav_desc, particle_begin_handle.Cpu());

	/*
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = particle_resource->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = particle_resource->SizeInBytes();
		b_cbv = b_cbv_descmgr->Alloc();
		device->CreateConstantBufferView(&cbv_desc, b_cbv.Cpu());
	}
	/*{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = matrix_resource->Data()->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = matrix_resource->SizeInBytes();
		b_cbv = b_cbv_descmgr->Alloc();
		device->CreateConstantBufferView(&cbv_desc, b_cbv.Cpu());
	}*/
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

	return hr;
}

HRESULT TestScene04::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();

	camera.eye = { 0.f, 0.f, -15.f };
	camera.focus_vec = { 0.f, 0.f, 1.f };
	camera.up = { 0.f, 1.f, 0.f };

	//XMStoreFloat3(&scene_buffer.mapped_data->light_vector, XMVector3Normalize(XMVectorSet(+0.2f, -0.7f, 0.5f, 0.f)));

	const XMMATRIX proj_mat = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(70.f),	// FOV
		static_cast<float>(window_size.x) / static_cast<float>(window_size.y),	// アスペクト比
		1.0f, 100.0f // near, far
	);
	XMStoreFloat4x4(&proj, proj_mat);
	auto* p_particles = particle_resource->Map();
	Particle particle_base = {};

	std::fill(&p_particles[0], &p_particles[particle_resource->Size()], particle_base);
	particle_resource->Unmap();

	next_particle_offset = 0u;

	{
		XMMATRIX W, S, R, T;
		S = XMMatrixScaling(1.f, 1.f, 1.f);
		R = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);
		T = XMMatrixTranslation(0.f, 0.f, -1.f);
		W = S * R * T;

		XMMATRIX WVP = W * KGL::CAMERA::GetView(camera) * proj_mat;
		CbvParam* param = matrix_resource->Map();
		XMStoreFloat4x4(&param->mat, WVP);
		param->color = { 1.f, 1.f, 1.f, 1.f };
		matrix_resource->Unmap();
	}

	spawn_counter = 0.f;

	return S_OK;
}

HRESULT TestScene04::Update(const SceneDesc& desc, float elapsed_time)
{
	using namespace DirectX;

	{
		auto& svproj = cpt_scene_buffer.mapped_data->view_proj;
		auto& svre_view = cpt_scene_buffer.mapped_data->re_view;
		XMMATRIX view = KGL::CAMERA::GetView(camera);
		XMStoreFloat4x4(&svproj, view * XMLoadFloat4x4(&proj));
		XMFLOAT4X4 viewf;
		XMStoreFloat4x4(&viewf, view);
		viewf._41 = 0.f; viewf._42 = 0.f; viewf._43 = 0.f;
		XMStoreFloat4x4(&svre_view, XMMatrixInverse(nullptr, XMLoadFloat4x4(&viewf)));
		cpt_scene_buffer.mapped_data->elapsed_time = elapsed_time;

		scene_buffer.mapped_data->view = view;
		scene_buffer.mapped_data->proj = XMLoadFloat4x4(&proj);
		scene_buffer.mapped_data->eye = camera.eye;
		scene_buffer.mapped_data->light_vector = { 0.f, 0.f, 1.f };
	}

#if 1
	{
		particle_pipeline->SetState(cpt_cmd_list);

		cpt_cmd_list->SetDescriptorHeaps(1, cpt_scene_buffer.handle.Heap().GetAddressOf());
		cpt_cmd_list->SetComputeRootDescriptorTable(0, cpt_scene_buffer.handle.Gpu());
		cpt_cmd_list->SetDescriptorHeaps(1, particle_begin_handle.Heap().GetAddressOf());
		cpt_cmd_list->SetComputeRootDescriptorTable(1, particle_begin_handle.Gpu());
		const UINT ptcl_size = KGL::SCAST<UINT>(particle_resource->Size());
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
		cpt_cmd_queue->Wait();

		cpt_cmd_allocator->Reset();
		cpt_cmd_list->Reset(cpt_cmd_allocator.Get(), nullptr);
	}
#else
	{
		auto particles = particle_resource->Map();
		const size_t i_max = particle_resource->Size();
		for (int i = 0; i < i_max; i++)
		{
			if (particles[i].exist_time <= 0.f) continue;
			XMVECTOR pos = XMLoadFloat3(&particles[i].position);
			XMVECTOR vel = XMLoadFloat3(&particles[i].velocity);
			pos += vel * elapsed_time;
			XMStoreFloat3(&particles[i].position, pos);
			particles[i].exist_time -= elapsed_time;
		}
		particle_resource->Unmap();
	}
#endif

	// 一秒秒間に[spawn_late]個のパーティクルを発生させる
	constexpr UINT spawn_late = 10000;
	constexpr float spawn_elapsed = 1.f / spawn_late;

	spawn_counter += elapsed_time;
	if (spawn_counter >= spawn_elapsed)
	{
		UINT spawn_num = KGL::SCAST<UINT>(spawn_counter / spawn_elapsed);
		spawn_counter = std::fmodf(spawn_counter, spawn_elapsed);
		UINT spawn_count = 0u;

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
						XMMatrixRotationRollPitchYaw(unorm(mt) * rad_360min, unorm(mt) * rad_360min, unorm(mt) * rad_360min)
					);

					if (particles[ti].exist_time > 0.f) continue;
					particles[ti].exist_time = 10.f;
					particles[ti].position = { 0.f, 0.f, 0.f };
					particles[ti].scale = { 0.1f, 0.1f, 0.f };
					XMStoreFloat3(&particles[ti].velocity, random_vec);
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
					spawn_num = spawn_num - spawn_count;
					spawn_count = 0u;
					continue;
				}
			}
			break;
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
	constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
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

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		board_renderer->SetState(cmd_list);
		cmd_list->SetDescriptorHeaps(1, scene_buffer.handle.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, scene_buffer.handle.Gpu());
		//cmd_list->SetDescriptorHeaps(1, b_cbv.Heap().GetAddressOf());
		//cmd_list->SetGraphicsRootDescriptorTable(1, b_cbv.Gpu());
		cmd_list->SetDescriptorHeaps(1, b_srv.Heap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(2, b_srv.Gpu());

		cmd_list->IASetVertexBuffers(0, 1, &b_vbv);
		cmd_list->DrawInstanced(SCAST<UINT>(particle_resource->Size()), 1, 0, 0);

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

HRESULT TestScene04::UnInit(const SceneDesc& desc)
{
	return S_OK;
}