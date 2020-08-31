#include "NearAlpha.hlsli"
#include "../Easings.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	float3 vec = input.pos.xyz - eye_pos.xyz;
	float lsq = dot(vec, vec);
	float lmaxsq = alpha_length_max * alpha_length_max;
	float lminsq = alpha_length_min * alpha_length_min;
	float norm_max_length = saturate((lsq - lminsq) / (lmaxsq - lminsq));
	//float norm_min_length = saturate(lminsq / lmaxsq);
	float alpha = 1.0f - EaseOutQuart(norm_max_length);
	if (alpha <= 0.f)
		discard;
	return float4(1.0f, 1.0f, 1.0f, alpha);
}