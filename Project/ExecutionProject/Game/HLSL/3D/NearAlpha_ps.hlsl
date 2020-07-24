#include "NearAlpha.hlsli"
#include "../Easings.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	float3 vec = input.pos.xyz - eye_pos;
	float lsq = dot(vec, vec);
	float lmaxsq = alpha_length_max * alpha_length_max;
	float norm_length = saturate(lsq / lmaxsq);

	return float4(1.0f, 1.0f, 1.0f, 1.0f - EaseOutQuart(norm_length));
}