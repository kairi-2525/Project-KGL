#include "Particle.hlsli"
#include "../Easings.hlsli"

#define LOD_MIN 1.f
#define LOD_MAX 150.f
#define LOD_DIST_MAX LOD_MAX - LOD_MIN

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

	//uint width, height, mip_level_max;
	const int id = input.tex_id - (input.tex_id * int(zero_texture));

	//// LODÉXÉPÅ[ÉãÇåàÇﬂÇÈ
	//float eye_length = length(eye - input.pos);
	//float lod_scale = (clamp(eye_length, LOD_MIN, LOD_MAX) / LOD_DIST_MAX);
	////float lod_scale = EaseOutQuart(clamp(eye_length, LOD_MIN, LOD_MAX) / LOD_DIST_MAX);

	//tex[id].GetDimensions(0, width, height, mip_level_max);
	//const float mip_level = lod_scale * (mip_level_max - 1.f);

	//const float4 color = tex[id].SampleLevel(smp, input.uv, mip_level) * input.color;
	const float4 color = tex[id].Sample(smp, input.uv) * input.color;
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