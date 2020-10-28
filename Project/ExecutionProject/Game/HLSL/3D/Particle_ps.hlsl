#include "Particle.hlsli"

Texture2D<float4> tex[] : register(t0);
SamplerState smp : register(s0);

struct PSOut
{
	float4 normal_color	: SV_TARGET0;
	float4 bloom_color	: SV_TARGET1;
};

PSOut PSMain(PSInput input) : SV_TARGET
{
	PSOut output = (PSOut)0;
	const float4 color = tex[input.tex_id - (input.tex_id * int(zero_texture))].SampleLevel(smp, input.uv, (clamp(input.pos.z, 0.f, 0.01f) * 100.f * (5.f - 1.f))) * input.color;
	if (input.bloom)
	{
		output.bloom_color = color;
	}
	else
	{
		output.normal_color = color;
	}
	return output;
}