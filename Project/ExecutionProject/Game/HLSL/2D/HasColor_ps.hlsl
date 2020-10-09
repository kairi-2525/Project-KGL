#include "Sprite.hlsli"

Texture2D<float4> tex : register (t0);
SamplerState smp : register (s0, space0);

float4 PSMain(PSInput input) : SV_TARGET
{
	float4 color = tex.Sample(smp, input.uv);
	return color.r + color.g + color.b > 0.f ? float4(color.rgb, 1.f) : color;
}