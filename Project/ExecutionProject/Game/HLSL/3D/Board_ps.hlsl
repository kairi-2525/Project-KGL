#include "Board.hlsli"
float4 PSMain(PSInput input) : SV_TARGET
{
	// ‹P“x
	float brightness = dot(-light_vec, input.normal.xyz);

	return tex.Sample(smp, input.uv) * brightness;
}