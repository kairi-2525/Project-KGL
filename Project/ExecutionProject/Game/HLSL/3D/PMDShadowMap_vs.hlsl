#include "PMDShaderHeader.hlsli"

//äÓñ{å`èÛóp
struct PrimitiveType {
	float4 svpos : SV_POSITION;
	float4 tpos : TPOS;
};

PrimitiveType VSMain(float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT,
	uint inst_no : SV_InstanceID)
{
	PrimitiveType output = (PrimitiveType)0;

	float w = weight / 100.0f;
	row_major matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1 - w);

	float4 world_pos = mul(mul(pos, bm), world);
	output.svpos = mul(world_pos, mul(view, proj));
	output.tpos = mul(world_pos, light_camera);
	return output;
}