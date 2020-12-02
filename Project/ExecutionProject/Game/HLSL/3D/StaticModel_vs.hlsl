#include "StaticModel.hlsli"

cbuffer ModelBuffer : register(b1)
{
	row_major matrix world;
	row_major matrix wvp;
};

struct VS_Input
{
	float3 position : POSITION;
	float2 uv		: TEXCOORD;
	float3 normal	: NORMAL;
};

PS_Input VSMain(VS_Input input)
{
	PS_Input output = (PS_Input)0;
	float4 pos = float4(input.position, 1.f);
	output.sv_position = mul(pos, wvp);
	output.position = mul(pos, world);
	output.normal = mul(input.normal, world);
	output.uv = input.uv;

	return output;
}