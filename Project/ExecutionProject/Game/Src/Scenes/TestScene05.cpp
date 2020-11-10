#include "../../Hrd/Scenes/Scenes.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>
#include <random>

HRESULT TestScene05::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

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

	D3D12_HEAP_PROPERTIES prop = {};
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	prop.CreationNodeMask = 1;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	prop.Type = D3D12_HEAP_TYPE_CUSTOM;
	prop.VisibleNodeMask = 1;
	cpt_resource = std::make_shared<KGL::Resource<PTC>>(device, 1u, &prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.Buffer.NumElements = KGL::SCAST<UINT>(cpt_resource->Size());
	uav_desc.Buffer.StructureByteStride = sizeof(PTC);

	cpt_descmgr = std::make_shared<KGL::DescriptorManager>(device, 1u);
	cpt_handle = cpt_descmgr->Alloc();
	device->CreateUnorderedAccessView(cpt_resource->Data().Get(), nullptr, &uav_desc, cpt_handle.Cpu());
	
	auto pipe_desc = KGL::ComputePipline::DEFAULT_DESC;
	//pipe_desc.cs_desc.hlsl = "./HLSL/3D/ComputeShader.hlsl";
	//pipe_desc.cs_desc.entry_point = "main";
	//pipe_desc.root_params.clear();
	//pipe_desc.root_params.push_back(
	//	{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
	//	{ { KGL::SCAST<UINT>(KGL::ComputePipline::DESCRIPTOR_RANGES1.size()), KGL::ComputePipline::DESCRIPTOR_RANGES1.data() } },
	//	D3D12_SHADER_VISIBILITY_ALL }
	//);
	cpt_pipeline = std::make_shared<KGL::ComputePipline>(device, desc.dxc, pipe_desc);

	return hr;
}

HRESULT TestScene05::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	{
		auto p = cpt_resource->Map();
		p[0].a.x = 10;
		p[0].b.x = -10;
		cpt_resource->Unmap();
	}

	return S_OK;
}

HRESULT TestScene05::Update(const SceneDesc& desc, float elapsed_time)
{
	auto input = desc.input;
	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<LoadScene00<TestScene04>>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<LoadScene00<TestScene00>>(desc);

	using namespace DirectX;
	auto rb = CD3DX12_RESOURCE_BARRIER::UAV(cpt_resource->Data().Get());
	cpt_cmd_list->ResourceBarrier(1, &rb);
	cpt_pipeline->SetState(cpt_cmd_list);
	cpt_cmd_list->SetDescriptorHeaps(1, cpt_handle.Heap().GetAddressOf());
	cpt_cmd_list->SetComputeRootDescriptorTable(1, cpt_handle.Gpu());

	cpt_cmd_list->Dispatch(1, 1, 1);

	cpt_cmd_list->ResourceBarrier(1, &rb);
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

	{
		D3D12_RANGE range;
		range.Begin = 0;
		range.End = cpt_resource->SizeInBytes();
		auto p = cpt_resource->Map(0, &range);
		KGLDebugOutPutStringNL("\r a : " + std::to_string(p[0].a.x) + " / b : " + std::to_string(p[0].b.x) + std::string(5, ' '));
		cpt_resource->Unmap(0, &range);
	}

	return Render(desc);
}

HRESULT TestScene05::Render(const SceneDesc& desc)
{


	if (desc.app->IsTearingSupport())
		desc.app->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	else
		desc.app->GetSwapchain()->Present(1, 1);

	
	return S_OK;
}

HRESULT TestScene05::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	return S_OK;
}