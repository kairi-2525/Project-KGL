#include "Particle.hlsli"

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

void PSMain(PSInput input)
{
	if (tex.Sample(smp, input.uv).a <= 0.f)
	{
		discard;
	}
}