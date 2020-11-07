#include "../Hrd/Particle.hpp"
#include "../Hrd/Fireworks.hpp"

static constexpr float G = 6.67e-11f;

void Particle::Update(float time, const ParticleParent* parent, const std::vector<Fireworks>& fireworks)
{
	using namespace DirectX;
	
	XMVECTOR resultant = XMVectorSet(0.f, 0.f, 0.f, 0.f);
	XMVECTOR accs = XMVectorSet(0.f, 0.f, 0.f, 0.f);

	XMVECTOR pos = XMLoadFloat3(&position);
	XMVECTOR vel = XMLoadFloat3(&velocity);
	
	size_t fw_size = fireworks.size();
	{
		XMVECTOR vec = XMVectorSet(0.f, -6378.1f * 1000.f, 0.f, 0.f) - pos;
		float l;
		XMStoreFloat(&l, XMVector3LengthSq(vec));
		float N = (G * mass * parent->center_mass) / l;
		resultant += XMVector3Normalize(vec) * N;
		resultant += -vel * (parent->resistivity * resistivity);
		accs += resultant / mass;
	}
	for (size_t i = 0; i < fw_size; i++)
	{
		XMVECTOR vec = XMLoadFloat3(&fireworks[i].pos) - pos;
		float l;
		XMStoreFloat(&l, XMVector3LengthSq(vec));
		float N = (G * mass * parent->center_mass) / l;
		resultant += XMVector3Normalize(vec) * N;
		resultant += -vel * (parent->resistivity * resistivity);
		accs += resultant / mass;
	}

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