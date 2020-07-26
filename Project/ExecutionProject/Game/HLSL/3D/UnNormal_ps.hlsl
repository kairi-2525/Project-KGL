#include "UnNormal.hlsli"

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
	//return float4(1.f, 1.f, 1.f, 1.f);
	return tex.Sample(smp, input.uv);
}