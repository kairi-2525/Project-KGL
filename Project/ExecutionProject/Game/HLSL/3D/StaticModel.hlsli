#include "Global.hlsli"

cbuffer MaterialBuffer : register(b2)
{
	float3			ambient_color;
	float3			diffuse_color;
	float3			specular_color;
	float			specular_weight;
	bool			specular_flg;
	float			dissolve;	// 透明度 1なら透明
	float			refraction;	// 屈折率
	bool			smooth;
};

struct VS_Input
{
	float3 position		: POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;

	// 法線を上方向(Y軸)とした場合の、X軸のベクトル
	float3 tangent		: TANGENT;
	// Cross(normal, tangent) = bitangent;
	float3 bitangent	: BITANGENT;
};

typedef VS_Input GS_Input;

struct PS_Input
{
	float4 sv_position	: SV_POSITION;
	float3 position		: POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;

	// 法線を上方向(Y軸)とした場合の、X軸のベクトル
	float3 tangent		: TANGENT;
	// Cross(normal, tangent) = bitangent;
	float3 bitangent	: BITANGENT;
};