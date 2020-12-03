#include "Global.hlsli"

struct PS_Input
{
	float4 sv_position	: SV_POSITION;
	float3 position		: POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;

	// �@���������(Y��)�Ƃ����ꍇ�́AX���̃x�N�g��
	float3 tangent		: TANGENT;
	// Cross(normal, tangent) = bitangent;
	float3 bitangent	: BITANGENT;
};