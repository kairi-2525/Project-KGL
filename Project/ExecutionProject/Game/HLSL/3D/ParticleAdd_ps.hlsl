#include "Particle.hlsli"
#include "../GrayScale.hlsli"

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct PSOut
{
	float4 color : SV_TARGET0;
	float4 high_color : SV_TARGET1;
};

PSOut PSMain(PSInput input)
{
	PSOut output = (PSOut)0;

	output.color =  tex.Sample(smp, input.uv) * input.color;
	output.high_color = GrayScale(output.color) > 0.99f ? output.color : float4(0.f, 0.f, 0.f, 0.f);

	return output;
}