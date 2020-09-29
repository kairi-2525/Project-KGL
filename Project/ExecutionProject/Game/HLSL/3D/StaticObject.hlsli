struct PSInput
{
	float4 sv_pos : SV_POSITION;
	float4 color : COLOR;
};

cbuffer Scene
{
	float4x4 view_proj;
};