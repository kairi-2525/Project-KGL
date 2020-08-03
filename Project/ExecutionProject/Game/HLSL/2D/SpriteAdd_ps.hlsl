#include "Sprite.hlsli"
#include "../GrayScale.hlsli"


float4 PSMain(PSInput input) : SV_TARGET
{
	float4 color = tex.Sample(smp, input.uv);

	return GrayScale(color) > 1.f ? color : float4(0.f, 0.f, 0.f, 0.f);
}