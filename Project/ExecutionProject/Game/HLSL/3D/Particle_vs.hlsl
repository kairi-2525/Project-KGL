#include "Particle.hlsli"

GSInput VSMain(Particle input, uint inst_no : SV_InstanceID)
{
	GSInput output = (GSInput)0;
	output.pos = input.pos.xyz;
	output.scale = input.scale;
	output.exist_time = input.exist_time;
	output.color = input.color;
	return output;
}