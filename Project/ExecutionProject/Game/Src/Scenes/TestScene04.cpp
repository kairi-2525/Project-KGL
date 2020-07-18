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

	hr = scene_buffer.Load(desc);
	RCHECK(FAILED(hr), "SceneBaseDx12::Loadに失敗", hr);

	{
		constexpr auto clear_value = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
		rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
			device, desc.app->GetRtvBuffers().at(0), clear_value));
		rtv_textures.emplace_back(std::make_shared<KGL::Texture>(
			device, desc.app->GetRtvBuffers().at(0), clear_value));
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
	particle_resource = std::make_shared<KGL::Resource<Particle>>(device, 500000u, &prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	particle_desc_mgr = std::make_shared<KGL::DescriptorManager>(device, particle_resource->Size());
	particle_pipeline = std::make_shared<KGL::ComputePipline>(device);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.Buffer.NumElements = KGL::SCAST<UINT>(particle_resource->Size());
	uav_desc.Buffer.StructureByteStride = sizeof(Particle);

	particle_begin_handle = particle_desc_mgr->Alloc();
	device->CreateUnorderedAccessView(particle_resource->Data().Get(), nullptr, &uav_desc, particle_begin_handle.Cpu());

	return hr;
}

HRESULT TestScene04::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto window_size = desc.window->GetClientSize();

	camera.eye = { 0.f, 10.f, -15.f };
	camera.focus_vec = { 0.f, 0.f, 15.f };
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

	return S_OK;
}

HRESULT TestScene04::Update(const SceneDesc& desc, float elapsed_time)
{
	using namespace DirectX;

	{
		auto& svproj = scene_buffer.mapped_data->view_proj;
		auto& svre_view = scene_buffer.mapped_data->re_view;
		XMMATRIX view = KGL::CAMERA::GetView(camera);
		XMStoreFloat4x4(&svproj, view * XMLoadFloat4x4(&proj));
		XMFLOAT4X4 viewf;
		XMStoreFloat4x4(&viewf, view);
		viewf._41 = 0.f; viewf._42 = 0.f; viewf._43 = 0.f;
		XMStoreFloat4x4(&svre_view, XMMatrixInverse(nullptr, XMLoadFloat4x4(&viewf)));
		scene_buffer.mapped_data->elapsed_time = elapsed_time;
	}

	D3D12_RANGE range;
	range.Begin = next_particle_offset;
	range.End = range.Begin + sizeof(Particle) * 10;
	const size_t offset_max = sizeof(Particle) * (particle_resource->Size() - 1);
	if (range.End > offset_max)
		range.End = offset_max;

	{
		std::random_device rd;
		std::mt19937 mt(rd());
		std::uniform_real_distribution<float> score(-1.f, 1.f);

		auto particles = particle_resource->Map(0u, &range);
		for (int i = 0; i < 10; i++)
		{
			if (particles[i].exist) continue;
			particles[i].exist = true;
			particles[i].position = { 0.f, 0.f, 0.f };
			particles[i].velocity = { score(mt), score(mt), 1.f };
		}
		particle_resource->Unmap(0u, &range);
	}
	next_particle_offset = range.End;
	if (next_particle_offset == offset_max)
		next_particle_offset = 0u;

	particle_pipeline->SetState(cpt_cmd_list);

	cpt_cmd_list->SetDescriptorHeaps(1, scene_buffer.handle.Heap().GetAddressOf());
	cpt_cmd_list->SetComputeRootDescriptorTable(0, scene_buffer.handle.Gpu());
	cpt_cmd_list->SetDescriptorHeaps(1, particle_begin_handle.Heap().GetAddressOf());
	cpt_cmd_list->SetComputeRootDescriptorTable(0, particle_begin_handle.Gpu());
	const UINT ptcl_size = KGL::SCAST<UINT>(particle_resource->Size());
	DirectX::XMUINT3 patch = {};
	constexpr auto patch_max = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	patch.x = (patch_max / 64) + (patch_max % 64);
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
	{
		const auto& rbs_rt = rtvs->GetRtvResourceBarriers(true);
		const UINT rtv_size = KGL::SCAST<UINT>(rbs_rt.size());
		cmd_list->ResourceBarrier(rtv_size, rbs_rt.data());
		rtvs->SetAll(cmd_list, &desc.app->GetDsvHeap()->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0u; i < rtv_size; i++) rtvs->Clear(cmd_list, clear_value, i);
		desc.app->ClearDsv(cmd_list);

		const auto& rbs_sr = rtvs->GetRtvResourceBarriers(false);
		cmd_list->ResourceBarrier(rtv_size, rbs_sr.data());
	}
	{
		cmd_list->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->SetRtv(cmd_list);

		desc.app->ClearRtv(cmd_list, clear_value);

		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		sprite_renderer->SetState(cmd_list);

		cmd_list->SetDescriptorHeaps(1, rtvs->GetSRVHeap().GetAddressOf());
		cmd_list->SetGraphicsRootDescriptorTable(0, rtvs->GetSRVGPUHandle(0));

		sprite->Render(cmd_list);

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