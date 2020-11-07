#include "ParticleStruct.hlsli"

cbuffer FrameBuffer : register(b0)
{
	uint					center_count;
	float					center_mass;
	float					elapsed_time;
	float					resistivity;
}
struct Parent
{
	float3 position;
};
ConstantBuffer<Parent> parents[] : register(b1);

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

	float3 resultant = (float3)0;
	particles[id].acceleration = (float3)0;

	for (uint i = 0; i < center_count; i++)
	{
		float3 vec = parents[i].position - particles[id].pos;
		float N = (G * particles[id].mass * center_mass) / dot(vec, vec);
		resultant += normalize(vec) * N;
		resultant += -(particles[id].velocity.xyz * (resistivity * particles[id].resistivity));
		particles[id].acceleration += resultant / particles[id].mass;
	}

	particles[id].velocity.xyz += particles[id].acceleration * elapsed_time;

	float3 frame_velocity = particles[id].velocity.xyz * elapsed_time;
	particles[id].move_length += length(frame_velocity);
	particles[id].pos.xyz += frame_velocity;
	particles[id].color += particles[id].color_speed * elapsed_time;
	particles[id].exist_time -= elapsed_time;
}