#include "Particle.hlsli"

[maxvertexcount(4)]
void GSMain(
	point Particle input[1],
	inout TriangleStream<PSInput> output
)
{
	if (input[0].exist_time <= 0.f) return;
	// ���W�ϊ� (���[���h���W�n �� �r���[���W�n)
	float4 pos = mul(float4(input[0].pos.xyz, 1.0), view);
	float4 vel = mul(float4(normalize(input[0].velocity.xyz), 1.0), view);
	float speed = length(input[0].velocity.xyz);

	float angle = radians(90);
	// �_��ʂɂ���
	float w = input[0].scale * 0.5f;
	float h = input[0].scale * 0.5f;

	/*row_major float2x2 Rotation2D =
	{	1, 0,
		0, 1
	};*/

	/*row_major float2x2 Rotation90plus =
	{ 
		cos(angle), sin(angle),
		-sin(angle), cos(angle)
	};
	float2 width_vel = mul(vel.xy, Rotation90plus);

	float4 pos_lt = pos + vel * speed * h;
	float4 pos_rb = pos - vel * speed * h;
	float4 pos_rt = pos + float4(width_vel, 0.0, 0.0) * w;
	float4 pos_lb = pos - float4(width_vel, 0.0, 0.0) * w;*/

	float4 pos_lt = pos + float4(-w, h, 0.0, 0.0);
	float4 pos_rt = pos + float4(w, h, 0.0, 0.0);
	float4 pos_lb = pos + float4(-w, -h, 0.0, 0.0);
	float4 pos_rb = pos + float4(w, -h, 0.0, 0.0);

	PSInput element = (PSInput)0;
	element.color = input[0].color;

	// ����̓_�̈ʒu(�ˉe���W�n)
	element.pos = mul(pos_lt, proj);
	//element.pos.w = 1.f;
	element.uv = float2(0.f, 0.f);
	output.Append(element);
	// �E��̓_�̈ʒu(�ˉe���W�n)
	element.pos = mul(pos_rt, proj);
	//element.pos.w = 1.f;
	element.uv = float2(1.f, 0.f);
	output.Append(element);
	// �����̓_�̈ʒu(�ˉe���W�n)
	element.pos = mul(pos_lb, proj);
	//element.pos.w = 1.f;
	element.uv = float2(0.f, 1.f);
	output.Append(element);
	// �E���̓_�̈ʒu(�ˉe���W�n)
	element.pos = mul(pos_rb, proj);
	//element.pos.w = 1.f;
	element.uv = float2(1.f, 1.f);
	output.Append(element);
	output.RestartStrip();
}