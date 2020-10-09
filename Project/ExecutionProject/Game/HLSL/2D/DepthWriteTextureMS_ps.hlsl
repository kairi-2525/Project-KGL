#include "Sprite.hlsli"

Texture2DMS<float2> tex : register (t0);
SamplerState smp : register (s0, space0);

float PSMain(PSInput input) : SV_DEPTH
{
	uint w, h, sample_num;
	tex.GetDimensions(w, h, sample_num);
	return tex.Load(int2(input.uv.r * w, input.uv.g * h), 0).r;
}