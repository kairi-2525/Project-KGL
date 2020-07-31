#include "Particle.hlsli"

[maxvertexcount(4)]
void GSMain(
	point Particle input[1],
	inout TriangleStream<PSInput> output
)
{
	float3 pos = input[0].pos.xyz;
	float3 vel = input[0].velocity.xyz;
	float3 front_pos = pos + (vel * 1.1f);
	float4 view_pos = mul(float4(pos, 1.0), view);
	float4 view_front_pos = mul(float4(front_pos, 1.0), view);

	float speed = length(vel);
	static const float angle = radians(90);
	static const row_major float2x2 Rotation2D =
	{
		cos(angle), -sin(angle),
		sin(angle), cos(angle)
	};

	// 点を面にする
	float scale_width = input[0].scale_width * 0.5f;
	float scale_speed_width = input[0].scale_speed_width * 0.5f;
	
	/*float w = input[0].scale * 0.5f;
	float h = input[0].scale * 0.5f;*/
	float2 view_vec = view_front_pos.xy - view_pos.xy;
	float2 view_vec_norm = normalize(view_vec);
	float2 view_down_norm = mul(view_vec_norm, Rotation2D);
	float4 pos_lt = view_pos + float4(view_vec_norm * input[0].scale_front + view_vec * input[0].scale_speed_front, 0.0, 0.0);
	float4 pos_rb = view_pos - float4(view_vec_norm * input[0].scale_back + view_vec * input[0].scale_speed_back, 0.0, 0.0);
	float4 pos_rt = view_pos + float4(view_down_norm * scale_width + view_down_norm * speed * scale_speed_width, 0.0, 0.0);
	float4 pos_lb = view_pos - float4(view_down_norm * scale_width + view_down_norm * speed * scale_speed_width, 0.0, 0.0);

	PSInput element = (PSInput)0;
	element.color = input[0].color;

	// 左上の点の位置(射影座標系)
	element.pos = mul(pos_lt, proj);
	//element.pos.w = 1.f;
	element.uv = float2(0.f, 0.f);
	output.Append(element);
	// 右上の点の位置(射影座標系)
	element.pos = mul(pos_rt, proj);
	//element.pos.w = 1.f;
	element.uv = float2(1.f, 0.f);
	output.Append(element);
	// 左下の点の位置(射影座標系)
	element.pos = mul(pos_lb, proj);
	//element.pos.w = 1.f;
	element.uv = float2(0.f, 1.f);
	output.Append(element);
	// 右下の点の位置(射影座標系)
	element.pos = mul(pos_rb, proj);
	//element.pos.w = 1.f;
	element.uv = float2(1.f, 1.f);
	output.Append(element);
	output.RestartStrip();
}