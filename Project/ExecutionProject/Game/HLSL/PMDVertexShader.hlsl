#include "PMDShaderHeader.hlsli"

Output BasicVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT)
{
	Output output = (Output)0;
	output.svpos = mul(pos, wvp);
	normal.w = 0;	// 平行移動成分を無効化
	output.normal = mul(normal, world);
	output.vnormal = mul(output.normal, view);
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);
	return output;
}