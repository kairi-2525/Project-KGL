#include "StaticModel.hlsli"

cbuffer ModelBuffer : register(b1)
{
	row_major matrix world;
	row_major matrix wvp;
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

PS_Input VSMain(VS_Input input)
{
	PS_Input output = (PS_Input)0;
	float4 pos = float4(input.position, 1.f);
	output.sv_position = mul(pos, wvp);
	output.position = mul(pos, world).rgb;
	output.normal = normalize(mul(float4(input.normal, 0.f), world)).rgb;
	output.tangent = normalize(mul(float4(input.tangent, 0.f), world)).rgb;
	output.bitangent = normalize(mul(float4(input.bitangent, 0.f), world)).rgb;
	output.uv = input.uv;

	return output;
}