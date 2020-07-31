#pragma once

#include <DirectXMath.h>

struct ParticleParent
{
	DirectX::XMFLOAT3	center_pos;
	float				center_mass;
	float				elapsed_time;
	float				resistivity;
};

struct Particle
{
	DirectX::XMFLOAT4	color;
	DirectX::XMFLOAT3	position;
	float				mass;
	float				scale_width;
	float				scale_front;
	float				scale_back;
	float				angle;
	DirectX::XMFLOAT3	velocity;
	float				pad1;
	DirectX::XMFLOAT3	accs;
	float exist_time;

public:
	bool Alive() { return exist_time > 0; }
	void Update(float time, const ParticleParent* parent);
};