// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.

// �q�b�g���A�ʖ�ray�y�C���[�h
// ���̃T���v���ɂ́A�V�F�[�f�B���O�J���[�ƃq�b�g�����݂̂��܂܂�Ă��܂��B
// �y�C���[�h�͂ł��邾���������ۂK�v������A
// ���̃T�C�Y�͑Ή�����D3D12_RAYTRACING_SHADER_CONFIG�p�C�v���C���T�u�I�u�W�F�N�g��
// �錾����K�v�����邱�Ƃɒ��ӂ��Ă��������B
struct HitInfo
{
  float4 colorAndDistance;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
// �T�[�t�F�X�ɓ��������Ƃ��Ƀ��C�g���[�V���O�ɂ���ďo�͂���鑮���A�����ł͏d�S���W
struct Attributes
{
  float2 bary;
};
