#include "ParticleStruct.hlsli"

struct PSInput
{
	float4 color			: COLOR;
	float4 sv_pos			: SV_POSITION;
	float4 pos				: POSITION;
	float2 uv				: TEXCOORD;
	bool bloom				: RENDER_MODE;
	uint tex_id				: TEXTURE_ID;
};

cbuffer scene_buff : register(b0)
{
	row_major matrix view;				// �r���[�ϊ��s��
	row_major matrix proj;				// �v���W�F�N�V�����ϊ��s��
	row_major matrix light_camera;
	float3 eye;							// ���_
	float3 light_vec;
	row_major matrix inv_view;			// �r���[�ϊ��t�s��
	bool zero_texture;
}