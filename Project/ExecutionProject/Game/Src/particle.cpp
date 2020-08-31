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
	XMVECTOR accs = resultant / mass;
	XMStoreFloat3(&this->accs, accs);
	vel += accs * time;
	XMStoreFloat3(&velocity, vel);
	XMStoreFloat3(&position, pos + vel * time);
	XMStoreFloat4(&color, XMLoadFloat4(&color) + XMLoadFloat4(&color_speed));
	exist_time -= time;
}