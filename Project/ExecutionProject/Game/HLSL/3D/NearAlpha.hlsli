cbuffer SceneBuffer : register(b0)
{
	row_major float4x4	wvp;
	row_major float4x4	world;
	float3				eye_pos;
	float				alpha_length_min;
	float				alpha_length_max;
};

struct PSInput
{
	float4 sv_pos : SV_POSITION;
	float4 pos : POSITION;
};