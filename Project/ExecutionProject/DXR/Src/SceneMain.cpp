#include "../Hrd/Scene.hpp"
#include <Helper/Cast.hpp>
#include <DirectXTex/d3dx12.h>
#include <Dx12/Helper.hpp>
#include <Helper/ThrowAssert.hpp>

#include "../DXRHelper/DXRHelper.h"
#include "../DXRHelper/nv_helpers_dx12/BottomLevelASGenerator.h"

// GPU���������̒��_�o�b�t�@�[�̃��X�g�Ƃ��̒��_���Ɋ�Â��āA
// �ŉ��ʃ��x���̉����\�����쐬���܂��B �r���h��3�̃X�e�b�v�ōs���܂��B
// �W�I���g���̎��W�A�K�v�ȃo�b�t�@�̃T�C�Y�̌v�Z�A���ۂ�AS�̃r���h�ł��B
SceneMain::AccelerationStructureBuffers SceneMain::CreateBottomLevelAS(
	const std::vector<std::pair<KGL::ComPtr<ID3D12Resource>, uint32_t>>& vertex_buffers)
{
	nv_helpers_dx12::BottomLevelASGenerator bottom_level_as;
	// ���ׂĂ̒��_�o�b�t�@�[��ǉ����A�����̈ʒu��ϊ����܂���B
	for (const auto& buffer : vertex_buffers)
	{
		bottom_level_as.AddVertexBuffer(buffer.first.Get(), 0, buffer.second, sizeof(TriangleVertex), 0, 0);
	}

	// AS�r���h�ɂ́A�ꎞ�I�ȏ����i�[���邽�߂̃X�N���b�`�̈悪�K�v�ł��B
	// �X�N���b�`�������̗ʂ́A�V�[���̕��G���Ɉˑ����܂��B
	UINT64 scratch_size_in_bytes = 0;
	// �Ō��AS�́A�����̒��_�o�b�t�@�[�ɉ����ĕۑ�����K�v������܂��B �T�C�Y�̓V�[���̕��G���ɂ��ˑ����܂��B
	UINT64 result_size_in_bytes = 0;

	bottom_level_as.ComputeASBufferSizes(device5.Get(), false, &scratch_size_in_bytes, &result_size_in_bytes);

	// �T�C�Y���擾�����ƁA�A�v���P�[�V�����͕K�v�ȃo�b�t�@�����蓖�Ă�K�v������܂��B
	// �����S�̂�GPU�ōs���邽�߁A�f�t�H���g�q�[�v�ɒ��ڊ��蓖�Ă邱�Ƃ��ł��܂��B
	AccelerationStructureBuffers buffers;
	buffers.scratch.Attach(nv_helpers_dx12::CreateBuffer(device5.Get(), scratch_size_in_bytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
		nv_helpers_dx12::kDefaultHeapProps));
	buffers.result.Attach(nv_helpers_dx12::CreateBuffer(device5.Get(), result_size_in_bytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps));

	// �����\�����\�z���܂��B ���̌Ăяo���͐������ꂽAS�̃o���A�𓝍����邽�߁A
	// ���̃��\�b�h�̒���Ƀg�b�v���x����AS���v�Z���邽�߂Ɏg�p�ł��邱�Ƃɒ��ӂ��Ă��������B
	bottom_level_as.Generate(cmd_list4.Get(), buffers.scratch.Get(), buffers.result.Get(), false, nullptr);

	return buffers;
}

// �V�[���̂��ׂẴC���X�^���X��ێ����郁�C���̉����\�����쐬���܂��B 
// �ŉ��ʂ�AS�����Ɠ��l�ɁA�C���X�^���X�̎��W�AAS�̃������v���̌v�Z�A
// �����AS���̂̍\�z��3�̃X�e�b�v�ōs���܂��B
void SceneMain::CreateTopLevelAS(
	const std::vector<std::pair<KGL::ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>
	&instances // �C���X�^���X�̍ŉ���AS�ƃ}�g���b�N�X�̃y�A
)
{
	// ���ׂẴC���X�^���X���r���_�[�w���p�[�Ɏ��W���܂�
	for (size_t i = 0; i < instances.size(); i++)
	{
		top_level_as_generator.AddInstance(
			instances[i].first.Get(), instances[i].second,
			KGL::SCAST<UINT>(i), 0u);
	}

	// �ŉ��ʂ�AS�Ɠ��l�ɁAAS�̍\�z�ɂ́A���ۂ�AS�ɉ����Ĉꎞ�f�[�^���i�[���邽�߂̃X�N���b�`�̈悪�K�v�ł��B
	// �g�b�v���x��AS�̏ꍇ�A�C���X�^���X�L�q�q��GPU�������Ɋi�[����K�v������܂��B
	// ���̌Ăяo���́A�A�v���P�[�V�������Ή����郁���������蓖�Ă邱�Ƃ��ł���悤�ɁA
	// ���ꂼ��̃������v���i�X�N���b�`�A���ʁA�C���X�^���X�L�q�q�j���o�͂��܂�
	UINT64 scratch_size, result_size, instance_descs_size;
	top_level_as_generator.ComputeASBufferSizes(device5.Get(), true,
		&scratch_size, &result_size, &instance_descs_size);

	// �X�N���b�`�o�b�t�@�ƌ��ʃo�b�t�@���쐬���܂��B
	// �r���h�͂��ׂ�GPU�ōs���邽�߁A�f�t�H���g�̃q�[�v�Ɋ��蓖�Ă邱�Ƃ��ł��܂�
	top_level_buffers.scratch.Attach(nv_helpers_dx12::CreateBuffer(
		device5.Get(), scratch_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nv_helpers_dx12::kDefaultHeapProps));

	top_level_buffers.result.Attach(nv_helpers_dx12::CreateBuffer(
		device5.Get(), result_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nv_helpers_dx12::kDefaultHeapProps));

	// �C���X�^���X���������o�b�t�@�[�FID�A�V�F�[�_�[�o�C���f�B���O���A�}�g���b�N�X...
	// �����̓}�b�s���O��ʂ��ăw���p�[�ɂ���ăo�b�t�@�[�ɃR�s�[����邽�߁A
	// �A�b�v���[�h�q�[�v�Ƀo�b�t�@�[�����蓖�Ă�K�v������܂��B
	top_level_buffers.instance_desc.Attach(nv_helpers_dx12::CreateBuffer(
		device5.Get(), instance_descs_size, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nv_helpers_dx12::kUploadHeapProps));

	// ���ׂẴo�b�t�@�����蓖�Ă�ꂽ��A�܂��͍X�V�݂̂��K�v�ȏꍇ�́A�����\�����\�z�ł��܂��B
	// �X�V�̏ꍇ�A������AS���u�ȑO�́vAS�Ƃ��Ă��n�����߁A
	// ����̈ʒu�ɍăt�B�b�g�ł��邱�Ƃɒ��ӂ��Ă��������B
	top_level_as_generator.Generate(cmd_list4.Get(),
		top_level_buffers.scratch.Get(),
		top_level_buffers.result.Get(),
		top_level_buffers.instance_desc.Get()
	);
}

// BLAS�r���h��TLAS�r���h��g�ݍ��킹�āA�V�[���̃��C�g���[�X�ɕK�v�ȉ����\���S�̂��\�z���܂�
void SceneMain::CreateAccelerationStructures(const std::shared_ptr<KGL::CommandQueue>& queue)
{
	// �O�p�`�̒��_�o�b�t�@�[���牺��AS���쐬���܂�
	AccelerationStructureBuffers bottom_level_buffers =
		CreateBottomLevelAS({ {t_vert_res->Data().Get(), 3} });

	// �����_�ł�1�̃C���X�^���X�̂�
	instances = { { bottom_level_buffers.result, DirectX::XMMatrixIdentity() } };
	CreateTopLevelAS(instances);

	// �R�}���h���X�g���t���b�V�����A�I������܂ő҂��܂�
	cmd_list4->Close();
	ID3D12CommandList* cmd_lists[] = { cmd_list4.Get() };
	queue->Data()->ExecuteCommandLists(1, cmd_lists);
	queue->Signal();
	queue->Wait();

	// �R�}���h���X�g�̎��s������������A��������Z�b�g���ă����_�����O�ɍė��p���܂�
	cmd_allocator->Reset();
	cmd_list4->Reset(cmd_allocator.Get(), nullptr);

	// AS�o�b�t�@�[��ۊǂ��܂��B �֐����I������ƁA�c��̃o�b�t�@���������܂�
	bottom_level_as = bottom_level_buffers.result;
}

HRESULT SceneMain::Load(const SceneDesc& desc)
{
	HRESULT hr = S_OK;
	try
	{
		if (!desc.app->IsDXRSupport())
			throw std::runtime_error("���̊��ł�DXR�����s�ł��܂���B");
	}
	catch (std::runtime_error& exception)
	{
		KGL::RuntimeErrorStop(exception);
	}

	const auto& device = desc.app->GetDevice();
	KGL::ComPtr<ID3D12GraphicsCommandList> cmd_list;
	hr = KGL::DX12::HELPER::CreateCommandAllocatorAndList<ID3D12GraphicsCommandList>(device, &cmd_allocator, &cmd_list);
	RCHECK(FAILED(hr), "�R�}���h�A���P�[�^�[/���X�g�̍쐬�Ɏ��s", hr);

	hr = device->QueryInterface(IID_PPV_ARGS(device5.GetAddressOf()));
	RCHECK(FAILED(hr), "device5�̍쐬�Ɏ��s", hr);
	hr = cmd_list->QueryInterface(IID_PPV_ARGS(cmd_list4.GetAddressOf()));
	RCHECK(FAILED(hr), "�R�}���h���X�g4�̍쐬�Ɏ��s", hr);

	{
		t_vert_res = std::make_shared<KGL::Resource<TriangleVertex>>(device, 3);
		std::vector<TriangleVertex> vertex(3);
		vertex[0] = { { +0.0f, +0.7f, +0.0f }, { 1.f, 1.f, 0.f, 1.f } };
		vertex[1] = { { +0.7f, -0.7f, +0.0f }, { 0.f, 1.f, 1.f, 1.f } };
		vertex[2] = { { -0.7f, -0.7f, +0.0f }, { 1.f, 0.f, 1.f, 1.f } };

		auto* mapped_vertices = t_vert_res->Map(0, &CD3DX12_RANGE(0, 0));
		std::copy(vertex.cbegin(), vertex.cend(), mapped_vertices);
		t_vert_res->Unmap(0, &CD3DX12_RANGE(0, 0));

		t_vert_view.BufferLocation = t_vert_res->Data()->GetGPUVirtualAddress();
		t_vert_view.StrideInBytes = sizeof(TriangleVertex);
		t_vert_view.SizeInBytes = sizeof(TriangleVertex) * vertex.size();

		auto renderer_desc = KGL::_2D::Renderer::DEFAULT_DESC;
		renderer_desc.vs_desc.hlsl = "./HLSL/2D/Triangle_vs.hlsl";
		renderer_desc.ps_desc.hlsl = "./HLSL/2D/Triangle_ps.hlsl";
		renderer_desc.vs_desc.version = "vs_5_1";
		renderer_desc.ps_desc.version = "ps_5_1";

		renderer_desc.input_layouts.clear();
		renderer_desc.input_layouts.push_back({
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.input_layouts.push_back({
			"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		renderer_desc.root_params.clear();
		renderer_desc.static_samplers.clear();
		t_renderer = std::make_shared<KGL::BaseRenderer>(device, renderer_desc);
	}

	{
		// ���C�g���[�V���O�p�̉����\���iAS�j���Z�b�g�A�b�v���܂��B
		// �W�I���g����ݒ肷��ꍇ�A�ŉ��ʂ̊eAS�ɂ͓Ǝ��̕ϊ��s�񂪂���܂��B
		CreateAccelerationStructures(desc.app->GetQueue());
	}

	return hr;
}

HRESULT SceneMain::Init(const SceneDesc& desc)
{
	HRESULT hr = S_OK;

	raster = true;

	return hr;
}

HRESULT SceneMain::Update(const SceneDesc& desc, float elapsed_time)
{
	HRESULT hr = S_OK;

	const auto& input = desc.input;
	
	if (input->IsKeyPressed(KGL::KEYS::SPACE))
	{
		raster = !raster;
	}

	return Render(desc);
}

HRESULT SceneMain::Render(const SceneDesc& desc)
{
	HRESULT hr = S_OK;

	using KGL::SCAST;
	auto window_size = desc.window->GetClientSize();

	D3D12_VIEWPORT viewport = {};
	viewport.Width = SCAST<FLOAT>(window_size.x);
	viewport.Height = SCAST<FLOAT>(window_size.y);
	viewport.TopLeftX = 0;//�o�͐�̍�����WX
	viewport.TopLeftY = 0;//�o�͐�̍�����WY
	viewport.MaxDepth = 1.0f;//�[�x�ő�l
	viewport.MinDepth = 0.0f;//�[�x�ŏ��l

	auto scissorrect = CD3DX12_RECT(
		0, 0,
		window_size.x, window_size.y
	);

	cmd_list4->RSSetViewports(1, &viewport);
	cmd_list4->RSSetScissorRects(1, &scissorrect);

	if (raster)
	{
		desc.app->SetRtvDsv(cmd_list4);
		cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtvDsv(cmd_list4, DirectX::XMFLOAT4(0.0f, 0.2f, 0.4f, 1.f));

		cmd_list4->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		t_renderer->SetState(cmd_list4);
		cmd_list4->IASetVertexBuffers(0, 1, &t_vert_view);
		cmd_list4->DrawInstanced(3, 1, 0, 0);

		cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));
	}
	else
	{
		desc.app->SetRtvDsv(cmd_list4);
		cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(true));
		desc.app->ClearRtvDsv(cmd_list4, DirectX::XMFLOAT4(0.6f, 0.8f, 0.4f, 1.f));

		cmd_list4->ResourceBarrier(1, &desc.app->GetRtvResourceBarrier(false));
	}

	cmd_list4->Close();
	ID3D12CommandList* cmd_list4s[] = { cmd_list4.Get() };
	desc.app->GetQueue()->Data()->ExecuteCommandLists(1, cmd_list4s);
	desc.app->GetQueue()->Signal();
	desc.app->GetQueue()->Wait();

	cmd_allocator->Reset();
	cmd_list4->Reset(cmd_allocator.Get(), nullptr);

	if (desc.app->IsTearingSupport())
		desc.app->GetSwapchain()->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	else
		desc.app->GetSwapchain()->Present(1, 1);

	return hr;
}

HRESULT SceneMain::UnInit(const SceneDesc& desc, std::shared_ptr<SceneBase> next_scene)
{
	HRESULT hr = S_OK;
	return hr;
}