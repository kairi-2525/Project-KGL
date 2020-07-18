
struct VSInput
{
	float4 pos : POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
};

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

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
}