#include "Sprite.hlsli"

Texture2D<float4> tex : register (t0);
Texture2DMS<float4> texms : register (t0);
SamplerState smp : register (s0, space0);

float4 PSMain(PSInput input) : SV_TARGET
{
	return tex.Sample(smp, input.uv);
}

float4 PSMainMS(PSInput input) : SV_TARGET
{
	float w, h, sample_count;
	texms.GetDimensions(w, h, sample_count);
	return texms.Load(uint2(input.uv.r * w, input.uv.g * h), 0);
}