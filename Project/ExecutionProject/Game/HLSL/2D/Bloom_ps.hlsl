#include "../GaussianBlur5x5.hlsli"

Texture2D<float4> tex[8] : register (t0);
Texture2DMS<float4> texms[8] : register (t0);

SamplerState smp : register (s0);

cbuffer RTV : register (b0)
{
	uint kernel;
	unorm float4 weight[2];
}

static float temp_weight[8] = (float[8])weight;

struct PSInput
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PSMainMS(PSInput input) : SV_TARGET
{
	float4 bloom_accum = float4(0.f, 0.f, 0.f, 0.f);

	for (int i = 0; i < kernel; i++)
	{
		bloom_accum += GaussianBlur5x5(texms[i], input.uv) * temp_weight[i];
	}

	return saturate(bloom_accum);
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float4 bloom_accum	= float4(0.f, 0.f, 0.f, 0.f);

	for (int i = 0; i < kernel; i++)
	{
		bloom_accum += GaussianBlur5x5(tex[i], smp, input.uv) * temp_weight[i];
	}

	return saturate(bloom_accum);
}