#include "../Hrd/Particle.hpp"
#include "../Hrd/Fireworks.hpp"

static constexpr float G = 6.67e-11f;

void Particle::Update(float time, const ParticleParent* parent,
	const std::vector<AffectObjects>& affect_objects,
	const std::vector<Fireworks>& affect_fireworks)
{
	using namespace DirectX;
	
	XMVECTOR resultant = XMVectorSet(0.f, 0.f, 0.f, 0.f);

	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR vel = XMLoadFloat3(&velocity);
	
	for (const auto& affect_obj : affect_objects)
	{
		XMVECTOR vec = XMLoadFloat3(&affect_obj.pos) - pos;
		float l;
		XMStoreFloat(&l, XMVector3LengthSq(vec));
		float N = (G * mass * affect_obj.mass) / l;
		resultant += XMVector3Normalize(vec) * N;
	}
	for (const auto& affect_fw : affect_fireworks)
	{
		XMVECTOR vec = XMLoadFloat3(&affect_fw.pos) - pos;
		float l;
		XMStoreFloat(&l, XMVector3LengthSq(vec));
		float N = (G * mass * affect_fw.mass) / l;
		resultant += XMVector3Normalize(vec) * N;
	}
	resultant += -vel * (parent->resistivity * resistivity);
	XMVECTOR accs = resultant / mass;

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