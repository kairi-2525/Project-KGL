#include "PMDShaderHeader.hlsli"

Output BasicVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT)
{
	Output output = (Output)0;

	float w = weight / 100.0f;
	row_major matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1 - w);

	pos = mul(pos, bm);
	output.svpos = mul(pos, wvp);
	normal.w = 0;	// •½sˆÚ“®¬•ª‚ğ–³Œø‰»

	output.normal = mul(normal, world);
	output.vnormal = mul(output.normal, view);
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);
	return output;
}