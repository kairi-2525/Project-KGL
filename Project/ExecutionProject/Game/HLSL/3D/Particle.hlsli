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
	row_major matrix view;				// ビュー変換行列
	row_major matrix proj;				// プロジェクション変換行列
	row_major matrix light_camera;
	float3 eye;							// 視点
	float3 light_vec;
	row_major matrix inv_view;			// ビュー変換逆行列
	bool zero_texture;
}