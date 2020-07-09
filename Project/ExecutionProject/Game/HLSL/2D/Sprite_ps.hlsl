#include "Sprite.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	return tex.Sample(smp, input.uv);
}