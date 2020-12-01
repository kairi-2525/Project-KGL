
cbuffer FrameBuffer : register(b0)
{
	matrix view;
	matrix proj;
	matrix view_proj;
	float3 eye_pos;
};