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
	payload.colorAndDistance = float4(0.9, 0.6, 0.2, 1);

	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).
	// ��ƍ��ڂ̃f�B�X�p�b�`���ꂽ2D�O���b�h���̏ꏊ���擾���܂�
	//�i�����̏ꍇ�s�N�Z���Ƀ}�b�v����邽�߁A����̓s�N�Z�����W��\�����Ƃ��ł��܂��j�B
	uint2 launchIndex = DispatchRaysIndex();

	gOutput[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
}
