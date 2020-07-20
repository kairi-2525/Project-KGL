cbuffer scene_buff : register(b0)
{
	row_major matrix view_proj;
	row_major matrix re_view;
	float			 elapsed_time;
}

struct Particle
{
	float4 pos : POSITION;
	float4 scale : SCALE;
	float4 velocity : VELOCITY;
	float3 accs : ACOS;
	float exist_time : EXIST;
};

RWStructuredBuffer<Particle> particles : register(u0);

[numthreads(64, 1, 1)]
void CSMain( uint3 dtid : SV_DispatchThreadID )
{
	uint id = dtid.x;
	if (id > 1000000) return;
	if (particles[id].exist_time <= 0.f) return;
	particles[id].pos += particles[id].velocity * elapsed_time;
	particles[id].exist_time -= elapsed_time;
}