#include "Sprite.hlsli"
#include "../GaussianBlur5x5.hlsli"

Texture2D<float4> tex : register (t0);
SamplerState smp : register (s0, space0);

float4 PSMain(PSInput input) : SV_TARGET
{
	return GaussianBlur5x5(tex, smp, input.uv);
}