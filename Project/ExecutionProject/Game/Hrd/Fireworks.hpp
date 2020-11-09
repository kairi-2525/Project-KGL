#pragma once

#include "Effects.hpp"
#include <memory>

class Fireworks
{
private:
	std::vector<Effect>						effects;
	FireworksDesc							desc;
public:
	float									time;
	DirectX::XMFLOAT3						pos;
	DirectX::XMFLOAT3						velocity;
	float									resistivity;
	float									mass;
private:
	void Init(const FireworksDesc& desc, float time = -1.f);
public:
	Fireworks(const FireworksDesc& desc, float time = -1.f) { Init(desc, time); }
	Fireworks(const Fireworks& fw) { Init(fw.GetDesc(), -1.f); }

	const FireworksDesc& GetDesc() const { return desc; }
	bool Update(
		float time,
		std::vector<Particle>* p_particles,
		const ParticleParent* p_parent,
		std::vector<Fireworks>* p_fireworks,
		const std::vector<AffectObjects>& affect_objects,
		const std::vector<Fireworks>& affect_fireworks);
};