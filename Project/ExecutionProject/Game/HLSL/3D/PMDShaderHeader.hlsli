// 頂点シェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION;	// システム用頂点座標
	float4 pos : POSITION;		// ポジション
	float4 normal : NORMAL0;	// 法線ベクトル
	float4 vnormal : NORMAL1;	// ビュー返還後の法線ベクトル
	float2 uv : TEXCOORD;		// uv値
	float3 ray : VECTOR;		// ベクトル
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
	row_major matrix view;				// ビュー変換行列
	row_major matrix proj;				// プロジェクション変換行列
	row_major matrix light_camera;
	float3 eye;							// 視点
	float3 light_vec;
}
cbuffer model_buff : register(b1)
{
	row_major matrix wvp;				// ワールドビュープロジェクション(あらかじめCPUで計算しておく)
	row_major matrix world : rowmajor;	// ワールド変換行列
	row_major matrix bones[512];		// ボーン行列
}

cbuffer Material : register(b2)
{
	float4 diffuse;
	float4 specular;
	float3 ambient;
}