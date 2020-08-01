#include "Sprite.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	float4 color = tex.Sample(smp, input.uv);
	return color.r + color.g + color.b > 0.f ? float4(color.rgb, 1.f) : color;
}