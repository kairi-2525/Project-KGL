#include "Board.hlsli"

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
	// ‹P“x
	float brightness = dot(-light_vec, input.normal.xyz);
	return tex.Sample(smp, input.uv) * brightness * color;
}