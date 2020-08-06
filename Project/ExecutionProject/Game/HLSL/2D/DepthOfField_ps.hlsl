#include "../GaussianBlur5x5.hlsli"

Texture2D<float> depth_tex	: register (t0);
Texture2D<float4> tex[8]	: register (t1);

SamplerState smp			: register (s0);

cbuffer RTV					: register (b0)
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

	float depth_diff = abs(depth_tex.Sample(smp, float2(0.5f, 0.5f)) - depth_tex.Sample(smp, input.uv));

	float no;
	modf(depth_diff * rtv_num, no);

	float4 bloom_accum = float4(0.f, 0.f, 0.f, 0.f);
	[unroll(8)] for (int i = 0; i < no; i++)
	{
		bloom_accum += GaussianBlur5x5(tex[i], smp, input.uv);
	}

	return saturate(bloom_accum);
}