#include "ParticleStruct.hlsli"

struct GSInput
{
	float3 pos : POSITION;
	float3 scale : SCALE;
	float exist_time : EXIST;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer scene_buff : register(b0)
{
	row_major matrix view;				// ビュー変換行列
	row_major matrix proj;				// プロジェクション変換行列
	row_major matrix light_camera;
	float3 eye;							// 視点
	float3 light_vec;
}