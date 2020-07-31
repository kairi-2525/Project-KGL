
struct Particle
{
	float4	color					: COLOR;
	float3	pos						: POSITION;
	float	mass					: MASS;
	float	scale_width				: SCALE_WIDTH;
	float	scale_front				: SCALE_FRONT;
	float	scale_back				: SCALE_BACK;
	float	scale_speed_width		: SCALE_SPEED_WIDTH;
	float	scale_speed_front		: SCALE_SPEED_FRONT;
	float	scale_speed_back		: SCALE_SPEED_BACK;
	float	center_propotion		: CENTER_PROPOTION;
	float	angle					: ANGLE;
	float4	velocity				: VELOCITY;
	float3	acceleration			: ACCS;
	float	exist_time				: EXIST;
};