#include "../../Hrd/Scenes/Scenes.hpp"

#include <DirectXTex/d3dx12.h>
#include <Helper/Cast.hpp>
#include <Helper/Debug.hpp>
#include <Dx12/BlendState.hpp>
#include <Dx12/Helper.hpp>
#include <Math/Gaussian.hpp>
#include <random>

HRESULT TestScene07::Load(const SceneDesc& desc)
{
	using namespace DirectX;

	HRESULT hr = S_OK;
	const auto& device = desc.app->GetDevice();

	lgn = 5u;
	step_max = lgn;
	for (UINT32 i = 1u; i <= lgn; i++)
	{
		step_max += i;
	}

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
	value_rs[0] = std::make_shared<KGL::Resource<UINT32>>(device, 1u << lgn, &prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	value_rs[1] = std::make_shared<KGL::Resource<UINT32>>(device, 1u << lgn, &prop, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	uav_desc.Buffer.NumElements = KGL::SCAST<UINT>(value_rs[0]->Size());
	uav_desc.Buffer.StructureByteStride = sizeof(UINT32);

	cpt_descmgr = std::make_shared<KGL::DescriptorManager>(device, 3u + step_max);
	value_uav_handle[0] = std::make_shared<KGL::DescriptorHandle>(cpt_descmgr->Alloc());
	value_uav_handle[1] = std::make_shared<KGL::DescriptorHandle>(cpt_descmgr->Alloc());
	device->CreateUnorderedAccessView(value_rs[0]->Data().Get(), nullptr, &uav_desc, value_uav_handle[0]->Cpu());
	device->CreateUnorderedAccessView(value_rs[1]->Data().Get(), nullptr, &uav_desc, value_uav_handle[1]->Cpu());

	auto pipe_desc = KGL::ComputePipline::DEFAULT_DESC;
	pipe_desc.cs_desc.hlsl = "./HLSL/CPT/BitonicSort_cs.hlsl";
	auto ranges = KGL::ComputePipline::DESCRIPTOR_RANGES2;
	ranges[0].BaseShaderRegister = 1;

	auto rpmt = D3D12_ROOT_PARAMETER
		{	D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			{ { SCAST<UINT>(ranges.size()), ranges.data() } },
			D3D12_SHADER_VISIBILITY_ALL
		};

	pipe_desc.root_params.push_back(rpmt);
	cpt_pipeline = std::make_shared<KGL::ComputePipline>(device, desc.dxc, pipe_desc);

	frame_buff_rs = std::make_shared<KGL::Resource<UINT32>>(device, 1u);
	frame_cbv_handle = std::make_shared<KGL::DescriptorHandle>(cpt_descmgr->Alloc());
	frame_buff_rs->CreateCBV(frame_cbv_handle);

	step_cbv_handles.resize(step_max);
	step_buff_rs = std::make_shared<KGL::MultiResource<StepBuffer>>(device, step_max);
	for (UINT32 i = 0u; i < step_max; i++)
	{
		step_cbv_handles[i] = std::make_shared<KGL::DescriptorHandle>(cpt_descmgr->Alloc());
		step_buff_rs->CreateCBV(step_cbv_handles[i], i);
	}

	return hr;
}

HRESULT TestScene07::Init(const SceneDesc& desc)
{
	using namespace DirectX;

	auto value_size = SCAST<UINT32>(value_rs[rs_idx]->Size());
	rs_idx = 0u;
	{
		auto p = value_rs[rs_idx]->Map();
		for (UINT32 i = 0u; i < value_size; i++)
		{
			p[i] = SCAST<UINT32>(rand());
		}
		value_rs[rs_idx]->Unmap();
	}
	{
		auto p = frame_buff_rs->Map();
		*p = 20;
		frame_buff_rs->Unmap();
	}

	{
		auto p = step_buff_rs->Map();
		p->block_step = 1;
		p->sub_block_step = 1;
		step_buff_rs->Unmap();
	}

	OutputValue();

	return S_OK;
}

HRESULT TestScene07::Update(const SceneDesc& desc, float elapsed_time)
{
	// [←][→]キーでシーン移動
	auto input = desc.input;
	if (input->IsKeyPressed(KGL::KEYS::LEFT))
		SetNextScene<LoadScene00<TestScene04>>(desc);
	if (input->IsKeyPressed(KGL::KEYS::RIGHT))
		SetNextScene<LoadScene00<TestScene00>>(desc);

	using namespace DirectX;
	auto rb = CD3DX12_RESOURCE_BARRIER::UAV(value_rs[rs_idx]->Data().Get());
	cpt_cmd_list->ResourceBarrier(1, &rb);
	cpt_pipeline->SetState(cpt_cmd_list);

	UINT32 count = 0u;
	for (UINT32 i = 1u; i <= lgn; i++)
	{
		for (UINT j = 0; j < i; j++)
		{
			cpt_cmd_list->SetDescriptorHeaps(1, frame_cbv_handle->Heap().GetAddressOf());

			cpt_cmd_list->SetComputeRootDescriptorTable(0, frame_cbv_handle->Gpu());
			cpt_cmd_list->SetComputeRootDescriptorTable(1, step_cbv_handles[count++]->Gpu());
			cpt_cmd_list->SetComputeRootDescriptorTable(2, value_uav_handle[rs_idx]->Gpu());
			cpt_cmd_list->SetComputeRootDescriptorTable(3, value_uav_handle[rs_idx == 0 ? 1 : 0]->Gpu());
		}
		cpt_cmd_list->Dispatch((value_rs[rs_idx]->Size() / 64) + 1, 1, 1);
	}

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

	rs_idx++;
	if (rs_idx >= 2) rs_idx = 0;
	OutputValue();
	CountPlus();

	Sleep(200);

	return Render(desc);
}

void TestScene07::OutputValue()
{
	auto value_size = SCAST<UINT32>(value_rs[rs_idx]->Size());
	auto p = value_rs[rs_idx]->Map();
	for (UINT32 i = 0u; i < value_size; i++)
	{
		KGLDebugOutPutStringNL(std::to_string(p[i]) + ", ");
	}
	value_rs[rs_idx]->Unmap();

	KGLDebugOutPutStringNL("\n *\n *\n *\n *\n");
}

void TestScene07::CountPlus()
{
	auto p = step_buff_rs->Map();
	p->sub_block_step++;
	if (p->sub_block_step > p->block_step)
	{
		p->block_step++;
		p->sub_block_step = 1u;

		if (p->block_step == 5)
			p->block_step = 1;
	}
	step_buff_rs->Unmap();
}

HRESULT TestScene07::Render(const SceneDesc& desc)
{


	if (desc.app->IsTearingSupport())
		desc.app->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	else
		desc.app->GetSwapchain()->Present(1, 1);


	return S_OK;
}

HRESULT TestScene07::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	return S_OK;
}