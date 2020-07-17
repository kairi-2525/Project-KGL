
float4 GaussianBlur5x5(Texture2D<float4> tex, SamplerState smp, float2 uv)
{
	float w, h, levels;
	tex.GetDimensions(0, w, h, levels);
	float dx = 1.0 / w;
	float dy = 1.0 / h;

	float4 ret = float4(0.f, 0.f, 0.f, 0.f);
	const float sp1 = 1.f;
	const float sp2 = sp1 * 2;

	ret += tex.Sample(smp, uv + float2(-sp2 * dx, -sp2 * dy)) * +1;
	ret += tex.Sample(smp, uv + float2(-sp1 * dx, -sp2 * dy)) * +4;
	ret += tex.Sample(smp, uv + float2(+0.f * dx, -sp2 * dy)) * +6;
	ret += tex.Sample(smp, uv + float2(+sp1 * dx, -sp2 * dy)) * +4;
	ret += tex.Sample(smp, uv + float2(+sp2 * dx, -sp2 * dy)) * +1;

	ret += tex.Sample(smp, uv + float2(-sp2 * dx, -sp1 * dy)) * +4;
	ret += tex.Sample(smp, uv + float2(-sp1 * dx, -sp1 * dy)) * +16;
	ret += tex.Sample(smp, uv + float2(+0.f * dx, -sp1 * dy)) * +24;
	ret += tex.Sample(smp, uv + float2(+sp1 * dx, -sp1 * dy)) * +16;
	ret += tex.Sample(smp, uv + float2(+sp2 * dx, -sp1 * dy)) * +4;

	ret += tex.Sample(smp, uv + float2(-sp2 * dx, +0.f * dy)) * +6;
	ret += tex.Sample(smp, uv + float2(-sp1 * dx, +0.f * dy)) * +24;
	ret += tex.Sample(smp, uv + float2(+0.f * dx, +0.f * dy)) * +36;
	ret += tex.Sample(smp, uv + float2(+sp1 * dx, +0.f * dy)) * +24;
	ret += tex.Sample(smp, uv + float2(+sp2 * dx, +0.f * dy)) * +6;

	ret += tex.Sample(smp, uv + float2(-sp2 * dx, +sp1 * dy)) * +4;
	ret += tex.Sample(smp, uv + float2(-sp1 * dx, +sp1 * dy)) * +16;
	ret += tex.Sample(smp, uv + float2(+0.f * dx, +sp1 * dy)) * +24;
	ret += tex.Sample(smp, uv + float2(+sp1 * dx, +sp1 * dy)) * +16;
	ret += tex.Sample(smp, uv + float2(+sp2 * dx, +sp1 * dy)) * +4;

	ret += tex.Sample(smp, uv + float2(-sp2 * dx, +sp2 * dy)) * +1;
	ret += tex.Sample(smp, uv + float2(-sp1 * dx, +sp2 * dy)) * +4;
	ret += tex.Sample(smp, uv + float2(+0.f * dx, +sp2 * dy)) * +6;
	ret += tex.Sample(smp, uv + float2(+sp1 * dx, +sp2 * dy)) * +4;
	ret += tex.Sample(smp, uv + float2(+sp2 * dx, +sp2 * dy)) * +1;

	return ret / 256;
}