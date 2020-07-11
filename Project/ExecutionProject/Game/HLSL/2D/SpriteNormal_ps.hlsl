#include "Sprite.hlsli"

Texture2D<float4> effect_tex : register(t1);

float4 PSMain(PSInput input) : SV_TARGET
{
	float2 nm_tex = effect_tex.Sample(smp, input.uv).xy;
	// -1 ~ +1 ‚É•ÏŠ·
	nm_tex = nm_tex * 2.f - 1.f;

	return tex.Sample(smp, input.uv + nm_tex * 0.1f);
}