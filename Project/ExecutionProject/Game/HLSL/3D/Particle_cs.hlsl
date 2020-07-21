#include "ParticleStruct.hlsli"

cbuffer scene_buff : register(b0)
{
	float3					center_pos;
	float					center_mass;
	float					elapsed_time;
}

//RWStructuredBuffer<uint> counter : register(u0);
RWStructuredBuffer<Particle> particles : register(u0);

static float G = 6.67e-11f;

[numthreads(64, 1, 1)]
void CSMain( uint3 dtid : SV_DispatchThreadID )
{
	uint id = dtid.x;
	if (id > 1000000) return;
	if (particles[id].exist_time <= 0.f) return;
	particles.IncrementCounter();

	float3 resultant = float3(0, 0, 0);

	float3 vec = center_pos - particles[id].pos;

	float N = (G * particles[id].mass * center_mass) / dot(vec, vec);
	resultant += normalize(vec) * N;

	particles[id].acceleration = resultant / particles[id].mass;
	particles[id].velocity.xyz += particles[id].acceleration * elapsed_time;
	particles[id].pos.xyz += particles[id].velocity.xyz * elapsed_time;
	particles[id].exist_time -= elapsed_time;
}