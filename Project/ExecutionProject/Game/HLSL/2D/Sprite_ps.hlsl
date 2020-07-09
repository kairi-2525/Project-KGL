#include "Sprite.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(input.uv.x, 1, input.uv.y, 1);
}