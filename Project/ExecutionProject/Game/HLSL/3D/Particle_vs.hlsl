#include "Particle.hlsli"

GSInput VSMain(Particle input, uint inst_no : SV_InstanceID)
{
	GSInput output = (GSInput)0;
	output.pos = input.pos.xyz;
	output.scale = input.scale.xyz;
	output.exist_time = input.exist_time;
	return output;
}