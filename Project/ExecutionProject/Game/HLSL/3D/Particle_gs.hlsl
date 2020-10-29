#include "Particle.hlsli"

//static const float angle = radians(90);
//static const row_major float2x2 Rotation2D =
//{
//	cos(angle), -sin(angle),
//	sin(angle), cos(angle)
//};

//[maxvertexcount(4)]
//void GSMain(
//	point Particle input[1],
//	inout TriangleStream<PSInput> output
//)
//{
//	float3 pos = input[0].pos.xyz;
//	float3 vel = input[0].velocity.xyz;
//	float3 vel_norm = normalize(vel);
//
//	float3 front_pos = pos + (vel_norm * input[0].scale_front + vel * input[0].scale_speed_front);
//	float3 back_pos = pos - (vel_norm * input[0].scale_back + vel * input[0].scale_speed_back);
//	float4 view_pos = mul(float4(pos, 1.0), view);
//	float4 view_front_pos = mul(float4(front_pos, 1.0), view);
//	float4 view_back_pos = mul(float4(back_pos, 1.0), view);
//
//	float speed = length(vel);
//	static const float angle = radians(90);
//	static const row_major float2x2 Rotation2D =
//	{
//		cos(angle), -sin(angle),
//		sin(angle), cos(angle)
//	};
//
//	// 点を面にする
//	float scale_width = input[0].scale_width * 0.5f;
//	float scale_speed_width = input[0].scale_speed_width * 0.5f;
//	
//	/*float w = input[0].scale * 0.5f;
//	float h = input[0].scale * 0.5f;*/
//
//	float2 view_front_vec = view_front_pos.xy - view_pos.xy;
//	float front_length = length(view_front_vec);
//	view_front_vec = normalize(view_front_vec);
//	float2 view_down_norm = mul(view_front_vec, Rotation2D);
//	view_front_vec = view_front_vec * max(front_length, scale_width);
//
//	float2 view_back_vec = view_back_pos.xy - view_pos.xy;
//	float back_length = length(view_back_vec);
//	view_back_vec = normalize(view_back_vec);
//	view_back_vec = view_back_vec * max(back_length, scale_width);
//
//	float4 pos_lt = view_front_pos;
//	float4 pos_rb = view_back_pos;
//	//float4 pos_lt = float4(view_pos.xy + view_front_vec, view_front_pos.zw);
//	//float4 pos_rb = float4(view_pos.xy + view_back_vec, view_back_pos.zw);
//	float4 pos_rt = view_pos + float4(view_down_norm * scale_width + view_down_norm * speed * scale_speed_width, 0.0, 0.0);
//	float4 pos_lb = view_pos - float4(view_down_norm * scale_width + view_down_norm * speed * scale_speed_width, 0.0, 0.0);
//
//	PSInput element = (PSInput)0;
//	element.color = input[0].color;
//	element.bloom = input[0].bloom;
//	// 左上の点の位置(射影座標系)
//	element.pos = mul(pos_lt, proj);
//	//element.pos.w = 1.f;
//	element.uv = float2(0.f, 0.f);
//	output.Append(element);
//	// 右上の点の位置(射影座標系)
//	element.pos = mul(pos_rt, proj);
//	//element.pos.w = 1.f;
//	element.uv = float2(1.f, 0.f);
//	output.Append(element);
//	// 左下の点の位置(射影座標系)
//	element.pos = mul(pos_lb, proj);
//	//element.pos.w = 1.f;
//	element.uv = float2(0.f, 1.f);
//	output.Append(element);
//	// 右下の点の位置(射影座標系)
//	element.pos = mul(pos_rb, proj);
//	//element.pos.w = 1.f;
//	element.uv = float2(1.f, 1.f);
//	output.Append(element);
//	output.RestartStrip();
//}

//[maxvertexcount(8)]
//void GSMain(
//	point Particle input[1],
//	inout TriangleStream<PSInput> output
//)
//{
//	float3 pos = input[0].pos.xyz;
//	float3 vel = input[0].velocity.xyz;
//	float3 vel_norm = normalize(vel);
//	float scale_width = input[0].scale_width * 0.5f;
//
//	// front
//	float3 front_v = vel_norm * input[0].scale_front + vel * input[0].scale_speed_front;
//	float3 front_pos = pos + front_v;
//	float3 front_center_pos = pos + normalize(front_v) * max(length(front_v) - scale_width, 0.f);
//	// back
//	float3 back_v = -(vel_norm * input[0].scale_back + vel * input[0].scale_speed_back);
//	float3 back_pos = pos + back_v;
//	float3 back_center_pos = pos + normalize(back_v) * max(length(back_v) - scale_width, 0.f);
//
//	float4 view_pos = mul(float4(pos, 1.0), view);
//	float4 view_front_pos = mul(float4(front_pos, 1.0), view);
//	float4 view_back_pos = mul(float4(back_pos, 1.0), view);
//	float4 view_frontct_pos = mul(float4(front_center_pos, 1.0), view);
//	float4 view_backct_pos = mul(float4(back_center_pos, 1.0), view);
//
//	float speed = length(vel);
//
//	// 点を面にする
//	//float scale_width = input[0].scale_width * 0.5f;
//	float scale_speed_width = input[0].scale_speed_width * 0.5f;
//
//	float3 view_front_vec = view_front_pos.xyz - view_frontct_pos.xyz;
//	float front_length = length(view_front_vec);
//	float2 view_side_norm = mul(normalize(view_front_vec.xy), Rotation2D);
//
//	float2 side_v = view_side_norm * scale_width + view_side_norm * speed * scale_speed_width;
//
//	view_front_vec = normalize(view_front_vec);
//	float3 view_front_axis = -normalize(cross(float3(view_side_norm, 0), float3(0, 0, 1)));
//	view_front_vec = view_front_axis * scale_width;
//
//	float3 view_back_vec = view_back_pos.xyz - view_backct_pos.xyz;
//	float back_length = length(view_back_vec);
//	view_back_vec = normalize(view_back_vec);
//	float3 view_back_axis = -view_front_axis;
//	view_back_vec = view_back_axis * scale_width;
//
//	float4 v[8];
//	float4 view_front_npos = float4(view_frontct_pos.xyz + view_front_vec, 1.f);
//	float4 view_back_npos = float4(view_backct_pos.xyz + view_back_vec, 1.f);
//	v[0] = view_front_npos + float4(side_v, 0.0, 0.0);
//	v[1] = view_front_npos - float4(side_v, 0.0, 0.0);
//	v[2] = view_frontct_pos + float4(side_v, 0.0, 0.0);
//	v[3] = view_frontct_pos - float4(side_v, 0.0, 0.0);
//	v[4] = view_backct_pos + float4(side_v, 0.0, 0.0);
//	v[5] = view_backct_pos - float4(side_v, 0.0, 0.0);
//	v[6] = view_back_npos + float4(side_v, 0.0, 0.0);
//	v[7] = view_back_npos - float4(side_v, 0.0, 0.0);
//
//	PSInput element = (PSInput)0;
//	element.color = input[0].color;
//	element.bloom = input[0].bloom;
//
//	element.pos = mul(v[0], proj);
//	element.uv = float2(0.f, 1.f);
//	output.Append(element);
//
//	element.pos = mul(v[1], proj);
//	element.uv = float2(1.f, 1.f);
//	output.Append(element);
//
//	element.pos = mul(v[2], proj);
//	element.uv = float2(0.f, 0.5f);
//	output.Append(element);
//
//	element.pos = mul(v[3], proj);
//	element.uv = float2(1.f, 0.5f);
//	output.Append(element);
//
//	element.pos = mul(v[4], proj);
//	element.uv = float2(0.f, 0.5f);
//	output.Append(element);
//
//	element.pos = mul(v[5], proj);
//	element.uv = float2(1.f, 0.5f);
//	output.Append(element);
//
//	element.pos = mul(v[6], proj);
//	element.uv = float2(0.f, 0.f);
//	output.Append(element);
//
//	element.pos = mul(v[7], proj);
//	element.uv = float2(1.f, 0.f);
//	output.Append(element);
//
//	output.RestartStrip();
//}

//[maxvertexcount(4)]
//void GSMain(
//	point Particle input[1],
//	inout TriangleStream<PSInput> output
//)
//{
//	float3 pos = input[0].pos.xyz;
//	float3 vel = input[0].velocity.xyz;
//	float3 vel_norm = normalize(vel);
//	float scale_width = input[0].scale_width * 0.5f;
//
//	// front
//	float3 front_v = vel_norm * input[0].scale_front + vel * input[0].scale_speed_front;
//	float3 front_pos = pos + front_v;
//	float3 front_center_pos = pos + normalize(front_v) * max(length(front_v) - scale_width, 0.f);
//	// back
//	float3 back_v = -(vel_norm * input[0].scale_back + vel * input[0].scale_speed_back);
//	float3 back_pos = pos + back_v;
//	float3 back_center_pos = pos + normalize(back_v) * max(length(back_v) - scale_width, 0.f);
//
//	float4 view_pos = mul(float4(pos, 1.0), view);
//	float4 view_front_pos = mul(float4(front_pos, 1.0), view);
//	float4 view_back_pos = mul(float4(back_pos, 1.0), view);
//	float4 view_frontct_pos = mul(float4(front_center_pos, 1.0), view);
//	float4 view_backct_pos = mul(float4(back_center_pos, 1.0), view);
//
//	float speed = length(vel);
//
//	// 点を面にする
//	//float scale_width = input[0].scale_width * 0.5f;
//	float scale_speed_width = input[0].scale_speed_width * 0.5f;
//
//	float3 view_front_vec = view_front_pos.xyz - view_frontct_pos.xyz;
//	float front_length = length(view_front_vec);
//	float2 view_side_norm = mul(normalize(view_front_vec.xy), Rotation2D);
//
//	float2 side_v = view_side_norm * scale_width + view_side_norm * speed * scale_speed_width;
//
//	view_front_vec = normalize(view_front_vec);
//	float3 view_front_axis = -normalize(cross(float3(view_side_norm, 0), float3(0, 0, 1)));
//	view_front_vec = view_front_axis * scale_width;
//
//	float3 view_back_vec = view_back_pos.xyz - view_backct_pos.xyz;
//	float back_length = length(view_back_vec);
//	view_back_vec = normalize(view_back_vec);
//	float3 view_back_axis = -view_front_axis;
//	view_back_vec = view_back_axis * scale_width;
//
//	float4 v[4];
//	float4 view_front_npos = float4(view_frontct_pos.xyz + view_front_vec, 1.f);
//	float4 view_back_npos = float4(view_backct_pos.xyz + view_back_vec, 1.f);
//	v[0] = view_front_npos + float4(side_v, 0.0, 0.0);
//	v[1] = view_front_npos - float4(side_v, 0.0, 0.0);
//	v[2] = view_back_npos + float4(side_v, 0.0, 0.0);
//	v[3] = view_back_npos - float4(side_v, 0.0, 0.0);
//
//	PSInput element = (PSInput)0;
//	element.color = input[0].color;
//	element.bloom = input[0].bloom;
//
//	element.pos = mul(v[0], proj);
//	element.uv = float2(0.f, 1.f);
//	output.Append(element);
//
//	element.pos = mul(v[1], proj);
//	element.uv = float2(1.f, 1.f);
//	output.Append(element);
//
//	/*element.pos = mul(v[2], proj);
//	element.uv = float2(0.f, 0.5f);
//	output.Append(element);
//
//	element.pos = mul(v[3], proj);
//	element.uv = float2(1.f, 0.5f);
//	output.Append(element);
//
//	element.pos = mul(v[4], proj);
//	element.uv = float2(0.f, 0.5f);
//	output.Append(element);
//
//	element.pos = mul(v[5], proj);
//	element.uv = float2(1.f, 0.5f);
//	output.Append(element);*/
//
//	element.pos = mul(v[2], proj);
//	element.uv = float2(0.f, 0.f);
//	output.Append(element);
//
//	element.pos = mul(v[3], proj);
//	element.uv = float2(1.f, 0.f);
//	output.Append(element);
//
//	output.RestartStrip();
//}

[maxvertexcount(4)]
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
	float scale_speed_width = input[0].scale_speed_width * 0.5f;

	// front
	float3 front_v = vel_norm * input[0].scale_front + vel * input[0].scale_speed_front;
	float3 front_pos = pos + front_v;
	// back
	float3 back_v = -(vel_norm * input[0].scale_back + vel * input[0].scale_speed_back);
	float3 back_pos = pos + back_v;

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

	//float3 view_top_v = normalize(cross(view_front_axis, view_front_v));
	//float3 view_bottom_v = normalize(cross(view_back_axis, view_back_v));

	float speed_scale = length(vel) * scale_speed_width;
	float3 view_front_side_v = view_front_axis * scale_width + view_front_axis * speed_scale;
	float3 view_back_side_v = view_back_axis * scale_width + view_back_axis * speed_scale;

	//float3 view_top_pos = view_front_pos - view_top_v * scale_width;
	//float3 view_bottom_pos = view_back_pos + view_bottom_v * scale_width;

	float4 v[4];
	v[0] = float4(view_front_pos + view_front_side_v, 1.f);
	v[1] = float4(view_front_pos - view_front_side_v, 1.f);
	v[2] = float4(view_back_pos + view_back_side_v, 1.f);
	v[3] = float4(view_back_pos - view_back_side_v, 1.f);

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

	output.RestartStrip();
}