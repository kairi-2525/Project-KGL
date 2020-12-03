#include "Global.hlsli"

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