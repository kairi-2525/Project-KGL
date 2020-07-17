cbuffer scene_buff : register(b0)
{
	row_major matrix view_proj;
	row_major matrix re_view;
	float			 elapsed_time;
}

struct Particle
{
	float3 pos : POSITION;
	float3 scale : SCALE;
	float3 velocity : VELOCITY;
	float3 accs : ACOS;
	bool exist : EXIST;
};

RWStructuredBuffer<Particle> particles : register(u0);

[numthreads(64, 1, 1)]
void CSMain( uint3 dtid : SV_DispatchThreadID )
{
	float id = dtid.x;
	if (id > 500000) return;
	if (!particles[id].exist) return;
	particles[id].pos += particles[id].velocity * elapsed_time;

}