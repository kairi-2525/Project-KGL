#include "Sprite.hlsli"
#include "../GaussianBlur5x5.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	return GaussianBlur5x5(tex, smp, input.uv);
}