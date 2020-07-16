// ���_�V�F�[�_�[�ւ̂����Ɏg�p����\����
struct Output
{
	float4 svpos : SV_POSITION;	// �V�X�e���p���_���W
	float4 pos : POSITION;		// �|�W�V����
	float4 normal : NORMAL0;	// �@���x�N�g��
	float4 vnormal : NORMAL1;	// �r���[�ԊҌ�̖@���x�N�g��
	float2 uv : TEXCOORD;		// uv�l
	float3 ray : VECTOR;		// �x�N�g��
	float4 tpos : TPOS;
};

Texture2D<float4> material_tex : register(t0);
Texture2D<float4> material_sph : register(t1);
Texture2D<float4> material_spa : register(t2);
Texture2D<float4> material_toon : register(t3);

SamplerState smp : register(s0);
SamplerState toom_smp : register(s1);

cbuffer scene_buff : register(b0)
{
	row_major matrix view;				// �r���[�ϊ��s��
	row_major matrix proj;				// �v���W�F�N�V�����ϊ��s��
	row_major matrix light_camera;
	float3 eye;							// ���_
	float3 light_vec;
}
cbuffer model_buff : register(b1)
{
	row_major matrix wvp;				// ���[���h�r���[�v���W�F�N�V����(���炩����CPU�Ōv�Z���Ă���)
	row_major matrix world : rowmajor;	// ���[���h�ϊ��s��
	row_major matrix bones[512];		// �{�[���s��
}

cbuffer Material : register(b2)
{
	float4 diffuse;
	float4 specular;
	float3 ambient;
}