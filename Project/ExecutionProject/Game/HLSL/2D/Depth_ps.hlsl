#include "Depth.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	float dep = depth_tex.Sample(smp, input.uv);
	dep = pow(dep, 20);
	return float4(dep, dep, dep, 1.f);
}