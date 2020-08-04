#include "../GaussianBlur5x5.hlsli"

Texture2D<float4> tex[8] : register (t0);

SamplerState smp : register (s0);

cbuffer RTV : register (b0)
{
	uint rtv_num;
}

struct PSInput
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_TARGET
{
	float4 bloom_accum	= float4(0.f, 0.f, 0.f, 0.f);

	for (int i = 0; i < rtv_num; i++)
	{
		bloom_accum += GaussianBlur5x5(tex[i], smp, input.uv);
	}

	return saturate(bloom_accum);
}