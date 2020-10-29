#include "../Hrd/Particle.hpp"

static constexpr float G = 6.67e-11f;

void Particle::Update(float time, const ParticleParent* parent)
{
	using namespace DirectX;
	
	auto resultant = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR vel = XMLoadFloat3(&velocity);
	XMVECTOR vec = XMLoadFloat3(&parent->center_pos) - pos;
	float l;
	XMStoreFloat(&l, XMVector3LengthSq(vec));
	float N = (G * mass * parent->center_mass) / l;
	resultant += XMVector3Normalize(vec) * N;
	resultant += -vel * (parent->resistivity * resistivity);
	const XMVECTOR accs = resultant / mass;
	XMStoreFloat3(&this->accs, accs);
	vel += accs * time;
	XMStoreFloat3(&velocity, vel);
	const XMVECTOR frame_vel = vel * time;
	float frame_length;
	XMStoreFloat(&frame_length, XMVector3Length(frame_vel));
	move_length += frame_length;
	XMStoreFloat3(&position, pos + frame_vel);
	XMStoreFloat4(&color, XMLoadFloat4(&color) + (XMLoadFloat4(&color_speed) * time));
	exist_time -= time;
}