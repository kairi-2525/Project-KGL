
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

float4 GaussianBlur5x5(Texture2DMS<float4> tex, float2 uv)
{
	float w, h, sample_count;
	tex.GetDimensions(w, h, sample_count);

	float4 ret = float4(0.f, 0.f, 0.f, 0.f);
	const float sp1 = 1.f;
	const float sp2 = sp1 * 2;

	// ピクセル単位のUV
	uint2 puv = uint2(uv.r * w, uv.g * h);

	ret += tex.Load(puv + uint2(-sp2, -sp2), 0) * +1;
	ret += tex.Load(puv + uint2(-sp1, -sp2), 0) * +4;
	ret += tex.Load(puv + uint2(+0.f, -sp2), 0) * +6;
	ret += tex.Load(puv + uint2(+sp1, -sp2), 0) * +4;
	ret += tex.Load(puv + uint2(+sp2, -sp2), 0) * +1;

	ret += tex.Load(puv + uint2(-sp2, -sp1), 0) * +4;
	ret += tex.Load(puv + uint2(-sp1, -sp1), 0) * +16;
	ret += tex.Load(puv + uint2(+0.f, -sp1), 0) * +24;
	ret += tex.Load(puv + uint2(+sp1, -sp1), 0) * +16;
	ret += tex.Load(puv + uint2(+sp2, -sp1), 0) * +4;

	ret += tex.Load(puv + uint2(-sp2, +0.f), 0) * +6;
	ret += tex.Load(puv + uint2(-sp1, +0.f), 0) * +24;
	ret += tex.Load(puv + uint2(+0.f, +0.f), 0) * +36;
	ret += tex.Load(puv + uint2(+sp1, +0.f), 0) * +24;
	ret += tex.Load(puv + uint2(+sp2, +0.f), 0) * +6;

	ret += tex.Load(puv + uint2(-sp2, +sp1), 0) * +4;
	ret += tex.Load(puv + uint2(-sp1, +sp1), 0) * +16;
	ret += tex.Load(puv + uint2(+0.f, +sp1), 0) * +24;
	ret += tex.Load(puv + uint2(+sp1, +sp1), 0) * +16;
	ret += tex.Load(puv + uint2(+sp2, +sp1), 0) * +4;

	ret += tex.Load(puv + uint2(-sp2, +sp2), 0) * +1;
	ret += tex.Load(puv + uint2(-sp1, +sp2), 0) * +4;
	ret += tex.Load(puv + uint2(+0.f, +sp2), 0) * +6;
	ret += tex.Load(puv + uint2(+sp1, +sp2), 0) * +4;
	ret += tex.Load(puv + uint2(+sp2, +sp2), 0) * +1;

	return ret / 256;
}