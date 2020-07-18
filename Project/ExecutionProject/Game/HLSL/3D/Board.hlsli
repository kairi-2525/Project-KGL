
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
	row_major matrix view;				// ビュー変換行列
	row_major matrix proj;				// プロジェクション変換行列
	row_major matrix light_camera;
	float3 eye;							// 視点
	float3 light_vec;
}
cbuffer model_buff : register(b1)
{
	row_major matrix wvp;				// ワールドビュープロジェクション(あらかじめCPUで計算しておく)
}