#include "StaticModel.hlsli"

cbuffer ModelBuffer : register(b1)
{
	matrix world;
	matrix wvp;
};

struct VS_Input
{
	float3 position : POSITION;
	float3 uv		: TEXCOORD;
	float3 normal	: NORMAL;
};

PS_Input VSMain(VS_Input input)
{
	PS_Input output = (PS_Input)0;
	output.sv_position = mul(wvp, input.position);
	output.position = mul(world, input.position);
	output.normal = mul(world, input.normal);
	output.uv = input.uv;

	return output;
}