
struct Particle
{
	float4	color					: COLOR;
	float4	color_speed				: COLOR_SPEED;
	float3	pos						: POSITION;
	float	mass					: MASS;
	float	scale_width				: SCALE_WIDTH;
	float	scale_front				: SCALE_FRONT;
	float	scale_back				: SCALE_BACK;
	float	scale_speed_width		: SCALE_SPEED_WIDTH;
	float	scale_speed_front		: SCALE_SPEED_FRONT;
	float	scale_speed_back		: SCALE_SPEED_BACK;
	float	resistivity				: RESISTIVITY;
	float	angle					: ANGLE;
	float3	velocity				: VELOCITY;
	bool	bloom					: RENDER_MODE;
	float3	acceleration			: ACCS;
	float	exist_time				: EXIST;
	float	move_length				: MOVE_LENGTH;
	uint	tex_num					: TEXTURE_NUM;
	float	scale_front_max			: SCALE_FRONT_MAX;
	float	scale_back_max			: SCALE_BACK_MAX;
};