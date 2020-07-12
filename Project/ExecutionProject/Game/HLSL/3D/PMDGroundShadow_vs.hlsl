#include "PMDGroundShadow.hlsli"

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

	output.pos = mul(pos, bm);
	output.pos = mul(output.pos, world);

	output.inst_no = inst_no;
	if (inst_no == 1)
	{
		output.pos = mul(output.pos, shadow);
	}

	output.svpos = mul(mul(output.pos, view), proj);

	normal.w = 0;	// ïΩçsà⁄ìÆê¨ï™Çñ≥å¯âª

	output.normal = mul(normal, world);
	output.vnormal = mul(output.normal, view);
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);
	return output;
}