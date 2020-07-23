#pragma once

#include "Effects.hpp"
#include <memory>

struct Particle;
struct ParticleParent;

class Fireworks
{
public:
	struct Desc
	{
		DirectX::XMFLOAT3	pos;
		DirectX::XMFLOAT3	velocity;

		std::vector<Effect>	effects;
	};
private:
	struct EffectData
	{
		Effect effect;
		float update_timer;
		float late_counter;
		float late;

		void Update(DirectX::CXMVECTOR pos, DirectX::CXMVECTOR velocity,
			float time, std::vector<Particle>* p_particles, const ParticleParent* p_parent);
	};
private:

	std::vector<EffectData>					effects;
	Desc									desc;
public:
	DirectX::XMFLOAT3						pos;
	DirectX::XMFLOAT3						velocity;
private:
	void Init(const Desc& desc);
public:
	Fireworks(const Desc& desc) { Init(desc); }
	Fireworks(const Fireworks& fw) { Init(fw.GetDesc()); }

	const Desc& GetDesc() const { return desc; }
	bool Update(float time, std::vector<Particle>* p_particles, const ParticleParent* p_parent);
};