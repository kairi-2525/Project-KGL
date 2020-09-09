#include "Particle.hlsli"

cbuffer World : register(b1)
{
	float3 position;
}

Particle VSMain(Particle input, uint inst_no : SV_InstanceID)
{
	Particle output = input;
	output.pos += position;
	return output;
}