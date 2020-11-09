#include "Particle.hlsli"
#include "../Easings.hlsli"

#define FLT_EPSILON     1.192092896e-07

[maxvertexcount(VERTEX_COUNT)]
void GSMain(
	point Particle input[1],
	inout TriangleStream<PSInput> output
) {
	PSInput element = (PSInput)0;
	element.color = input[0].color;
	element.bloom = input[0].bloom;
	element.tex_id = input[0].tex_num;

	float3 pos = input[0].pos.xyz;
	float3 vel = input[0].velocity.xyz;
	float3 vel_norm = normalize(vel);
	float scale_width = input[0].scale_width * 0.5f;
	float scale_front = max(input[0].scale_front, scale_width);
	float scale_back = max(input[0].scale_back, scale_width);
	float scale_speed_width = input[0].scale_speed_width * 0.5f;

	// front
	float3 front_v = vel_norm * scale_front + vel * input[0].scale_speed_front;
	float3 front_pos = pos + normalize(front_v) * max(min(length(front_v), input[0].scale_front_max) - scale_width, 0.001f);
	// back
	float3 back_v = -(vel_norm * scale_back + vel * input[0].scale_speed_back);
	float3 back_pos = pos + normalize(back_v) * max(min(min(length(back_v), input[0].move_length), input[0].scale_back_max) -scale_width, 0.001f);

	// 0地点がカメラ位置なのでそのままベクトルとしてポジションを扱う
	float4 view_front_pos = mul(float4(front_pos, 1.0), view);
	float3 view_front_v = normalize(view_front_pos);
	float4 view_back_pos = mul(float4(back_pos, 1.0), view);
	float3 view_back_v = normalize(view_back_pos);

	// back to front vector
	float3 view_btfv = view_front_pos.xyz - view_back_pos.xyz;
	float3 view_btfv_norm = normalize(view_btfv);

	float3 view_front_axis = normalize(cross(view_btfv_norm, view_front_v));
	float3 view_back_axis = normalize(cross(view_btfv_norm, view_back_v));

	float3 view_top_v = normalize(cross(view_front_axis, view_front_v));
	float3 view_bottom_v = normalize(cross(view_back_axis, view_back_v));

	float speed_scale = length(vel) * scale_speed_width;
	float3 view_front_side_v = view_front_axis * scale_width + view_front_axis * speed_scale;
	float3 view_back_side_v = view_back_axis * scale_width + view_back_axis * speed_scale;

	float3 view_top_pos = view_front_pos - view_top_v * scale_width;
	float3 view_bottom_pos = view_back_pos + view_bottom_v * scale_width;

#if VERTEX_COUNT == 4
	float4 v[4];
	v[0] = float4(view_top_pos + view_front_side_v, 1.f);
	v[1] = float4(view_top_pos - view_front_side_v, 1.f);
	v[2] = float4(view_bottom_pos + view_back_side_v, 1.f);
	v[3] = float4(view_bottom_pos - view_back_side_v, 1.f);

	element.pos = mul(v[0], inv_view);
	element.sv_pos = mul(v[0], proj);
	element.uv = float2(0.f, 1.f);
	output.Append(element);

	element.pos = mul(v[1], inv_view);
	element.sv_pos = mul(v[1], proj);
	element.uv = float2(1.f, 1.f);
	output.Append(element);

	element.pos = mul(v[2], inv_view);
	element.sv_pos = mul(v[2], proj);
	element.uv = float2(0.f, 0.f);
	output.Append(element);

	element.pos = mul(v[3], inv_view);
	element.sv_pos = mul(v[3], proj);
	element.uv = float2(1.f, 0.f);
	output.Append(element);

#elif VERTEX_COUNT == 8

	float4 v[8];
	v[0] = float4(view_top_pos + view_front_side_v, 1.f);
	v[1] = float4(view_top_pos - view_front_side_v, 1.f);
	v[2] = float4(view_front_pos + view_front_side_v, 1.f);
	v[3] = float4(view_front_pos - view_front_side_v, 1.f);
	v[4] = float4(view_back_pos + view_back_side_v, 1.f);
	v[5] = float4(view_back_pos - view_back_side_v, 1.f);
	v[6] = float4(view_bottom_pos + view_back_side_v, 1.f);
	v[7] = float4(view_bottom_pos - view_back_side_v, 1.f);

	element.pos = mul(v[0], inv_view);
	element.sv_pos = mul(v[0], proj);
	element.uv = float2(0.f, 1.f);
	output.Append(element);

	element.pos = mul(v[1], inv_view);
	element.sv_pos = mul(v[1], proj);
	element.uv = float2(1.f, 1.f);
	output.Append(element);

	element.pos = mul(v[2], inv_view);
	element.sv_pos = mul(v[2], proj);
	element.uv = float2(0.f, 0.5f);
	output.Append(element);

	element.pos = mul(v[3], inv_view);
	element.sv_pos = mul(v[3], proj);
	element.uv = float2(1.f, 0.5f);
	output.Append(element);

	element.pos = mul(v[4], inv_view);
	element.sv_pos = mul(v[4], proj);
	element.uv = float2(0.f, 0.5f);
	output.Append(element);

	element.pos = mul(v[5], inv_view);
	element.sv_pos = mul(v[5], proj);
	element.uv = float2(1.f, 0.5f);
	output.Append(element);

	element.pos = mul(v[6], inv_view);
	element.sv_pos = mul(v[6], proj);
	element.uv = float2(0.f, 0.f);
	output.Append(element);

	element.pos = mul(v[7], inv_view);
	element.sv_pos = mul(v[7], proj);
	element.uv = float2(1.f, 0.f);
	output.Append(element);
#endif

	output.RestartStrip();
}