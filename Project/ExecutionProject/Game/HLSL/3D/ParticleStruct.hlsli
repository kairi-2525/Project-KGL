
struct Particle
{
	float3	pos				: POSITION;
	float	mass			: MASS;
	float3	scale			: SCALE;
	float	scale_power		: SCALE_POWER;
	float4	velocity		: VELOCITY;
	float3	acceleration	: ACCS;
	float	exist_time		: EXIST;
};