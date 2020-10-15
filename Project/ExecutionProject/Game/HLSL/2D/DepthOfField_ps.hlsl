#include "../GaussianBlur5x5.hlsli"

Texture2D<float> depth_tex	: register (t0);
Texture2D<float4> tex[8]	: register (t1);

SamplerState smp			: register (s0);

cbuffer RTV					: register (b0)
{
	uint rtv_num;
}

struct PSInput
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PSMain(PSInput input) : SV_TARGET
{
	float depth_center = depth_tex.Sample(smp, float2(0.5f, 0.5f));
	float depth = depth_tex.Sample(smp, input.uv);
	float depth_diff = abs(depth_center - depth);
	depth_diff = pow(depth_diff, 0.5);
	float no;
	float t = modf(depth_diff * rtv_num, no);

	float4 color[2];
	int tex_num_0 = max(no - 1, 0);
	int tex_num_1 = no;

	if (no == 0)
	{
		color[0] = tex[0].Sample(smp, input.uv);
	}
	else
	{
		color[0] = GaussianBlur5x5(tex[tex_num_0], smp, input.uv);
	}
	color[1] = GaussianBlur5x5(tex[tex_num_1], smp, input.uv);

	return lerp(color[0], color[1], t);
}