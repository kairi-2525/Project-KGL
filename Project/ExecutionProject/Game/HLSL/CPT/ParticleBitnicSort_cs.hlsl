#include "../3D/ParticleStruct.hlsli"

cbuffer FrameBuffer : register(b0)
{
	uint value_size;
}

cbuffer StepBuffer : register(b1)
{
	uint block;
	uint step;
}

// 出力バッファ
RWStructuredBuffer<Particle> particles : register(u0);

[numthreads(64, 1, 1)]
void CSMain(uint3 idx_tid : SV_DispatchThreadID)
{
	uint idx = idx_tid.x;

	if (idx >= 1 << value_size)
		return;

	uint e = idx ^ step;

	if (e > idx)
	{
		Particle v1 = particles[idx];
		Particle v2 = particles[e];

		if ((idx & block) != 0)
		{
			if (v1.exist_time > v2.exist_time)
			{
				particles[e] = v1;
				particles[idx] = v2;
			}
		}
		else
		{
			if (v1.exist_time < v2.exist_time)
			{
				particles[e] = v1;
				particles[idx] = v2;
			}
		}
	}
}