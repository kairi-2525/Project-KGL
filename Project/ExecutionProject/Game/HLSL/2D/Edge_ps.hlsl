#include "Sprite.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0 / w;
	float dy = 1.0 / h;

	float4 ret = float4(0.f, 0.f, 0.f, 0.f);
	const float spc = 1.f;
	ret += tex.Sample(smp, input.uv + float2(+0.f * dx, -spc * dy)) * -1;

	ret += tex.Sample(smp, input.uv + float2(-spc * dx, +0.f * dy)) * -1;
	ret += tex.Sample(smp, input.uv + float2(+0.f * dx, +0.f * dy)) * +4;
	ret += tex.Sample(smp, input.uv + float2(+spc * dx, +0.f * dy)) * -1;

	ret += tex.Sample(smp, input.uv + float2(+0.f * dx, -spc * dy)) * -1;

	const float3 Convert_YUV = { 0.299f, 0.578f, 0.114f };

	float Y = dot(ret.rgb, Convert_YUV);

	Y = pow(1.f - Y, 25.f);
	Y = step(0.1, Y);

	return float4(Y, Y, Y, ret.a);
}