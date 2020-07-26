struct VSInput
{
	float4 pos	: POSITION;
	float2 uv	: TEXCOORD;
};

struct PSInput
{
	float4 pos	: SV_POSITION;
	float2 uv	: TEXCOORD;
};