#include "Sprite.hlsli"
#include "../GaussianBlur5x5.hlsli"

float4 Generate(PSInput input) : SV_TARGET
{
	return tex.Sample(smp, input.uv);
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float4 bloom_accum	= float4(0.f, 0.f, 0.f, 0.f);
	float2 uv_size		= float2(0.5f, 0.5f);
	float2 uv_offset	= float2(0.0f, 0.0f);

	for (int i = 0; i < 8; i++)
	{
		bloom_accum += GaussianBlur5x5(tex, smp, input.uv * uv_size + uv_offset);
		uv_offset.y += uv_size.y;
		uv_size *= 0.5f;
	}

	return saturate(bloom_accum);
}