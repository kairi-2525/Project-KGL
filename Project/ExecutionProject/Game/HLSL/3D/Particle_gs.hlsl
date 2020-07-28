#include "Particle.hlsli"

[maxvertexcount(4)]
void GSMain(
	point Particle input[1],
	inout TriangleStream<PSInput> output
)
{
	if (input[0].exist_time <= 0.f) return;
	// 座標変換 (ワールド座標系 → ビュー座標系)
	float4 pos = mul(float4(input[0].pos.xyz, 1.0), view);
	float4 vel = mul(float4(normalize(input[0].velocity.xyz), 1.0), view);
	float speed = length(input[0].velocity.xyz);

	float angle = sqrt(dot(vel.xy, float2(1.f, 0.f)));
	// 点を面にする
	float w = input[0].scale * 0.5f;
	float h = input[0].scale * 0.5f * speed;

	row_major float2x2 Rotation2D =
	{	1, 0,
		0, 1
	};

	/*row_major float2x2 Rotation2D =
	{ cos(angle), sin(angle),
		-sin(angle), cos(angle)
	};*/

	float4 pos_lt = pos + float4(mul(float2(-w, h), Rotation2D), 0.0, 0.0);
	float4 pos_lb = pos + float4(mul(float2(-w, -h), Rotation2D), 0.0, 0.0);
	float4 pos_rt = pos + float4(mul(float2(w, h), Rotation2D), 0.0, 0.0);
	float4 pos_rb = pos + float4(mul(float2(w, -h), Rotation2D), 0.0, 0.0);

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