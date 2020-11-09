#pragma once

#include <DirectXMath.h>
#include <vector>

using UINT32 = unsigned __int32;
class Fireworks;

struct ParticleParent
{
	UINT32				affect_obj_count;
	float				elapsed_time;
	float				resistivity;
};

struct AffectObjects
{
	DirectX::XMFLOAT3 pos;
	float mass;
};

struct Particle
{
	DirectX::XMFLOAT4	color;
	DirectX::XMFLOAT4	color_speed;
	DirectX::XMFLOAT3	position;
	float				mass;
	float				scale_width;
	float				scale_front;
	float				scale_back;
	float				scale_speed_width;
	float				scale_speed_front;
	float				scale_speed_back;
	float				resistivity;
	float				angle;
	DirectX::XMFLOAT3	velocity;
	bool				bloom;
	DirectX::XMFLOAT3	accs;
	float				exist_time;
	float				move_length;
	uint32_t			texture_num;
	float				scale_front_max;
	float				scale_back_max;
public:
	bool Alive() { return exist_time > 0; }
	void Update(float time, const ParticleParent* parent, 
		const std::vector<AffectObjects>& affect_objects,
		const std::vector<Fireworks>& affect_fireworks);
};