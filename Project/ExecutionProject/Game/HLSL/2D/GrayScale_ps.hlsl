#include "Sprite.hlsli"

// îíçïÇ…Ç∑ÇÈ
float4 PSMain(PSInput input) : SV_TARGET
{
	const float3 Convert_YUV = { 0.299f, 0.578f, 0.114f };
	float4 col =  tex.Sample(smp, input.uv);
	float Y = dot(col.rgb, Convert_YUV);
	return float4(Y, Y, Y, col.a);
}