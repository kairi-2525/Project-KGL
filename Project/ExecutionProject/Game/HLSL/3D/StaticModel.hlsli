#include "Global.hlsli"

struct PS_Input
{
	float4 sv_position	: SV_POSITION;
	float3 position		: POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;
};