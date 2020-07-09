#include "Sprite.hlsli"

// F‚ÌŠK’²‚ğ‰º‚°‚é(1.0f / 0.25f@= 4ŠK’²)
float4 PSMain(PSInput input) : SV_TARGET
{
	float4 col = tex.Sample(smp, input.uv);
	return float4(col.rgb - fmod(col.rgb, 0.25f), col.a);
}