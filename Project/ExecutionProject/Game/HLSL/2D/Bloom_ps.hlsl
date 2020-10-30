#include "../GaussianBlur5x5.hlsli"

Texture2D<float4> tex : register (t0);

SamplerState smp : register (s0);

cbuffer FrameBuffer : register (b0)
{
	uint kernel;
	unorm float4 weight[2];
}
cbuffer GaussianBuffer : register (b1)
{
	float4 bkweights[2];
}

static float temp_weight[8] = (float[8])weight;
static float bk_weight[8] = (float[8])bkweights;

struct PSInput
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PSMainW(PSInput input) : SV_TARGET
{
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0 / w;

	float4 col = tex.Sample(smp, input.uv);
	float4 ret = float4(0.f, 0.f, 0.f, 0.f);

	ret += bkweights[0] * col;

	for (uint i = 0u; i < kernel; ++i)
	{
		ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, saturate(input.uv + float2(i * dx, 0))) * temp_weight[i];
		ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, saturate(input.uv + float2(-int(i) * dx, 0))) * temp_weight[i];
	}

	return ret;
}

float4 PSMainH(PSInput input) : SV_TARGET
{
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dy = 1.0 / h;

	float4 col = tex.Sample(smp, input.uv);
	float4 ret = float4(0.f, 0.f, 0.f, 0.f);

	ret += bkweights[0] * col;

	for (uint i = 0u; i < kernel; ++i)
	{
		ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, saturate(input.uv + float2(0, i * dy))) * temp_weight[i];
		ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, saturate(input.uv + float2(0, -int(i) * dy))) * temp_weight[i];
	}

	return ret;
}