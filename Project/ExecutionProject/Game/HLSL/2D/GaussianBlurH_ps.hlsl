#include "Sprite.hlsli"

Texture2D<float4> tex : register (t0);
SamplerState smp : register (s0, space0);

cbuffer PostEffect : register (b0)
{
	float4 bkweights[2];
}

float4 PSMain(PSInput input) : SV_TARGET
{
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	//float dx = 1.0 / w;
	float dy = 1.0 / h;
	float4 col = tex.Sample(smp, input.uv);
	float4 ret = float4(0.f, 0.f, 0.f, 0.f);

	ret += bkweights[0] * col;

	for (uint i = 1u; i < 8u; ++i)
	{
		ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, i * dy));
		ret += bkweights[i >> 2][i % 4] * tex.Sample(smp, input.uv + float2(0, -int(i) * dy));
	}

	return float4(ret.rgb, col.a);
}