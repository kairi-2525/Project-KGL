#include "Sprite.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0 / w;
	float dy = 1.0 / h;

	float4 ret = float4(0.f, 0.f, 0.f, 0.f);
	const float spc = 1.f;
	ret += tex.Sample(smp, input.uv + float2(-spc * dx, -spc * dy));
	ret += tex.Sample(smp, input.uv + float2(+0.f * dx, -spc * dy));
	ret += tex.Sample(smp, input.uv + float2(+spc * dx, -spc * dy));

	ret += tex.Sample(smp, input.uv + float2(-spc * dx, +0.f * dy));
	ret += tex.Sample(smp, input.uv + float2(+0.f * dx, +0.f * dy));
	ret += tex.Sample(smp, input.uv + float2(+spc * dx, +0.f * dy));

	ret += tex.Sample(smp, input.uv + float2(-spc * dx, -spc * dy));
	ret += tex.Sample(smp, input.uv + float2(+0.f * dx, -spc * dy));
	ret += tex.Sample(smp, input.uv + float2(+spc * dx, -spc * dy));

	return ret / 9;
}