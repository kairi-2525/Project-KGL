#include "Global.hlsli"

cbuffer MaterialBuffer : register(b2)
{
	float3			ambient_color;
	float3			diffuse_color;
	float3			specular_color;
	float			specular_weight;
	bool			specular_flg;
	float			dissolve;	// �����x 1�Ȃ瓧��
	float			refraction;	// ���ܗ�
	bool			smooth;
};

struct VS_Input
{
	float3 position		: POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;

	// �@���������(Y��)�Ƃ����ꍇ�́AX���̃x�N�g��
	float3 tangent		: TANGENT;
	// Cross(normal, tangent) = bitangent;
	float3 bitangent	: BITANGENT;
};

typedef VS_Input GS_Input;

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