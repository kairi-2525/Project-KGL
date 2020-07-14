#include "PMDShaderHeader.hlsli"

Output VSMain(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT,
	uint inst_no : SV_InstanceID)
{
	Output output = (Output)0;

	float w = weight / 100.0f;
	row_major matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1 - w);

	float4 bm_pos = mul(pos, bm);
	output.svpos = mul(bm_pos, wvp);
	output.tpos = mul(mul(bm_pos, world), light_camera);

	normal.w = 0;	// •½sˆÚ“®¬•ª‚ğ–³Œø‰»

	output.normal = mul(normal, world);
	output.vnormal = mul(output.normal, view);
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);
	return output;
}