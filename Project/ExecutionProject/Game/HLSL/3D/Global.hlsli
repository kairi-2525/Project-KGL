
cbuffer FrameBuffer : register(b0)
{
	row_major matrix	view;
	row_major matrix	proj;
	row_major matrix	view_proj;
	float3				eye_pos;
	float3				light_vec;
	float3				light_color;
	float3				ambient_light_color;
};