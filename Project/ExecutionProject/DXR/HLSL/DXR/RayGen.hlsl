#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
// UAV�Ƃ��ăA�N�Z�X����郌�C�g���[�V���O�o�̓e�N�X�`��
RWTexture2D< float4 > gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
// SRV�Ƃ��ăA�N�Z�X����郌�C�g���[�V���O�����\��
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")] 
void RayGen() {
	// Initialize the ray payload
	// �����y�C���[�h������������
	HitInfo payload;
	payload.colorAndDistance = float4(0, 0, 0, 0);

	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).
	// ��ƍ��ڂ̃f�B�X�p�b�`���ꂽ2D�O���b�h���̏ꏊ���擾���܂�
	//�i�����̏ꍇ�s�N�Z���Ƀ}�b�v����邽�߁A����̓s�N�Z�����W��\�����Ƃ��ł��܂��j�B
	uint2 launch_index = DispatchRaysIndex().xy;
	float2 dims = float2(DispatchRaysDimensions().xy);
	float2 d = (((launch_index.xy + 0.5f) / dims.xy) * 2.f - 1.f);

	// ���_�A�����A�ŏ�-�ő勗���̒l�ō\�������������`���܂�
	RayDesc ray;
	ray.Origin = float3(d.x, -d.y, 1);
	ray.Direction = float3(0, 0, -1);
	ray.TMin = 0;
	ray.TMax = 100000;

	// ������ǐՂ���
	TraceRay(
		// �p�����[�^�[���FAccelerationStructure
		// �����\��
		SceneBVH,

		// �p�����[�^���FRayFlags
		// �t���O���g�p���āA�T�[�t�F�X�ɓ��������Ƃ��̓�����w��ł��܂�
		RAY_FLAG_NONE,

		// �p�����[�^�[���FInstanceInclusionMask
		// �C���X�^���X�C���N���[�W�����}�X�N�B������g�p���āA
		// �W�I���g���}�X�N�Ń}�X�N��AND-ING���邱�Ƃɂ��A
		// ���̌����ɑ΂��Ĉꕔ�̃W�I���g�����}�X�N�ł��܂��B 
		// 0xFF�t���O�́A�W�I���g�����}�X�N����Ȃ����Ƃ������܂�
		0xFF,

		// �p�����[�^�[���FRayContributionToHitGroupIndex
		// ���C�̃^�C�v�ɉ����āA����̃I�u�W�F�N�g�ɕ����̃q�b�g�O���[�v���A�^�b�`���邱�Ƃ��ł��܂�
		// �i�܂�A�ʏ�̃V�F�[�f�B���O���v�Z���邽�߂ɉ����Ƃ��̏����ƁA�V���h�E���v�Z���邽�߂ɉ����Ƃ��̏����j�B 
		// �����̃q�b�g�O���[�v��SBT�ŏ��ԂɎw�肳��邽�߁A
		// �ȉ��̒l�́A���̃��C�̃q�b�g�O���[�v�ɓK�p����I�t�Z�b�g�i4�r�b�g�j�������܂��B 
		// ���̃T���v���ł́A�I�u�W�F�N�g���Ƃ�1�̃q�b�g�O���[�v�����Ȃ����߁A�I�t�Z�b�g��0�ł��B
		0,

		// �p�����[�^���FMultiplierForGeometryContributionToHitGroupIndex
		// SBT�̃I�t�Z�b�g�́A�I�u�W�F�N�gID�A���̃C���X�^���XID����v�Z�ł��܂����A
		// �I�u�W�F�N�g���A�N�Z�����[�V�����\���Ƀv�b�V�����ꂽ�����ɂ���Ă��v�Z�ł��܂��B 
		// ����ɂ��A�A�v���P�[�V�����́AAS�ɒǉ����ꂽ�̂Ɠ���������SBT�̃V�F�[�_�[���O���[�v���ł��܂��B
		// ���̏ꍇ�A�ȉ��̒l�́A2�̘A������I�u�W�F�N�g�Ԃ̃X�g���C�h�i�q�b�g�O���[�v�̐���\��4�r�b�g�j��\���܂��B
		0,

		// �p�����[�^�[���FMissShaderIndex
		// SBT�ɕ����̘A������~�X�V�F�[�_�[�����݂���ꍇ�Ɏg�p����~�X�V�F�[�_�[�̃C���f�b�N�X�B 
		// ����ɂ��A�W�I���g���Ƀq�b�g���Ȃ������ꍇ�̃v���O�����̓����ύX�ł��܂��B
		// ���Ƃ��΁A�ʏ�̃����_�����O�ł͋�̐F��Ԃ��A����1�ł̓V���h�E���C�̊��S�ȉ����̒l��Ԃ��܂��B
		// ���̃T���v���ɂ̓~�X�V�F�[�_�[��1�����Ȃ����߁A�C���f�b�N�X0
		0,

		// �p�����[�^���FRay
		// �ǐՂ���������
		ray,

		// �p�����[�^���FPayload
		// ���C�Ɋ֘A�t����ꂽ�y�C���[�h�B�q�b�g/�~�X�V�F�[�_�[�ƃ��C�Q���Ԃ̒ʐM�Ɏg�p����܂��B
		payload
	);

	gOutput[launch_index] = float4(payload.colorAndDistance.rgb, 1.f);
}
