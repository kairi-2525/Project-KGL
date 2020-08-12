#include "../Hrd/Scene.hpp"
#include <Helper/Cast.hpp>
#include <DirectXTex/d3dx12.h>
#include <Dx12/Helper.hpp>
#include <Helper/ThrowAssert.hpp>

#include "../DXRHelper/DXRHelper.h"
#include "../DXRHelper/nv_helpers_dx12/BottomLevelASGenerator.h"
#include "../DXRHelper/nv_helpers_dx12/RaytracingPipelineGenerator.h"
#include "../DXRHelper/nv_helpers_dx12/RootSignatureGenerator.h"

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

// ���������V�F�[�_�[��2�̃��\�[�X�ɃA�N�Z�X����K�v������܂��F�����ǐՏo�͂ƍŏ�ʂ̉����\��
KGL::ComPtr<ID3D12RootSignature> SceneMain::CreateRayGenSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter(
		{
			{
				0 /*u0*/, 1 /*1 descriptor*/, 0 /*�Öق̃��W�X�^�X�y�[�X0���g�p����*/,
				D3D12_DESCRIPTOR_RANGE_TYPE_UAV /*�o�̓o�b�t�@�[��\��UAV*/,
				0 /*UAV����`����Ă���q�[�v�X���b�g*/
			},
			{
				0 /*t0*/, 1, 0,
				D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*�g�b�v���x���̉����\��*/,
				1
			}
		}
	);
	KGL::ComPtr<ID3D12RootSignature> rs;
	rs.Attach(rsc.Generate(device5.Get(), true));
	rs->SetName(L"RayGenSignature");
	return rs;
}

// �~�X�V�F�[�_�[�̓��C�y�C���[�h����Ă̂ݒʐM���邽�߁A���\�[�X��K�v�Ƃ��܂���
KGL::ComPtr<ID3D12RootSignature> SceneMain::CreateMissSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	KGL::ComPtr<ID3D12RootSignature> rs;
	rs.Attach(rsc.Generate(device5.Get(), true));
	rs->SetName(L"MissSignature");
	return rs;
}

// �q�b�g�V�F�[�_�[�̓��C�y�C���[�h����Ă̂ݒʐM���邽�߁A���\�[�X��K�v�Ƃ��܂���
KGL::ComPtr<ID3D12RootSignature> SceneMain::CreateHitSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	KGL::ComPtr<ID3D12RootSignature> rs;
	rs.Attach(rsc.Generate(device5.Get(), true));
	rs->SetName(L"HitSignature");
	return rs;
}


// ���C�g���[�V���O�p�C�v���C���́A�V�F�[�_�[�R�[�h�A���[�g�V�O�l�`���A�p�C�v���C���������A
// DXR���V�F�[�_�[���Ăяo���A���C�g���[�V���O���Ɉꎞ���������Ǘ����邽�߂Ɏg�p����P��̍\���Ƀo�C���h���܂��B
void SceneMain::CreateRaytracingPipeline()
{
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(device5.Get());

	// �p�C�v���C���ɂ́A���C�g���[�V���O�v���Z�X���ɐ��ݓI�Ɏ��s����邷�ׂẴV�F�[�_�[��DXIL�R�[�h���܂܂�Ă��܂��B
	// ���̃Z�N�V�����ł́AHLSL�R�[�h����A��DXIL���C�u�����ɃR���p�C�����܂��B
	// ���m�ɂ��邽�߂ɁA�������̃��C�u�����̃R�[�h���Z�}���e�B�b�N
	//�i���������A�q�b�g�A�~�X�j�ŕ������邱�Ƃ�I�����܂����B �C�ӂ̃R�[�h���C�A�E�g���g�p�ł��܂��B
	ray_gen_library.Attach(nv_helpers_dx12::CompileShaderLibrary(L"./HLSL/DXR/RayGen.hlsl"));
	miss_library.Attach(nv_helpers_dx12::CompileShaderLibrary(L"./HLSL/DXR/Miss.hlsl"));
	hit_library.Attach(nv_helpers_dx12::CompileShaderLibrary(L"./HLSL/DXR/Hit.hlsl"));

	// DLL�Ɠ��l�ɁA�e���C�u�����̓G�N�X�|�[�g���ꂽ�����̃V���{���Ɋ֘A�t�����Ă��܂��B
	// ����́A�ȉ��̍s�Ŗ����I�ɍs���K�v������܂��B 
	// �P��̃��C�u�����ɂ͔C�ӂ̐��̃V���{�����܂߂邱�Ƃ��ł��A
	// ���̈Ӗ���[shader�i "xxx"�j]�\�����g�p����HLSL�Ŏw�肳��܂��B
	pipeline.AddLibrary(ray_gen_library.Get(), { L"RayGen" });
	pipeline.AddLibrary(miss_library.Get(), { L"Miss" });
	pipeline.AddLibrary(hit_library.Get(), { L"ClosestHit" });

	// �eDX12�V�F�[�_�[���g�p����ɂ́A�A�N�Z�X����p�����[�^�[�ƃo�b�t�@�[���`���郋�[�g�������K�v�ł��B
	ray_gen_signature = CreateRayGenSignature();
	miss_signature = CreateMissSignature();
	hit_signature = CreateHitSignature();

	// 3�̈قȂ�V�F�[�_�[���Ăяo���Č������擾�ł��܂��B
	// �������Ȃ��V�F�[�_�[�́A��O�p�`�W�I���g���̃o�E���f�B���O�{�b�N�X�������ƌĂяo����܂��B
	// ����́A���̃`���[�g���A���͈̔͂𒴂��Ă��܂��B 
	// �q�b�g�̉\��������V�F�[�_�[�́A���ݓI�Ȍ����ŌĂяo����܂��B 
	// ���̃V�F�[�_�[�́A���Ƃ��΁A�A���t�@�e�X�g�����s���Ĉꕔ�̌�����j���ł��܂��B
	// �Ō�ɁA�ł��߂��q�b�g�̃v���O�����́A���C�̌��_�ɍł��߂���_�ŌĂяo����܂��B
	// �����3�̃V�F�[�_�[��1�̃q�b�g�O���[�v�ɂ܂Ƃ߂��܂��B

	// �O�p�`�̃W�I���g���̏ꍇ�A�����V�F�[�_�[���g�ݍ��܂�Ă��邱�Ƃɒ��ӂ��Ă��������B
	// ��̔C�Ӄq�b�g�V�F�[�_�[���f�t�H���g�Œ�`����Ă��邽�߁A
	// �P���ȃP�[�X�ł́A�e�q�b�g�O���[�v�ɂ͍ł��߂��q�b�g�V�F�[�_�[�݂̂��܂܂�Ă��܂��B
	// �G�N�X�|�[�g���ꂽ�V���{���͏�L�Œ�`����Ă��邽�߁A�V�F�[�_�[�͖��O�ŊȒP�ɎQ�Ƃł��܂��B

	// �V�F�[�_�[�����_�J���[���Ԃ��邾���ŁA�O�p�`�̃q�b�g�O���[�v
	pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");

	// ���̃Z�N�V�����ł́A���[�g�V�O�l�`�����e�V�F�[�_�[�Ɋ֘A�t���܂��B
	// �ꕔ�̃V�F�[�_�[���������[�g���������L���Ă��邱�Ƃ𖾎��I�Ɏ������Ƃ��ł��邱�Ƃɒ��ӂ��Ă�������
	//�i��FMiss��ShadowMiss�j�B 
	// �q�b�g�V�F�[�_�[�͌��݁A�q�b�g�O���[�v�Ƃ̂݌Ă΂�邱�Ƃɒ��ӂ��Ă��������B
	// �܂�A��ɂȂ�����Aany-hit�A�����nearest-hit�V�F�[�_�[�͓������[�g�V�O�l�`�������L���܂��B
	pipeline.AddRootSignatureAssociation(ray_gen_signature.Get(), { L"RayGen" });
	pipeline.AddRootSignatureAssociation(miss_signature.Get(), { L"Miss" });
	pipeline.AddRootSignatureAssociation(hit_signature.Get(), { L"HitGroup" });

	// �y�C���[�h�T�C�Y�́A���C�ɂ���ĉ^�΂��f�[�^�̍ő�T�C�Y���`���܂��B
	// HLSL�R�[�h��HitInfo�\���ȂǁA�V�F�[�_�[�ԂŌ��������f�[�^�B
	// �l����������ƁA�s�K�v�ȃ���������ƃL���b�V���g���b�V���O���������邽�߁A
	// ���̒l���ł��邾���Ⴍ�ۂ��Ƃ��d�v�ł��B
	pipeline.SetMaxPayloadSize(4 * sizeof(float)); // RGB + distance

	// DXR�́A�T�[�t�F�X�ɓ�����ƁA���̃q�b�g�ɂ������̑�����񋟂ł��܂��B
	// �T���v���ł́A�O�p�`�̍Ō��2�̒��_�̏d��u�Av�Œ�`���ꂽ�d�S���W���g�p���Ă��܂��B
	// ���ۂ̏d�S�́Afloat3 barycentrics = float3�i1.f-u-v�Au�Av�j;���g�p���Ď擾�ł��܂��B
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // �d�S���W

	// ���C�g���[�V���O�v���Z�X�ł́A�����̃q�b�g�|�C���g���烌�C������Ƃ��ł��邽�߁A
	// TraceRay�Ăяo�����l�X�g����܂��B
	// �T���v���R�[�h��1�������݂̂��g���[�X���邽�߁A�g���[�X�[�x1���K�v�ł��B
	// �ō��̃p�t�H�[�}���X�𓾂�ɂ́A���̍ċA�[�x���ŏ����ɗ}����K�v������܂��B
	// �p�X�g���[�V���O�A���S���Y���́A���������ŒP���ȃ��[�v�ɊȒP�Ƀt���b�g���ł��܂��B
	pipeline.SetMaxRecursionDepth(1);

	// GPU�Ŏ��s���邽�߂Ƀp�C�v���C�����R���p�C������
	rt_state_object.Attach(pipeline.Generate());

	// �X�e�[�g�I�u�W�F�N�g���v���p�e�B�I�u�W�F�N�g�ɃL���X�g���A
	// ��Ŗ��O�ŃV�F�[�_�[�|�C���^�[�ɃA�N�Z�X�ł���悤�ɂ��܂�
	HRESULT hr = rt_state_object->QueryInterface(IID_PPV_ARGS(rt_state_object_props.GetAddressOf()));
	RCHECK(FAILED(hr), "rt_state_object->QueryInterface�Ɏ��s");
	
}

// ���C�g���[�V���O�o�͂�ێ�����o�b�t�@���A�o�͉摜�Ɠ����T�C�Y�Ŋ��蓖�Ă܂�
void SceneMain::CreateRaytracingOutputBuffer(const DirectX::XMUINT2& screen_size)
{
	D3D12_RESOURCE_DESC res_desc = {};
	res_desc.DepthOrArraySize = 1;
	res_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	// �o�b�N�o�b�t�@�[�͎��ۂɂ�DXGI_FORMAT_R8G8B8A8_UNORM_SRGB�ł����A
	// sRGB�`����UAV�Ŏg�p�ł��܂���B 
	// ���m���̂��߂ɁA�V�F�[�_�[�Ŏ������g��sRGB�ɕϊ�����K�v������܂�
	res_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	res_desc.Width = screen_size.x;
	res_desc.Height = screen_size.y;
	res_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	res_desc.MipLevels = 1;
	res_desc.SampleDesc.Count = 1;

	HRESULT hr = device5->CreateCommittedResource(
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &res_desc,
		D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
		IID_PPV_ARGS(output_resource.GetAddressOf())
	);
	RCHECK(FAILED(hr), "RaytracingOutputBuffer��CreateCommittedResource�Ɏ��s");


}

// �V�F�[�_�[���g�p���郁�C���q�[�v���쐬���܂��B
// ����ɂ��A���C�g���[�V���O�o�͂ƍŏ�ʂ̉����\���ɃA�N�Z�X�ł��܂��B
void SceneMain::CreateShaderResourceHeap()
{
	// SRV / UAV / CBV�L�q�q�q�[�v���쐬���܂��B 
	// 2�̃G���g�����K�v�ł� - ���C�g���[�V���O�o�͗p��1 UAV��TLAS�p��1 SRV
	srv_uav_heap.Attach(
		nv_helpers_dx12::CreateDescriptorHeap(device5.Get(), 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true)
	);

	// �L�q�q�𒼐ڏ������ނ��Ƃ��ł���悤�ɁACPU���̃q�[�v�������ւ̃n���h�����擾���܂��B
	D3D12_CPU_DESCRIPTOR_HANDLE srv_handle =
		srv_uav_heap->GetCPUDescriptorHandleForHeapStart();

	// UAV���쐬���܂��B �쐬�������[�g�����Ɋ�Â��āA
	// �ŏ��̃G���g���ł��B Create * View���\�b�h�́A�r���[����srvHandle�ɒ��ڏ������݂܂�
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device5->CreateUnorderedAccessView(output_resource.Get(), nullptr, &uav_desc,
		srv_handle);

	// ���C�g���[�V���O�o�̓o�b�t�@�[�̒���Ƀg�b�v���x��AS SRV��ǉ�����
	srv_handle.ptr += device5->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc;
	srv_desc.Format = DXGI_FORMAT_UNKNOWN;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.RaytracingAccelerationStructure.Location =
		top_level_buffers.result->GetGPUVirtualAddress();
	// �q�[�v�ɃA�N�Z�����[�V�����\���r���[����������
	device5->CreateShaderResourceView(nullptr, &srv_desc, srv_handle);
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
		t_vert_view.SizeInBytes = SCAST<UINT>(sizeof(TriangleVertex) * vertex.size());

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

		// ���C�g���[�V���O�p�C�v���C�����쐬���A
		// �V�F�[�_�[�R�[�h���V���{�����Ƃ��̃��[�g�V�O�l�`���Ɋ֘A�t���A
		// ���C���^�ԃ������̗ʂ��`���܂��i���C�y�C���[�h�j
		CreateRaytracingPipeline();

		// ���C�g���[�V���O�o�͂��i�[����o�b�t�@���A�^�[�Q�b�g�C���[�W�Ɠ����T�C�Y�Ŋ��蓖�Ă܂�
		CreateRaytracingOutputBuffer(desc.window->GetClientSize());

		// ���C�g���[�V���O�̌��ʂ��܂ރo�b�t�@�[���쐬���i���UAV�ɏo�́j�A
		// ���C�g���[�V���O�Ŏg�p����郊�\�[�X�i�����\���Ȃǁj���Q�Ƃ���q�[�v���쐬���܂��B
		CreateShaderResourceHeap();
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