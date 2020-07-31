
struct Particle
{
	float4	color			: COLOR;
	float3	pos				: POSITION;
	float	mass			: MASS;
	float	scale_width		: SCALE_WIDTH;
	float	scale_front		: SCALE_FRONT;
	float	scale_back		: SCALE_BACK;
	float	angle			: ANGLE;
	float4	velocity		: VELOCITY;
	float3	acceleration	: ACCS;
	float	exist_time		: EXIST;
};