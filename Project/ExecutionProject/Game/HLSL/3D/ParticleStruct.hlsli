
struct Particle
{
	float4	color			: COLOR;
	float4	pos				: POSITION;
	float	mass			: MASS;
	float	scale			: SCALE;
	float	scale_power		: SCALE_POWER;
	float	angle			: ANGLE;
	float4	velocity		: VELOCITY;
	float3	acceleration	: ACCS;
	float	exist_time		: EXIST;
};