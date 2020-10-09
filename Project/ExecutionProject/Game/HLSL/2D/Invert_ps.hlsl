#include "Sprite.hlsli"

Texture2D<float4> tex : register (t0);
SamplerState smp : register (s0, space0);

// F”½“]
float4 PSMain(PSInput input) : SV_TARGET
{
	float4 col = tex.Sample(smp, input.uv);
	return float4(1.0f - col.rgb, col.a);
}