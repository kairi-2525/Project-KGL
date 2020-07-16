#include "PMDGroundShadow.hlsli"

struct VSInput
{
	float4 pos : POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
	min16uint2 boneno : BONE_NO;
	min16uint weight : WEIGHT;
};

Output VSMain(VSInput input, uint inst_no : SV_InstanceID)
{
	Output output = (Output)0;

	float w = input.weight / 100.0f;
	row_major matrix bm = bones[input.boneno[0]] * w + bones[input.boneno[1]] * (1 - w);

	output.pos = mul(mul(input.pos, bm), world);

	output.inst_no = inst_no;
	if (inst_no == 1)
	{
		output.pos = mul(output.pos, shadow);
	}

	output.svpos = mul(mul(output.pos, view), proj);

	input.normal.w = 0;	// •½sˆÚ“®¬•ª‚ğ–³Œø‰»

	output.normal = mul(input.normal, world);
	output.vnormal = mul(output.normal, view);
	output.uv = input.uv;
	output.ray = normalize(input.pos.xyz - eye);
	return output;
}